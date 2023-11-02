#include "peerwireclient.h"
#include "external/asio-1.28.0/include/asio.hpp"
#include "external/cryptopp/sha.h"
#include <functional>
#include <random>

PieceQueue::PieceQueue(size_t num, size_t size, std::string pieceStr) : 
    numPieces(num), pieceSize(size), pieces(pieceStr) {
    for (size_t i = 0; i < numPieces; i++) {
        undownloaded.push_back(i);
    }
    std::random_device dev;
    std::mt19937 rng(dev());
    std::shuffle(undownloaded.begin(), undownloaded.end(), rng);
}

PeerWireClient::PeerWireClient(asio::io_context &ioc, std::string addrStr, size_t peerNum, 
    std::string hs, PieceQueue& pq) : 
    pieceQueue(pq), peerNumber(peerNum), handshake(hs), ioContext(ioc), socket(ioc), acceptor(ioc) {

    // Convert the address and port from raw binary string to usable types
    std::string ipStr = addrStr.substr(0, 4);
    std::array<uint8_t, 4> rawIP;
    std::copy(std::begin(ipStr), std::end(ipStr), std::begin(rawIP));
    addr = asio::ip::make_address_v4(rawIP);
    std::string portStr = addrStr.substr(4, 2);
    uint8_t portByte1 = (uint8_t)portStr[0];
    uint8_t portByte2 = (uint8_t)portStr[1];
    port = ((asio::ip::port_type)portByte1 << 8) |
        (asio::ip::port_type)portByte2;
    peerPrefix = "[Peer " + std::to_string(peerNumber) + "] ";
    peerValidated = false;
    bitfLength = pq.numPieces/8 + (pq.numPieces % 8 != 0);
    // TODO: add this to host mode
    pieceInMemory.resize(pieceQueue.pieceSize);
    // TODO: for testing only, REMOVE!!
    currentPiece = 5;
    currentBlock = 0;
    startConnection();
}

// TODOs to finish project
// 1. implement piece requesting
// 2. fix the flow of messages, e.g. when should I be requesting pieces, etc.
// 3. construct file from pieces
// 4. handle peer reconnecting/timeouts/etc.

