#include "torrent.h"
#include <sstream>
#include <iostream>
#include "peerwireclient.h"

Torrent::Torrent(string trackerResponse, string iHash, string pID) {
    infoHash = iHash;
    peerID = pID;
    // parse tracker response
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
    
    // Generate the handshake
    handshake = "";
    string protocol = "BitTorrent protocol";
    unsigned char pStrLen = protocol.length();
    handshake += (char)pStrLen;
    handshake += protocol;
    unsigned char zero = 0;
    for (size_t i = 0; i < 8; i++) {
        handshake += (char)zero;
    }
    handshake += infoHash + peerID;
    // delete once you are sure the handshake is well-formed.
    std::cout << "Handshake length " << handshake.length() << std::endl;
}

// This function makes asynchronous calls to connectToPeer(), attempting to open a connection with
// as many peers as possible up to a maximum of 30 connections
// TODO: add server functionality
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
    }

    // TODO: don't use raw pointers?
    asio::io_context io;
    vector<PeerWireClient *> clients;
    for (size_t i = 0; i < peerList.size(); i++) {
        clients.push_back(new PeerWireClient(io, peerList[i], handshake, i, infoHash));
    }
    io.run();

    // TODO: call destructors?
}