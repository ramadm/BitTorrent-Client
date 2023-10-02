#include "peerwireclient.h"
#include "external/asio-1.28.0/include/asio.hpp"
#include <functional>

PeerWireClient::PeerWireClient(asio::io_context &ioc, std::string addrStr, std::string hs, 
    size_t peerNum, std::string ih, size_t nPieces, std::deque<uint32_t>& pieceQ) : 
    peerNumber(peerNum), numPieces(nPieces), infoHash(ih), handshake(hs), ioContext(ioc), socket(ioc), pieceQueue(pieceQ) {
    // Convert the address and port from raw binary string to usable types
    std::string ipStr = addrStr.substr(0, 4);
    std::array<uint8_t, 4> rawIP;
    std::copy(std::begin(ipStr), std::end(ipStr), std::begin(rawIP));
    addr = asio::ip::make_address_v4(rawIP);
    std::string portStr = addrStr.substr(4, 2);
    port = (asio::ip::port_type)portStr[0] << 8 | 
        (asio::ip::port_type)portStr[1];
    peerPrefix = "[Peer " + std::to_string(peerNumber) + "] ";
    peerValidated = false;
    startConnection();
}

// TODO: how should we handle refused connections?
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
        socket.async_write_some(asio::buffer(handshake), callback);
    }
    // TODO: connections are frequently refused. This might be normal but I need to look into it.
    // I don't print the connection errors because it tends to flood the terminal
}


void PeerWireClient::handleWrite(const asio::error_code &error, size_t /*bytesWritten*/) {
    if (error) {
        std::cerr << peerPrefix << "Write err: " << error.message() << std::endl;
    } else {
        auto callback = std::bind(&PeerWireClient::handleRead, this, std::placeholders::_1, 
            std::placeholders::_2);
        socket.async_read_some(asio::buffer(messageBuffer), callback);
    }

}

void PeerWireClient::handleRead(const asio::error_code &error, size_t bytesRead) {
    if (error) {
        std::cerr << peerPrefix << "Read err: " << error.message() << std::endl;
        return;
    }
    
    if (!socket.is_open()) {
        return;
    }

    std::cout << peerNumber << ": Received response: " << bytesRead << " bytes\n";

    size_t readIndex = 0;
    while (readIndex < bytesRead) {
        if (!peerValidated) {
            checkHandshake();
            std::cout << "Handshake validated\n";
            readIndex += HANDSHAKE_LENGTH;
        }

        uint32_t lengthPrefix = 0;
        for (size_t i = 0; i < 4; i++) {
            uint8_t lenByte = (uint8_t)(messageBuffer[readIndex + i]);
            lengthPrefix |= lenByte << (24 - (8 * i));
        }
        std::cout << "Length prefix: " << lengthPrefix << std::endl;

        if (lengthPrefix == 0) {
            std::cout << "keep alive\n";
            return;
            // TODO: process keep alive
        }
    
        // Process message by ID
        uint8_t messageID = messageBuffer[readIndex + 4];
        switch (messageID) {
            case (MessageID::Choke):
                std::cout << "Choke\n";
                abort();
            case (MessageID::Unchoke):
                std::cout << "Unchoke\n";
                abort();
            case (MessageID::Interested):
                std::cout << "Interested\n";
                abort();
            case (MessageID::NotInterested):
                std::cout << "Not Interested\n";
                abort();
            case (MessageID::Have):
                std::cout << "Have\n";
                abort();
            case (MessageID::Bitfield):
            {
                processBitfield(lengthPrefix, readIndex);
                break;
            }
            case (MessageID::Request):
                std::cout << "Request\n";
                abort();
            case (MessageID::Piece):
                std::cout << "Piece\n";
                abort();
            case (MessageID::Cancel):
                std::cout << "Cancel\n";
                abort();
            case (MessageID::Port):
                std::cout << "Port";
                abort();
            default:
                // TODO: skip the message
                abort();
        }
        // double check this number
        readIndex += lengthPrefix + 4;
    } 

    // TODO: how to decide when we should start sending messages?  
    // TODO: maybe we should do an incoming and outgoing message queue type thing
    // then as we cycle through the peer connections, if there's work to do, we just
    // pop e.g. 5 messages from each queue before returning?
}

