#include "peerwireclient.h"
#include "external/asio-1.28.0/include/asio.hpp"
#include "external/cryptopp/sha.h"
#include <functional>
#include <random>

PieceData::PieceData(uint32_t pIndex, uint32_t pSize) : pieceIndex(pIndex), pieceSize(pSize) {
    data.resize(pieceSize);
    size_t numBlocks = pieceSize / BLOCK_SIZE + (pieceSize % BLOCK_SIZE != 0);
    for (size_t i = 0; i < numBlocks; i++) {
        need.push_back(i);
        blockInfo.push_back(false);
    }
}

bool PieceData::finished()
{
    for (size_t i = 0; i < blockInfo.size(); i++) {
        if (!blockInfo[i]) { return false; }
    }
    return true;
}

std::vector<uint32_t> PieceData::getRequests() {
    std::vector<uint32_t> requests;

    for (size_t i = 0; i < QUEUE_LENGTH; i++) {
        if (need.empty()) { break; }
        requests.push_back(need.front() * BLOCK_SIZE);
        need.pop_front();
    }

    return requests;
}

void PieceData::requeueBlocks() {
    for (size_t i = 0; i < blockInfo.size(); i++) {
        if (!blockInfo[i]) {
            need.push_back(i);
        }
    }
}

bool PieceData::verifyHash(std::array<uint8_t, 20> hash) {
    CryptoPP::byte digest[CryptoPP::SHA1::DIGESTSIZE];
    CryptoPP::SHA1().CalculateDigest(digest, data.data(), data.size());
    return std::equal(hash.begin(), hash.end(), std::begin(digest), std::end(digest));
}

FileData::FileData(size_t pSize, size_t fSize, std::string hashes, std::string fileName) : 
    pieceSize(pSize), output(fileName, std::ios::binary)
{
    numPieces = fSize / pSize + (fSize % pSize != 0);
    lastPieceSize = fSize % pSize == 0 ? pSize : fSize - ((numPieces - 1) * pSize);

    for (size_t i = 0; i < numPieces; i++) {
        // convert pieces str to a format that can be checked against individual piece hashes later
        std::string hashStr = hashes.substr(i * 20, 20);
        std::array<uint8_t, 20> hashArr;
        std::copy_n(hashes.begin() + (i * 20), 20, hashArr.begin());
        pieceHashes.push_back(hashArr);

        undownloaded.push_back(i);         // randomize piece download order
        pieceInfo.push_back(false);
    }

    std::random_device dev;
    std::mt19937 rng(dev());
    std::shuffle(undownloaded.begin(), undownloaded.end(), rng);


    
}

bool FileData::finished()
{
    for (size_t i = 0; i < pieceInfo.size(); i++) {
        if (!pieceInfo[i]) { return false; }
    }
    return true;
}

PeerWireClient::PeerWireClient(asio::io_context &ioc, std::string addrStr, size_t peerNum, 
    std::string hs, FileData& fData) :
    ioContext(ioc), socket(ioc), acceptor(ioc), timer(ioc), peerNumber(peerNum), fileData(fData),
    handshake(hs)
{

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
    bitfLength = fileData.numPieces/8 + (fileData.numPieces % 8 != 0);
    startConnection();
}

