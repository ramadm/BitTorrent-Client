#include "peerwireclient.h"

int main(int argc, char **argv) {
    // TODO: establish a localhost connection between 2 peerwire clients
    // TODO: test sending some messages back and forth
    asio::io_context io;
    std::string addrStr = asio::ip::address_v4::loopback().to_string();

    // Generate the handshake
    std::string infoHash = "01234567890123456789";
    std::string pID0 = "-rd0001-012345678900";
    std::string pID1 = "-rd0001-012345678901";
    std::string handshake0 = generateHandshake(infoHash, pID0);
    std::string handshake1 = generateHandshake(infoHash, pID1);

    PieceQueue pieceQ(1, 100);

    PeerWireClient host(io, addrStr, 0, handshake0, pieceQ);
    PeerWireClient peer(io, addrStr, 1, handshake1, pieceQ);

    io.run();
}

std::string generateHandshake(std::string infoHash, std::string peerID) {
    std::string handshake = "";
    std::string protocol = "BitTorrent protocol";
    uint8_t pStrLen = protocol.length();
    handshake += (char)pStrLen;
    handshake += protocol;
    uint8_t zero = 0;
    for (size_t i = 0; i < 8; i++) {
        handshake += (char)zero;
    }
    handshake += infoHash + peerID;
    return handshake;
}