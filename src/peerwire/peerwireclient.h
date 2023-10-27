#include <iostream>
#include "external/asio-1.28.0/include/asio.hpp"
#include <deque>
#define MSG_BUF_SIZE 1024
#define BLOCK_SIZE 16384
#define HANDSHAKE_LENGTH 68

class PieceQueue {
public:
    size_t numPieces;
    size_t pieceSize;
    std::deque<uint32_t> undownloaded;

    PieceQueue() = default;
    PieceQueue(size_t num, size_t size);
};

enum MessageID : uint8_t {
    Choke = 0,
    Unchoke = 1,
    Interested = 2,
    NotInterested = 3,
    Have = 4,
    Bitfield = 5,
    Request = 6,
    Piece = 7,
    Cancel = 8,
    Port = 9
};

class PeerWireMessage {
public:
    uint32_t lengthPrefix;
    MessageID id;
    std::vector<uint8_t> payload;
    PeerWireMessage();
    PeerWireMessage(uint32_t lengthPref, MessageID mID, std::vector<uint8_t> pl);

    void printReadable();
};

class BitfieldStorage {
public:
    std::vector<uint8_t> data;
    BitfieldStorage() = default;
    BitfieldStorage(std::vector<uint8_t> bitf);

    bool hasPiece(uint32_t pieceIndex);
    void setPiece(uint32_t pieceIndex);
};

class PeerWireClient {
public:
    PeerWireClient(asio::io_context& ioc, std::string addrStr, size_t peerNum, std::string hs,
        PieceQueue& pq);
    PeerWireClient(asio::io_context &ioc, std::string hs, PieceQueue& pq, asio::ip::port_type port);



private:
    // network info
    asio::ip::address_v4 addr;
    asio::ip::port_type port;
    asio::io_context& ioContext;
    asio::ip::tcp::socket socket;
    asio::ip::tcp::acceptor acceptor;
    std::array<char, MSG_BUF_SIZE> messageBuffer;
    std::array<char, MSG_BUF_SIZE> writeBuffer;

    // peer info
    size_t peerNumber;
    std::string peerPrefix;
    std::string peerID;
    bool peerValidated;
    bool amChoking = true;
    bool amInterested = false;
    bool peerChoking = true;
    bool peerInterested = false;

    // file info
    std::string infoHash;
    std::string handshake;
    size_t bitfLength;
    BitfieldStorage bitfield;
    PieceQueue& pieceQueue;
    uint32_t currentPiece;
    uint32_t currentBlock;

    void startConnection();
    void handleAccept(const asio::error_code &error);
    void handleConnect(const asio::error_code &error);
    void handleWrite(const asio::error_code &error, size_t bytesWritten);
    void handleRead(const asio::error_code &error, size_t bytesRead);
    void checkHandshake();
    void dropConnection();
    std::vector<PeerWireMessage> generateMessageList(size_t bytesRead);
    PeerWireMessage generateNextRequest();
    size_t writeMessageListToBuffer(std::vector<PeerWireMessage> messageList);
    void processMessage(PeerWireMessage msg);
};

