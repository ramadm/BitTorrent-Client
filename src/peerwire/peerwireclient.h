#include <iostream>
#include <fstream>
#include "external/asio-1.28.0/include/asio.hpp"
#include <deque>
#define MSG_BUF_SIZE 1048576
#define BLOCK_SIZE 16384
#define HANDSHAKE_LENGTH 68
#define QUEUE_LENGTH 5
#define TIME_LIMIT 15

class PieceData {
public:
    uint32_t pieceIndex;
    uint32_t pieceSize;

    // blockInfo acts like an uncompressed bitfield for blocks, i.e. a downloaded block has value 1,
    // a needed block 0
    std::vector<bool> blockInfo;
    // need acts as a queue for pieces that need to be requested. Once empty the queue can be
    // regenerated from blockInfo
    std::deque<uint32_t> need;
    // the actual piece data
    std::vector<uint8_t> data;
    unsigned int requeues = 0;

    PieceData() = default;
    PieceData(uint32_t pieceIndex, uint32_t pieceSize);
    bool finished();
    std::vector<uint32_t> getRequests();
    void requeueBlocks();
    bool verifyHash(std::array<uint8_t, 20> hash);

    // TODO
    // verifyHash?
    // writeToFile?
    // bool finished()
};

class FileData {
public:
    size_t numPieces;
    size_t pieceSize;
    std::vector<bool> pieceInfo;
    std::vector<std::array<uint8_t, 20>> pieceHashes;
    std::deque<uint32_t> undownloaded;
    std::ofstream output;

    FileData() = default;
    FileData(size_t num, size_t size, std::string hashes, std::string fileName);
    //void writeToFile(std::vector<uint8_t> data, size_t pos);
};

/*class PieceQueue {
public:
    size_t numPieces;
    size_t pieceSize;
    std::deque<uint32_t> undownloaded;
    std::string pieces;

    PieceQueue() = default;
    PieceQueue(size_t num, size_t size, std::string pieceStr);
};*/

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
        FileData& fData);
    PeerWireClient(asio::io_context &ioc, std::string hs, FileData& fData, asio::ip::port_type port);



private:
    // network info
    asio::ip::address_v4 addr;
    asio::ip::port_type port;
    asio::io_context& ioContext;
    asio::ip::tcp::socket socket;
    asio::ip::tcp::acceptor acceptor;
    // TODO: timeout piece
    asio::steady_timer timer;
    // buffers
    std::array<char, MSG_BUF_SIZE> tempBuffer;
    std::array<char, MSG_BUF_SIZE> readBuffer;
    size_t readBufBytes = 0;
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
    FileData& fileData;
    std::string infoHash;
    std::string handshake;
    size_t bitfLength;
    BitfieldStorage bitfield;

    PieceData pieceData;


    void startConnection();
    void handleAccept(const asio::error_code &error);
    void handleConnect(const asio::error_code &error);
    void handleWrite(const asio::error_code &error, size_t bytesWritten);
    void handleRead(const asio::error_code &error, size_t bytesRead);
    void checkHandshake();
    void dropConnection();
    std::vector<PeerWireMessage> generateMessageList();
    PeerWireMessage generateRequest(size_t pieceIndex, size_t blockIndex);
    void processPieceMessage(PeerWireMessage msg);
    size_t writeMessageListToBuffer(std::vector<PeerWireMessage> messageList);
    void processMessage(PeerWireMessage msg);
};

