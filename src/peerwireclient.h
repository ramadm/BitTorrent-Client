#include <iostream>
#include "external/asio-1.28.0/include/asio.hpp"

class PeerWireClient {
public:
    PeerWireClient(asio::io_context& ioc, std::string addrStr, std::string hs);

private:
    // TODO: copying this from boost's example, but I believe ioContext is a reference variable
    // because a single io_context object is maintained between all connections
    std::string handshake;
    asio::ip::address_v4 addr;
    asio::ip::port_type port;
    asio::io_context& ioContext;
    asio::ip::tcp::socket socket;
    std::array<char, 128> messageBuffer;
    void startConnection();
    void handleConnect(const asio::error_code &error);
    void handleWrite(const asio::error_code &error, size_t bytesWritten);
    void handleRead(const asio::error_code &error, size_t bytesRead);
};