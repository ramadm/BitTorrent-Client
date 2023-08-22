#include <iostream>
#include "external/asio-1.28.0/include/asio.hpp"
#define MSG_BUF_SIZE 1024

enum MessageID : unsigned char {
    Choke = 0,
    Unchoke = 1,
    Interested = 2,
    NotInterested = 3,
    Have = 4,
    Bitfield = 5,
    Request = 6,
    Piece = 7,
    Cancel = 8,
    Port = 9 // may not be used frequently
};

class PeerWireClient {
public:
    PeerWireClient(asio::io_context& ioc, std::string addrStr, std::string hs, size_t peerNum, 
        std::string ih);

private:
    size_t peerNumber;
    std::string peerPrefix;
    std::string peerID;
    std::string infoHash;
    std::string handshake;
    asio::ip::address_v4 addr;
    asio::ip::port_type port;
    asio::io_context& ioContext;
    asio::ip::tcp::socket socket;
    std::array<char, MSG_BUF_SIZE> messageBuffer;
    bool peerValidated;
    void startConnection();
    void handleConnect(const asio::error_code &error);
    void handleWrite(const asio::error_code &error, size_t bytesWritten);
    void handleFirstRead(const asio::error_code &error, size_t bytesRead);
    void handleRead(const asio::error_code &error, size_t bytesRead);
    void checkHandshake(size_t &bytesRead);
};