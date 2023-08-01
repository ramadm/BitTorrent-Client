#include "torrent.h"
#include <sstream>
#include <iostream>
#include "external/asio-1.28.0/include/asio.hpp"

Torrent::Torrent(string trackerResponse) {
    std::istringstream ss(trackerResponse);
    Bencoding *benc = parse(ss);
    benc->verifyType(BencDict);
    for (size_t i = 0; i < benc->dictData.size(); i++) {
        string key = benc->dictData[i].first;
        Bencoding *value = benc->dictData[i].second;
        if (key == "complete") {
            value->verifyType(BencInt);
            numSeeders = value->intData;
        } else if (key == "incomplete") {
            value->verifyType(BencInt);
            numLeechers = value->intData;
        } else if (key == "interval") {
            value->verifyType(BencInt);
            interval = value->intData;
        } else if (key == "min interval") {
            value->verifyType(BencInt);
            minAnnounceInterval = value->intData;
        } else if (key == "peers") {
            if (value->type == BencStr) {
                for (size_t i = 0; i < value->strData.length(); i+=6) {
                    peerList.push_back(value->strData.substr(i, 6));
                }
            } else {
                std::cout << "Peer list is not in binary format.\n";
            }
        }
    }
}

/**
 * Connects to the peer at the specified address to begin downloading or uploading
 * @param binAddr a 6-byte string containing the 4-byte IP and 2-byte port of the peer
*/
void Torrent::connectToPeer(string binAddr) {
    using asio::ip::port_type;

    // Init. an ASIO address for use in the endpoint
    string binAddrIP = binAddr.substr(0, 4);
    // Our string is raw binary, which luckily ASIO supports for address construction
    std::array<unsigned char, 4> rawIP;
    std::copy(std::begin(binAddrIP), std::end(binAddrIP), std::begin(rawIP));
    asio::ip::address_v4 addr(rawIP);

    // Convert our raw binary port to a usable ASIO-style port number
    string binAddrPort = binAddr.substr(4, 2);
    const char *cStrPort = binAddrPort.c_str();
    port_type port = (port_type)cStrPort[0] << 8 | (port_type)cStrPort[1];

    

    // Open a TCP connection
    asio::io_context ioContext;
    asio::ip::tcp::endpoint endpoint(addr, port);
    asio::ip::tcp::socket socket(ioContext, endpoint.protocol());
    socket.connect(endpoint);
    std::cout << "Connected to peer at " << addr.to_string() << ":" << port << std::endl;

    // Self and peer statuses w.r.t. whether they are ready to send/receive data
    /*
    unsigned char amChoking = 1;
    unsigned char amInterested = 0;
    unsigned char peerChoking = 1;
    unsigned char peerInterested = 0;

    // Every connection begins with the handshake
    string protocol = "BitTorrent Protocol";
    unsigned char pStrLen = protocol.length();
    */

    // TODO: send handshake
    // TODO: the connect call should be async I think?
    for (;;) {
        std::array<char, 128> buf;
        asio::error_code error;

        size_t readLen = socket.read_some(asio::buffer(buf), error);
        if (error = asio::error::eof) {
            break; // Connection closed cleanly by peer
        } else if (error) {
            throw asio::system_error(error);
        }

        std::cout.write(buf.data(), readLen);
    }
}

// This function makes asyncrhonous calls to connectToPeer(), attempting to open a connection with
// as many peers as possible up to a maximum of 30 connections
void Torrent::startDownloading() {
    std::cout << "Seeders: " << numSeeders << std::endl;
    std::cout << "Leechers: " << numLeechers << std::endl;
    std::cout << "interval: " << interval << std::endl;
    std::cout << "Number of peers: " << peerList.size() << std::endl;
    /*for (size_t i = 0; i < peerList.size(); i++) {
        // TODO: connect to peer i
        string addr = peerList[i];
        std::cout << addr << std::endl;
    }*/
    if (peerList.size() == 0) {
        std::cout << "Error: no peers.\n";
        abort();
    } else {
        connectToPeer(peerList[0]);
    }
}