#include "peerwireclient.h"
#include "external/asio-1.28.0/include/asio.hpp"
#include <functional>

PeerWireClient::PeerWireClient(asio::io_context &ioc, std::string addrStr, std::string hs) : 
    ioContext(ioc), socket(ioc), handshake(hs) {
    // Convert the address and port from raw binary string to usable types
    std::string ipStr = addrStr.substr(0, 4);
    std::array<unsigned char, 4> rawIP;
    std::copy(std::begin(ipStr), std::end(ipStr), std::begin(rawIP));
    addr = asio::ip::make_address_v4(rawIP);
    std::string portStr = addrStr.substr(4, 2);
    port = (asio::ip::port_type)portStr[0] << 8 | 
        (asio::ip::port_type)portStr[1];
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
    if (error) {
        std::cerr << error.message() << std::endl;
    } else {
        std::cout << "Connected to peer at " << addr.to_string() << ":" << port << std::endl;
        auto callback = std::bind(&PeerWireClient::handleWrite, this, std::placeholders::_1,
            std::placeholders::_2);
        socket.async_write_some(asio::buffer(handshake), callback);
    }
}


void PeerWireClient::handleWrite(const asio::error_code &error, size_t bytesWritten) {
    if (error) {
        std::cerr << error.message() << std::endl;
    } else {
        std::cout << "Handshake sent with " << bytesWritten << " bytes written to peer\n";
        auto callback = std::bind(&PeerWireClient::handleRead, this, std::placeholders::_1, 
            std::placeholders::_2);
        socket.async_read_some(asio::buffer(messageBuffer), callback);
    }

}

void PeerWireClient::handleRead(const asio::error_code &error, size_t bytesRead) {
    if (error) {
        std::cerr << error.message() << std::endl;
    } else {
        std::cout << "Response of length " << bytesRead << " received from peer\n";
        std::cout.write(messageBuffer.data(), bytesRead);
    }
}

// TODO: double check address and port are formatted correctly
// TODO: check method signatures of callback functions
// TODO: although I don't think this would be the case, the BitTorrent protocol ver. 1 we are using
// may not be supported by other clients