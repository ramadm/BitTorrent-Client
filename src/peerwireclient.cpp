#include "peerwireclient.h"
#include "external/asio-1.28.0/include/asio.hpp"
#include <functional>

PeerWireClient::PeerWireClient(asio::io_context &ioc, std::string addrStr, std::string hs, 
    size_t peerNum, std::string ih) : 
    peerNumber(peerNum), infoHash(ih), handshake(hs), ioContext(ioc), socket(ioc) {
    // Convert the address and port from raw binary string to usable types
    std::string ipStr = addrStr.substr(0, 4);
    std::array<unsigned char, 4> rawIP;
    std::copy(std::begin(ipStr), std::end(ipStr), std::begin(rawIP));
    addr = asio::ip::make_address_v4(rawIP);
    std::string portStr = addrStr.substr(4, 2);
    port = (asio::ip::port_type)portStr[0] << 8 | 
        (asio::ip::port_type)portStr[1];
    peerPrefix = "[Peer " + std::to_string(peerNumber) + "] ";
    startConnection();
}

// TODO: should we connect immediately and how to handle refused connections?
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

// Use messageBuffer to access the results of this read
void PeerWireClient::handleRead(const asio::error_code &error, size_t bytesRead) {
    if (error) {
        std::cerr << peerPrefix << "Read err: " << error.message() << std::endl;
    } else {
        std::cout << peerPrefix << "Received response (" << bytesRead << " bytes)\n";

        unsigned char pStrLen = messageBuffer[0];
        std::string pStr = "";
        for (size_t i = 0; i < pStrLen; i++) {
            pStr += messageBuffer[i+1];
        }
        // TODO: reserved bits
        peerID = "";
        std::string peerInfoHash = "";
        for (size_t i = 0; i < 20; i++) {
            peerInfoHash += messageBuffer[i+pStrLen+9];
            peerID += messageBuffer[i+pStrLen+29];
        }
        
        if (infoHash != peerInfoHash) {
            std::cout << peerPrefix << "Incorrect info hash!\n";
            // TODO: drop connection
            abort();
        }
        if (pStr != "BitTorrent protocol") {
            std::cout << peerPrefix << "Unrecognized protocol version: " << pStr << std::endl;
            // TODO: drop connection?
            abort();
        }
        
        std::string remainder = "";
        for (size_t i = pStrLen + 49; i < bytesRead; i++) {
            remainder += messageBuffer[i];
        }
        std::cout << peerPrefix << remainder << std::endl;
    }
}

// We receive handshake in response, followed by some more information I need to parse. Looks to be
// a ton of binary
// TODO: send choking/interested info over
// TODO: start downloading pieces!!