// host mode
PeerWireClient::PeerWireClient(asio::io_context &ioc, std::string hs, FileData& fData, 
asio::ip::port_type port) :
    ioContext(ioc), socket(ioc), acceptor(ioc, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
    timer(ioc), fileData(fData), handshake(hs)
{

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

void PeerWireClient::startConnection() {
    asio::ip::tcp::endpoint endpoint(addr, port);
    auto callback = std::bind(&PeerWireClient::handleConnect, this, std::placeholders::_1);
    socket.async_connect(endpoint, callback);
}

// Perform the initial stuff required when connecting, e.g. the handshake
void PeerWireClient::handleConnect(const asio::error_code &error) {
    if (!error) {
        //std::cout << "Connected to peer " << peerNumber << " at " << addr.to_string() << ":" 
        //    << port << std::endl;
        auto callback = std::bind(&PeerWireClient::handleWrite, this, std::placeholders::_1,
            std::placeholders::_2);
        asio::async_write(socket, asio::buffer(handshake, HANDSHAKE_LENGTH), callback);
    } else {
        //std::cerr << "Connection error: " << error.message() << std::endl;
    }
}

/**
 * Handles async_write call by initiating another read.
*/
void PeerWireClient::handleWrite(const asio::error_code &error, size_t /*bytesWritten*/) {
    if (error) {
        //std::cerr << peerPrefix << "Write err: " << error.message() << std::endl;
        return;
    }

    if (!socket.is_open()) {
        return;
    }

    auto callback = std::bind(&PeerWireClient::handleRead, this, std::placeholders::_1, 
        std::placeholders::_2);
    socket.async_read_some(asio::buffer(tempBuffer), callback);
}

/**
 * Handles async_read_some call. Parses the message(s), writes the response to writeBuffer, and 
 * initates another async_write call if there is work to do.
*/
void PeerWireClient::handleRead(const asio::error_code &error, size_t bytesRead) {
    if (error) {
        //std::cerr << peerPrefix << "Read err: " << error.message() << std::endl;
        if (peerValidated && !fileData.pieceInfo[pieceData.pieceIndex]) {
            fileData.undownloaded.push_back(pieceData.pieceIndex);
        }
        return;
    }

    if (!socket.is_open()) {
        return;
    }

    if (bytesRead + readBufBytes > MSG_BUF_SIZE) {
        std::cout << peerPrefix << "Read buf overflow.\n";
        std::cout << "Bytes read " << bytesRead << std::endl;
        std::cout << "piece " << pieceData.pieceIndex << std::endl;
        
        abort();
    }

    std::copy_n(tempBuffer.begin(), bytesRead, readBuffer.begin() + readBufBytes);

    readBufBytes += bytesRead;

    if (!peerValidated) { 
        checkHandshake();
        // get the first piece
        size_t pIndex = fileData.undownloaded.front();
        fileData.undownloaded.pop_front();
        size_t pSize = pIndex == (fileData.numPieces - 1) ? 
            fileData.lastPieceSize : fileData.pieceSize;
        pieceData = PieceData(pIndex, pSize);
        timer.expires_from_now(std::chrono::seconds(TIME_LIMIT));
        auto callback = std::bind(&PeerWireClient::handleTimeout, this, std::placeholders::_1);
        timer.async_wait(callback);
    }

    // process all incoming messages, updating the current piece if possible
    std::vector<PeerWireMessage> inboundMessageList = generateMessageList();
    for (PeerWireMessage msg : inboundMessageList) {
        processMessage(msg);
    }

    if (pieceData.finished() && pieceData.inProgress) {
        processFinishedPiece();
    }

    if (fileData.finished()) {
        ioContext.stop();
        fileData.output.close();
        return;
    }

    std::vector<PeerWireMessage> outboundMessageList;

    if (peerChoking) { 
        outboundMessageList.push_back(PeerWireMessage(1, MessageID::Interested, 
            std::vector<uint8_t>()));
    } else if (inboundMessageList.size() == 0 || !pieceData.inProgress) {
        // keep alive
        outboundMessageList.push_back(PeerWireMessage());
    } else {
        std::vector<uint32_t> requests = pieceData.getRequests();
        for (size_t blockInd : requests) {
            if (pieceData.timedOut) {
                std::cout << "Requesting piece " << pieceData.pieceIndex << " block " << blockInd << std::endl;
            }
            outboundMessageList.push_back(generateRequest(pieceData.pieceIndex, blockInd));
            //std::cout << peerPrefix << "Req piece " << pieceData.pieceIndex << " block: " << blockInd << std::endl;
        }
        if (outboundMessageList.empty()) {
            // keep alive
            outboundMessageList.push_back(PeerWireMessage());
        }
    }

    size_t bytesWritten = writeMessageListToBuffer(outboundMessageList);

    if (bytesWritten == 0) {
        auto callback = std::bind(&PeerWireClient::handleRead, this, std::placeholders::_1, 
            std::placeholders::_2);
        socket.async_read_some(asio::buffer(tempBuffer), callback);
    } else {
        auto callback = std::bind(&PeerWireClient::handleWrite, this, std::placeholders::_1,
            std::placeholders::_2);
        asio::async_write(socket, asio::buffer(writeBuffer, bytesWritten), callback);
    }
}

void PeerWireClient::handleTimeout(const asio::error_code &error)
{
    if (error == asio::error::operation_aborted) {
        return;
    }

    if (!fileData.pieceInfo[pieceData.pieceIndex]) {
        fileData.undownloaded.push_back(pieceData.pieceIndex);
        dropConnection();
    }
}

/**
 * Process a peer wire protocol message
*/
void PeerWireClient::processMessage(PeerWireMessage msg) {
    if (msg.lengthPrefix == 0) {
        return;
    }

    switch (msg.id) {
        case (MessageID::Choke):
            peerChoking = true;
            break;
        case (MessageID::Unchoke):
            peerChoking = false;
            break;
        case (MessageID::Interested):
            peerInterested = true;
            break;
        case (MessageID::NotInterested):
            peerInterested = false;
            break;
        case (MessageID::Have):
        {
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
            break;
        case (MessageID::Piece):
            processPieceMessage(msg);
            break;
        case (MessageID::Cancel):
            std::cout << "Cancel\n";
            msg.printReadable();
            break;
        default:
            std::cout << "Unknown message type\n";
            msg.printReadable();
    }
}

void PeerWireClient::updatePiece()
{
    if (fileData.undownloaded.empty()) {
        pieceData.inProgress = false;
        return;
    }
    pieceData.inProgress = true;

    uint32_t newPiece = fileData.undownloaded.front();
    fileData.undownloaded.pop_front();

    // attempt to guarantee that the peer has the piece
    for (size_t i = 0; i < 5; i++) {
        if (!bitfield.hasPiece(newPiece)) {
            fileData.undownloaded.push_back(newPiece);
            newPiece = fileData.undownloaded.front();
            fileData.undownloaded.pop_front();
        }
    }
    
    size_t pSize = newPiece == (fileData.numPieces - 1) ? 
        fileData.lastPieceSize : fileData.pieceSize;
    pieceData = PieceData(newPiece, pSize);
    timer.expires_from_now(std::chrono::seconds(TIME_LIMIT));
    auto callback = std::bind(&PeerWireClient::handleTimeout, this, std::placeholders::_1);
    timer.async_wait(callback);
}

void PeerWireClient::processFinishedPiece()
{
    // verify hash
    std::array<uint8_t, 20> metainfoHash = fileData.pieceHashes[pieceData.pieceIndex];
    if (!pieceData.verifyHash(metainfoHash)) {
        std::cerr << "Unable to verify hash: piece " << pieceData.pieceIndex << std::endl;
        fileData.undownloaded.push_back(pieceData.pieceIndex);
        updatePiece();
        return;
    }

    // write piece to file
    size_t fileOffset = pieceData.pieceIndex * pieceData.pieceSize;
    fileData.output.seekp(fileOffset);
    const char *dataPtr = (const char *)pieceData.data.data();
    fileData.output.write(dataPtr, pieceData.data.size());
    fileData.output.flush();

    // update fileData
    fileData.pieceInfo[pieceData.pieceIndex] = true;

    // print download status
    size_t numDownloaded = 0;
    for (size_t i = 0; i < fileData.numPieces; i++) {
        if (fileData.pieceInfo[i]) { numDownloaded++; }
    }

    float progress = (float)numDownloaded / (float)fileData.numPieces;
    int barWidth = 70;

    std::cout << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
      if (i < pos)
        std::cout << "=";
      else if (i == pos)
        std::cout << ">";
      else
        std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();

    // get a new piece and start a new timer
    updatePiece();
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
std::vector<PeerWireMessage> PeerWireClient::generateMessageList() {
    size_t readIndex = 0;
    std::vector<PeerWireMessage> messageList;

    while (readIndex < readBufBytes) {
        // TODO: if breaking early, copy over the beginning of readBuffer with the remainder

        if (readIndex + 4 > readBufBytes) {
            // shift left
            std::copy_n(readBuffer.begin() + readIndex, readBufBytes, readBuffer.begin());
            break;
        }

        uint32_t lengthPrefix = ((uint32_t)(uint8_t)(readBuffer[readIndex]) << 24) |
            ((uint32_t)(uint8_t)(readBuffer[readIndex + 1]) << 16) |
            ((uint32_t)(uint8_t)(readBuffer[readIndex + 2]) << 8) |
            (uint32_t)(uint8_t)(readBuffer[readIndex + 3]);

        // keep alive
        if (lengthPrefix == 0) {
            messageList.push_back(PeerWireMessage());
            readIndex += 4;
            continue;
        }

        if (readIndex + lengthPrefix + 4 > readBufBytes) {      
            // shift left
            std::copy_n(readBuffer.begin() + readIndex, readBufBytes, readBuffer.begin());
            break;
        }

        // it is possible for id to be outside the range of defined codes, but we allow this
        MessageID id = (MessageID)(readBuffer[readIndex + 4]);

        std::vector<uint8_t> payload;
        if (lengthPrefix == 0) {
            std::cerr << "lp 0" << std::endl;
        }
        for (size_t i = 0; i < (lengthPrefix - 1); i++) {
            payload.push_back((uint8_t)(readBuffer[readIndex + 5 + i]));
        }

        messageList.push_back(PeerWireMessage(lengthPrefix, id, payload));
        readIndex += lengthPrefix + 4;
    }
    readBufBytes -= readIndex;
    
    return messageList;
}

PeerWireMessage PeerWireClient::generateRequest(size_t pieceIndex, size_t blockIndex) {
    // request: <len=0013><id=6><index><begin><length>
    std::vector<uint8_t> payload;
    
    // index
    payload.push_back((uint8_t)(pieceIndex >> 24));
    payload.push_back((uint8_t)(pieceIndex >> 16));
    payload.push_back((uint8_t)(pieceIndex >> 8));
    payload.push_back((uint8_t)pieceIndex);

    // begin
    payload.push_back((uint8_t)(blockIndex >> 24));
    payload.push_back((uint8_t)(blockIndex >> 16));
    payload.push_back((uint8_t)(blockIndex >> 8));
    payload.push_back((uint8_t)blockIndex);

    size_t blockLength = BLOCK_SIZE;
    if (pieceIndex == fileData.numPieces - 1) {
        if (fileData.lastPieceSize - blockIndex < BLOCK_SIZE) {
            blockLength = fileData.lastPieceSize - blockIndex;
        }
    }

    // length
    payload.push_back((uint8_t)(blockLength >> 24));
    payload.push_back((uint8_t)(blockLength >> 16));
    payload.push_back((uint8_t)(blockLength >> 8));
    payload.push_back((uint8_t)blockLength);

    PeerWireMessage requestMsg = PeerWireMessage(13, MessageID::Request, payload);

    return requestMsg;
}

// Processes a Piece message, updating the pieceData object
void PeerWireClient::processPieceMessage(PeerWireMessage msg) {
    // piece: <len=0009+X><id=7><index><begin><block>
    // get piece and block indices
    uint32_t pieceIndex = (uint32_t)(msg.payload[0]) << 24 |
        (uint32_t)(msg.payload[1]) << 16 |
        (uint32_t)(msg.payload[2]) << 8 |
        (uint32_t)(msg.payload[3]);
    uint32_t blockIndex = (uint32_t)(msg.payload[4]) << 24 |
        (uint32_t)(msg.payload[5]) << 16 |
        (uint32_t)(msg.payload[6]) << 8 |
        (uint32_t)(msg.payload[7]);

    if (pieceIndex != pieceData.pieceIndex || blockIndex % BLOCK_SIZE != 0) { 
        std::cerr << peerPrefix << "Incorrect piece: " << pieceIndex << std::endl;
        std::cerr << peerPrefix << "Needed piece: " << pieceData.pieceIndex << std::endl;
        return; 
    }
    if (pieceData.blockInfo[blockIndex / BLOCK_SIZE]) { 
        std::cerr << peerPrefix << "Already have block: " << pieceIndex << ": " << blockIndex << std::endl;
        return;
    }

    std::copy(msg.payload.begin() + 8, msg.payload.end(), pieceData.data.begin() + blockIndex);
    pieceData.blockInfo[blockIndex / BLOCK_SIZE] = true;
}

// Check that the handshake is valid and close the connection if it's not. Remove the handshake
// from the readBuffer
void PeerWireClient::checkHandshake() {
    std::string infoHash = handshake.substr(28, 20);

    uint8_t pStrLen = readBuffer[0];
    std::string pStr = "";
    for (size_t i = 0; i < pStrLen; i++) {
        pStr += readBuffer[i+1];
    }
    // this is where you would handle reserved bits if they become used
    peerID = "";
    std::string peerInfoHash = "";
    for (size_t i = 0; i < 20; i++) {
        peerInfoHash += readBuffer[i+pStrLen+9];
        peerID += readBuffer[i+pStrLen+29];
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
    readBufBytes -= HANDSHAKE_LENGTH;
    std::copy_n(readBuffer.begin() + HANDSHAKE_LENGTH, readBufBytes, readBuffer.begin());
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