// host mode
PeerWireClient::PeerWireClient(asio::io_context &ioc, std::string hs, PieceQueue& pq, 
    asio::ip::port_type port) :
    pieceQueue(pq), handshake(hs), ioContext(ioc), socket(ioc), 
    acceptor(ioc, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {

    auto callback = std::bind(&PeerWireClient::handleAccept, this, std::placeholders::_1);
    acceptor.async_accept(socket, callback);
}

void PeerWireClient::handleAccept(const asio::error_code &error) {
    if (error) {
        std::cerr << "Accept error: " << error.message() << std::endl;
    } else {
        std::cout << "Peer connected at " << socket.remote_endpoint().address().to_string();
        std::cout << std::endl;
        peerPrefix = "[" + socket.remote_endpoint().address().to_string() + "] ";
        
        std::cout << peerPrefix << "Writing handshake.\n";
        auto callback = std::bind(&PeerWireClient::handleWrite, this, std::placeholders::_1,
            std::placeholders::_2);
        asio::async_write(socket, asio::buffer(handshake, HANDSHAKE_LENGTH), callback);
    }
}

// TODO: how should we handle refused connections?
// TODO: connections are frequently refused. This might be normal but I need to look into it.
// I don't print the connection errors because it tends to flood the terminal
void PeerWireClient::startConnection() {
    asio::ip::tcp::endpoint endpoint(addr, port);
    auto callback = std::bind(&PeerWireClient::handleConnect, this, std::placeholders::_1);
    socket.async_connect(endpoint, callback);
}

// Perform the initial stuff required when connecting, e.g. the handshake
void PeerWireClient::handleConnect(const asio::error_code &error) {
    if (!error) {
        std::cout << "Connected to peer " << peerNumber << " at " << addr.to_string() << ":" 
            << port << std::endl;
        auto callback = std::bind(&PeerWireClient::handleWrite, this, std::placeholders::_1,
            std::placeholders::_2);

        // TODO: write handshake to messageBuffer? Does it even matter
        // socket.async_write_some(asio::buffer(handshake), callback);
        std::cout << peerPrefix << "Writing handshake.\n";
        asio::async_write(socket, asio::buffer(handshake, HANDSHAKE_LENGTH), callback);
    } else {
        std::cerr << "Connection error: " << error.message() << std::endl;

    }
}

/**
 * Handles async_write call by initiating another read.
*/
void PeerWireClient::handleWrite(const asio::error_code &error, size_t bytesWritten) {
    if (error) {
        std::cerr << peerPrefix << "Write err: " << error.message() << std::endl;
        return;
    }

    std::cout << peerPrefix << "Wrote " << bytesWritten << " bytes.\n";

    auto callback = std::bind(&PeerWireClient::handleRead, this, std::placeholders::_1, 
        std::placeholders::_2);
    //asio::async_read(socket, asio::buffer(messageBuffer), callback);
    socket.async_read_some(asio::buffer(messageBuffer), callback);
}

/**
 * Handles async_read_some call. Parses the message(s), writes the response to writeBuffer, and 
 * initates another async_write call if there is work to do.
*/
void PeerWireClient::handleRead(const asio::error_code &error, size_t bytesRead) {
    if (error) {
        std::cerr << peerPrefix << "Read err: " << error.message() << std::endl;
        return;
    }

    // TODO: why is this here?    
    if (!socket.is_open()) {
        return;
    }

    // helps differentiate peers in terminal output
    std::cout << std::endl;
    std::cout << peerPrefix << "Received response of " << bytesRead << " bytes\n";

    // TODO: feels a little weird that we do this check every time we get new messages
    // I think it would make more sense to have a separate handleFirstRead method
    if (!peerValidated) {
        checkHandshake();
        bytesRead -= HANDSHAKE_LENGTH;
        std::array<char, MSG_BUF_SIZE> temp;
        for (size_t i = 0; i < bytesRead; i++) {
            temp[i] = messageBuffer[i + HANDSHAKE_LENGTH];
        }
        std::copy_n(std::begin(temp), bytesRead, std::begin(messageBuffer));
        std::cout << peerPrefix << "Handshake validated\n";
    }

    std::vector<PeerWireMessage> inboundMessageList = generateMessageList(bytesRead);
    for (PeerWireMessage msg : inboundMessageList) {
        processMessage(msg);
    }

    std::vector<PeerWireMessage> outboundMessageList;
    // TODO: decide what messages to send
    // TODO: once piece requesting is working, fix this so that it sends the proper number of requests,
    // etc.
    if (peerChoking) { 
        outboundMessageList.push_back(PeerWireMessage(1, MessageID::Interested, 
            std::vector<uint8_t>()));
    } else {
        PeerWireMessage msgRequest = generateNextRequest();
        msgRequest.printReadable();
        std::cout << std::endl;
        outboundMessageList.push_back(msgRequest);
    }

    size_t bytesWritten = writeMessageListToBuffer(outboundMessageList);

    if (bytesWritten == 0) {
        auto callback = std::bind(&PeerWireClient::handleRead, this, std::placeholders::_1, 
            std::placeholders::_2);
        asio::async_read(socket, asio::buffer(messageBuffer), callback);
    } else {
        auto callback = std::bind(&PeerWireClient::handleWrite, this, std::placeholders::_1,
            std::placeholders::_2);
        asio::async_write(socket, asio::buffer(writeBuffer, bytesWritten), callback);
    }
}

/**
 * Process a peer wire protocol message
*/
void PeerWireClient::processMessage(PeerWireMessage msg) {
    std::cout << peerPrefix << "Processing message:\n";

    if (msg.lengthPrefix == 0) {
        std::cout << "Keep alive\n";
        return;
    }

    switch (msg.id) {
        case (MessageID::Choke):
            std::cout << "Choked\n";
            peerChoking = true;
            break;
        case (MessageID::Unchoke):
            std::cout << "Unchoked\n";
            peerChoking = false;
            break;
        case (MessageID::Interested):
            std::cout << "Interested\n";
            peerInterested = true;
            break;
        case (MessageID::NotInterested):
            std::cout << "Not Interested\n";
            peerInterested = false;
            break;
        case (MessageID::Have):
        {
            std::cout << "Have\n";
            uint32_t pieceIndex = ((uint32_t)(msg.payload[0]) << 24) |
                ((uint32_t)(msg.payload[1]) << 16) |
                ((uint32_t)(msg.payload[2]) << 8) |
                (uint32_t)(msg.payload[3]);
            bitfield.setPiece(pieceIndex);
            break;
        }
        case (MessageID::Bitfield):
            bitfield = BitfieldStorage(msg.payload);
            break;   
        case (MessageID::Request):
            std::cout << "Request\n";
            // ignore requests for now
            break;
        case (MessageID::Piece):
            std::cout << "Piece\n";
            processPieceMessage(msg);
            abort();
        case (MessageID::Cancel):
            // TODO
            std::cout << "Cancel\n";
            abort();
        default:
            std::cout << "Unknown message type\n";
            abort();
    }
}

/**
 * Write the messages in messageList to the outgoing message buffer and return the number of bytes 
 * written
*/
size_t PeerWireClient::writeMessageListToBuffer(std::vector<PeerWireMessage> messageList) {
    // TODO: it might be possible to simplify all of this with htonl and memcpy, e.g.
    // memcpy(writeBuffer, htonl(lengthPrefix)) or something if that's possible
    
    size_t writeIndex = 0;

    for (PeerWireMessage msg : messageList) {
        uint32_t lengthPrefix = msg.lengthPrefix;
        if (writeIndex + lengthPrefix + 4 > writeBuffer.size()) {
            // TODO: throw exception?
            std::cerr << "Failed to write message to buffer: not enough space.\n";
            return writeIndex;
        }

        writeBuffer[writeIndex] = (char)(uint8_t)(lengthPrefix >> 24);
        writeBuffer[writeIndex + 1] = (char)(uint8_t)(lengthPrefix >> 16);
        writeBuffer[writeIndex + 2] = (char)(uint8_t)(lengthPrefix >> 8);
        writeBuffer[writeIndex + 3] = (char)(uint8_t)(lengthPrefix);

        // keep alive
        if (lengthPrefix == 0) {
            writeIndex += 4;
            break;
        }

        writeBuffer[writeIndex + 4] = (char)(msg.id);

        for (size_t i = 0; i < msg.payload.size(); i++) {
            writeBuffer[writeIndex + 5 + i] = (char)msg.payload[i];
        }

        writeIndex += lengthPrefix + 4;
    }

    return writeIndex;
}

/**
 * Read from the incoming message buffer and generate a list of PeerWireMessages
*/
std::vector<PeerWireMessage> PeerWireClient::generateMessageList(size_t bytesRead) {
    size_t readIndex = 0;
    std::vector<PeerWireMessage> messageList;

    if (leftoverBytes != 0) {
        if (leftoverBytes + bytesRead > MSG_BUF_SIZE) {
            // TODO: fix this
            std::cerr << "leftoverBytes + bytesRead = " << (leftoverBytes + bytesRead) << std::endl;
            abort();
        }
        std::array<char, MSG_BUF_SIZE> temp;
        std::copy_n(leftoverBuffer.begin(), leftoverBytes, temp.begin());
        std::copy_n(messageBuffer.begin(), bytesRead, temp.begin() + leftoverBytes);
        
        bytesRead += leftoverBytes;
        leftoverBytes = 0;
        std::copy_n(temp.begin(), bytesRead, messageBuffer.begin());
    }

    while(readIndex < bytesRead) {

        if (readIndex + 4 > bytesRead) {
            std::cerr << peerPrefix << "Failed to read message from buffer: incomplete message.\n";
            break;
        }

        uint32_t lengthPrefix = ((uint32_t)(uint8_t)(messageBuffer[readIndex]) << 24) |
        ((uint32_t)(uint8_t)(messageBuffer[readIndex + 1]) << 16) |
        ((uint32_t)(uint8_t)(messageBuffer[readIndex + 2]) << 8) |
        (uint32_t)(uint8_t)(messageBuffer[readIndex + 3]);

        // keep alive
        if (lengthPrefix == 0) {
            messageList.push_back(PeerWireMessage());
            readIndex += 4;
            continue;
        }

        if (readIndex + lengthPrefix + 4 > bytesRead) {      
            std::cout << "Incomplete message. Copying to leftover buffer.\n";
            leftoverBytes = bytesRead - readIndex;
            std::copy_n(messageBuffer.begin() + readIndex,  leftoverBytes, leftoverBuffer.begin());
            break;
        }

        // it is possible for id to be outside the range of defined codes, but we allow this
        MessageID id = (MessageID)(messageBuffer[readIndex + 4]);

        std::vector<uint8_t> payload;
        for (size_t i = 0; i < (lengthPrefix - 1); i++) {
            payload.push_back((uint8_t)(messageBuffer[readIndex + 5 + i]));
        }

        messageList.push_back(PeerWireMessage(lengthPrefix, id, payload));
        readIndex += lengthPrefix + 4;
    }

    return messageList;
}

// TODO: just have to decide which piece to request
PeerWireMessage PeerWireClient::generateNextRequest() {
    // TODO: get the actual pieceIndex
    uint32_t pieceIndex = 5;
    uint32_t pieceBegin = 0;
    
    // something like this
    /*
    uint32_t pieceIndex = pieceQueue.front();
    pieceQueue.pop_front();
    if (!bitfield.hasPiece(pieceIndex)) {
        pieceQueue.push_back(pieceIndex);
        ...
    }
    */

    // request: <len=0013><id=6><index><begin><length>
    std::vector<uint8_t> payload;
    
    // index
    payload.push_back((uint8_t)(pieceIndex >> 24));
    payload.push_back((uint8_t)(pieceIndex >> 16));
    payload.push_back((uint8_t)(pieceIndex >> 8));
    payload.push_back((uint8_t)pieceIndex);

    // begin
    payload.push_back((uint8_t)(pieceBegin >> 24));
    payload.push_back((uint8_t)(pieceBegin >> 16));
    payload.push_back((uint8_t)(pieceBegin >> 8));
    payload.push_back((uint8_t)pieceBegin);

    // length
    payload.push_back((uint8_t)(BLOCK_SIZE >> 24));
    payload.push_back((uint8_t)(BLOCK_SIZE >> 16));
    payload.push_back((uint8_t)(BLOCK_SIZE >> 8));
    payload.push_back((uint8_t)BLOCK_SIZE);

    PeerWireMessage requestMsg = PeerWireMessage(13, MessageID::Request, payload);

    // TODO: what to do if there's no valid request?
    return requestMsg;
}

// Processes a Piece message, writing the block to pieceInMemory, and writing pieceInMemory to file
// if this is the last block in the piece.
void PeerWireClient::processPieceMessage(PeerWireMessage msg) {
    // piece: <len=0009+X><id=7><index><begin><block>
    // get piece and block indices
    uint32_t pieceIndex = (uint32_t)(msg.payload[0]) << 24 |
        (uint32_t)(msg.payload[1]) << 16 |
        (uint32_t)(msg.payload[2]) << 8 |
        (uint32_t)(msg.payload[3]);
    uint32_t pieceBegin = (uint32_t)(msg.payload[4]) << 24 |
        (uint32_t)(msg.payload[5]) << 16 |
        (uint32_t)(msg.payload[6]) << 8 |
        (uint32_t)(msg.payload[7]);
    if (pieceIndex != currentPiece || pieceBegin != currentBlock) {
        std::cerr << "Incorrect piece index or block index received.\n";
        return;
    }
 
    std::copy(msg.payload.begin() + 8, msg.payload.end(), pieceInMemory.begin() + currentBlock);
    currentBlock += BLOCK_SIZE;

    std::cout << "Piece copied to pieceInMemory.\n";

    if (currentBlock > pieceQueue.pieceSize) {
        // verify SHA1 hash (TODO: test)
        CryptoPP::byte digest[CryptoPP::SHA1::DIGESTSIZE];
        CryptoPP::SHA1().CalculateDigest(digest, pieceInMemory.data(), pieceInMemory.size());
        std::array<uint8_t, 20> metainfoHash;
        // TODO: test this, implicit type conversion could cause problems
        std::copy_n(pieceQueue.pieces.begin() + (pieceIndex * 20), 20, metainfoHash.begin());
        if (std::equal(metainfoHash.begin(), metainfoHash.end(), 
            std::begin(digest), std::end(digest))) {
            std::cout << "Piece " << pieceIndex << ": Hash verified.\n";
            abort();
            // TODO: write the piece to memory
        } else {
            std::cerr << "Piece " << pieceIndex << ": Incorrect hash.\n";
            abort();
            // TODO: add the piece back to the queue, etc.
        }
        

        // TODO: get new currentPiece
        currentBlock = 0;
    }
}

// Check that the handshake is valid and close the connection if it's not.
// TODO: the current design of this method is sort of weird. I think that its not modular because
// it doesn't take care of the entire job of handling the handshake, but it also does more than just
// checking if its valid and returning a bool.
void PeerWireClient::checkHandshake() {
    std::string infoHash = handshake.substr(28, 20);

    uint8_t pStrLen = messageBuffer[0];
    std::string pStr = "";
    for (size_t i = 0; i < pStrLen; i++) {
        pStr += messageBuffer[i+1];
    }
    // this is where you would handle reserved bits if they become used
    peerID = "";
    std::string peerInfoHash = "";
    for (size_t i = 0; i < 20; i++) {
        peerInfoHash += messageBuffer[i+pStrLen+9];
        peerID += messageBuffer[i+pStrLen+29];
    }
    
    if (infoHash != peerInfoHash) {
        std::cerr << peerPrefix << "Incorrect info hash!\n";
        dropConnection();
        return;
    }
    
    if (pStr != "BitTorrent protocol") {
        std::cerr << peerPrefix << "Unrecognized protocol version: " << pStr << std::endl;
        dropConnection();
        return;
    }

    peerValidated = true;
}

void PeerWireClient::dropConnection() {
    asio::error_code ec;
    socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    if (ec) { std::cerr << ec.message() << std::endl; }
    socket.close(); 
}

PeerWireMessage::PeerWireMessage() {
    lengthPrefix = 0;
}

PeerWireMessage::PeerWireMessage(uint32_t lengthPref, MessageID mID, std::vector<uint8_t> pl) : 
    lengthPrefix(lengthPref), id(mID), payload(pl) {}

void PeerWireMessage::printReadable() {

    std::cout << "Length Prefix: " << lengthPrefix << std::endl;
    std::cout << "Message ID: " << (uint32_t)id << std::endl;
    if (payload.size() == 0) { return; }

    std::cout << "Payload: " << std::hex;
    for (size_t i = 0; i < payload.size(); i++) {
        int currByte = payload[i];
        std::cout << currByte << " ";
    }
    std::cout << std::dec << std::endl;
}

BitfieldStorage::BitfieldStorage(std::vector<uint8_t> bitf) : data(bitf) {}

// TODO: test this
bool BitfieldStorage::hasPiece(uint32_t pieceIndex) {
    uint8_t bfByte = data[pieceIndex/8]; 
    uint8_t bfBit = (bfByte >> (7 - (pieceIndex % 8))) & 0x01;
    return (bfBit == 0x01);
}

// TODO: test this
void BitfieldStorage::setPiece(uint32_t pieceIndex) {
    uint8_t bfMask = 0x01 << (7 - (pieceIndex % 8));
    data[pieceIndex/8] |= bfMask;
}