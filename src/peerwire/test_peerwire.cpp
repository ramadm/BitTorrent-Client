#include "peerwireclient.h"

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

int main(int argc, char **argv) {
    // TODO: establish a localhost connection between 2 peerwire clients
    // TODO: test sending some messages back and forth
    asio::io_context io;
    asio::ip::port_type port = 6881;
    std::string infoHash = "01234567890123456789";
    std::string pID = "-rd0001-012345678900";
    std::string handshake = generateHandshake(infoHash, pID);
    PieceQueue pieceQ(1, 100);
    PeerWireClient *p;

    if (argc != 1 && std::string(argv[1]) == "host") {
        p = new PeerWireClient(io, handshake, pieceQ, port);
        std::cout << "Hosting...\n";
    } else {
        // address -> raw bytes conversion
        auto byteAddr = asio::ip::address_v4::loopback().to_bytes();
        std::string addrStr = "";
        for (size_t i = 0; i < byteAddr.size(); i++) {
            addrStr += byteAddr[i];
        }

        uint8_t portByte1 = ((port & 0xFF00) >> 8);
        uint8_t portByte2 = (port & 0x00FF);
        addrStr += (char)portByte1;
        addrStr += (char)portByte2;
        p = new PeerWireClient(io, addrStr, 1, handshake, pieceQ);
        std::cout << "Running as client...\n";
    }

    io.run();
}
