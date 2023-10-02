#include <iostream>
#include "external/asio-1.28.0/include/asio.hpp"
#include <deque>
#define MSG_BUF_SIZE 1024
#define BLOCK_SIZE 16384
#define HANDSHAKE_LENGTH 68

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
    Port = 9 // may not be used frequently
};

// man this class has a lot of contructor args, idk if this is a good thing
class PeerWireClient {
public:
    PeerWireClient(asio::io_context& ioc, std::string addrStr, std::string hs, size_t peerNum, 
        std::string ih, size_t nPieces, std::deque<uint32_t>& pieceQ);

private:
    size_t peerNumber;
    size_t numPieces;
    std::string peerPrefix;
    std::string peerID;
    std::string infoHash;
    std::string handshake;

    std::string bitfield;
    

    // TODO: create a std:deque (or other DS) of pieces and use std:shuffle on it to create a random
    // piece download order. When we want to allocate pieces to a peerwireclient, we pop those off
    // the deque and hold them in a vector or similar here in the peerwireclient class. We can check
    // against the bitfield to verify this peer has those pieces and push them back on the deque
    // during the assignment, or when this connection closes out as part of its cleanup. If the
    // piece gets successfully downloaded we can just remove it from our storage here without
    // re-inserting it
    // Note that deque isn't thread-safe, so if your clients might access the shared deque at the 
    // same time you need to have concurrency management.
    
    // It's possible the connection gets dropped while the piece is partially downloaded. In this
    // case the piece will stay marked as undownloaded, and we will just overwrite the same data
    // again. There are ways around this if it turns out that this is very expensive.
    std::deque<uint32_t> pieceQueue;
    // TODO: format this with actual requests
    std::deque<uint32_t> requestQueue;

    uint32_t currentPiece;
    uint32_t currentBlock;
    // of the current piece


    asio::ip::address_v4 addr;
    asio::ip::port_type port;
    asio::io_context& ioContext;
    asio::ip::tcp::socket socket;
    std::array<char, MSG_BUF_SIZE> messageBuffer;

    bool peerValidated;
    void startConnection();
    void handleConnect(const asio::error_code &error);
    void handleWrite(const asio::error_code &error, size_t bytesWritten);
    void handleRead(const asio::error_code &error, size_t bytesRead);
    void requestBlock();
    void getNextPiece();
    void checkHandshake();
    bool hasPiece(uint32_t index);
    void processBitfield(size_t lengthPrefix, size_t readIndex);
    void sendMessage(uint32_t len, uint8_t id);
    void dropConnection();
};