/* Process the bitfield message and store the bitfield if it seems to be valid */
void PeerWireClient::processBitfield(size_t lengthPrefix, size_t readIndex) {
    std::cout << "Bitfield\n";
    size_t bitfLength = numPieces/8 + (numPieces % 8 != 0);
    if (bitfLength != (lengthPrefix - 1)) {
        std::cout << "Bitfield length mismatch.\n";
        std::cout << "    Calculated: " << bitfLength << std::endl;
        std::cout << "    Peer: " << (lengthPrefix-1) << std::endl;
        dropConnection();
        return;
    }
    std::cout << "Bitfield length: " << bitfLength << std::endl;
    for (size_t i = 0; i < bitfLength; i++) {
        bitfield += messageBuffer[readIndex + 4 + i];
    }
}

// TODO: test length prefix
void PeerWireClient::sendMessage(uint32_t len, uint8_t id) {
    for (size_t i = 0; i < 4; i++) {
        uint8_t lenByte = (uint8_t)(len << (4*i));
        messageBuffer[i] = (char)lenByte;
    }
    messageBuffer[4] = (char)id;
    auto callback = std::bind(&PeerWireClient::handleWrite, this, std::placeholders::_1,
        std::placeholders::_2);
    socket.async_write_some(asio::buffer(messageBuffer), callback);
}

// TODO: this could stall pretty bad if we end up connecting to a peer without a lot of pieces
// is there a better way to do this?
// maybe measure bitfield density?
void PeerWireClient::getNextPiece() {
    if (pieceQueue.empty()) {
        // TODO: cleanup
    }

    uint32_t piece = pieceQueue.front();
    pieceQueue.pop_front();
    if (hasPiece(piece)) {
        currentPiece = piece;
    } else {
        pieceQueue.push_back(piece);
    }    
}

// TODO: edit to adjust for readIndex
void PeerWireClient::requestBlock() {
    // request: <len=0013><id=6><index><begin><length>
    uint32_t len = 13;
    for (size_t i = 0; i < 4; i++) {
        messageBuffer[i] = (char)(len << (4*i));
        messageBuffer[5+i] = (char)(currentPiece << (4*i));
        messageBuffer[9+i] = (char)(currentBlock << (4*i));
        messageBuffer[13+i] = (char)(BLOCK_SIZE << (4*i));
    }
    uint8_t id = 6;
    messageBuffer[4] = (char)id;


    // send a request off and call async_write_some with the handleWrite callback
    auto callback = std::bind(&PeerWireClient::handleWrite, this, std::placeholders::_1,
        std::placeholders::_2);
    socket.async_write_some(asio::buffer(messageBuffer), callback);

    // TODO: if you start getting some really weird errors try using a separate read/write buffer
    // As I understand the buffer shouldn't get overwritten in the middle of a read/write but
    // I'm not 100% sure

    // P.S., I need to read up on how the pipelining is supposed to work. I think maybe we should be
    // sending off multiple async_writes at a time, and similar with async_reads, keeping them in a 
    // queue, the idea being there's always messages on the line in both directions
}

// TODO: test this!
bool PeerWireClient::hasPiece(uint32_t index) {
    uint8_t bitfVal = (bitfield[index/8] >> (index % 8) & 0x01);
    return (bitfVal == 0x01);
}

// Check that the handshake is valid and close the connection if it's not
void PeerWireClient::checkHandshake() {
    uint8_t pStrLen = messageBuffer[0];
    std::string pStr = "";
    for (size_t i = 0; i < pStrLen; i++) {
        pStr += messageBuffer[i+1];
    }
    // TODO: handle reserved bits
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