#include "torrent.h"
#include <sstream>
#include <iostream>
#include <random>
#include <cmath>
#include "peerwireclient.h"
#include "external/cryptopp/sha.h"
#include "external/cryptopp/hex.h"

Torrent::Torrent(Bencoding *minfo)
{
    metainfo = minfo;
    
    // initialize a peer ID
    std::random_device dev;
    std::mt19937 rng(dev());
    // random 12 character long sequence of numbers
    std::uniform_int_distribution<std::mt19937::result_type> dist12char(100000000000, 999999999999);
    peerID = "-rd0001-" + std::to_string(dist12char(rng));

    // Populate fields with data from metainfo file
    metainfo->verifyType(BencDict);
    for (size_t i = 0; i < metainfo->dictData.size(); i++) {
        string key = metainfo->dictData[i].first;
        Bencoding *value = metainfo->dictData[i].second;
        if (key == "announce") {
            value->verifyType(BencStr);
            announceURL = value->strData;
        } else if (key == "info") {
            // Get the info_hash param, which is a sha1 hash of the info dict from metainfo file
            string infoStr = bencode(value);
            size_t infoLen = infoStr.length();
            CryptoPP::byte digest[CryptoPP::SHA1::DIGESTSIZE];
            CryptoPP::SHA1().CalculateDigest(digest, (CryptoPP::byte *)infoStr.c_str(), infoLen);
            infoHash = std::string((char *)digest, 20);
            // iterate over the info dict to get fields
            value->verifyType(BencDict);
            for (size_t i = 0; i < value->dictData.size(); i++) {
                string infoKey = value->dictData[i].first;
                Bencoding *infoVal = value->dictData[i].second;
                if (infoKey == "length") {
                    infoVal->verifyType(BencInt);
                    length = infoVal->intData;
                } else if (infoKey == "piece length") {
                    infoVal->verifyType(BencInt);
                    pieceLength = infoVal->intData;
                } else if (infoKey == "name") {
                    infoVal->verifyType(BencStr);
                    fileName = infoVal->strData;
                } else if (infoKey == "pieces") {
                    infoVal->verifyType(BencStr);
                    pieces = infoVal->strData;
                }
            }
        }
        // Add whatever other info is needed...
    }

    numPieces = length/pieceLength + (length % pieceLength != 0);
    for (size_t i; i < numPieces; i++) {
        pieceQueue.push_back(i);
    }
    // No real reason we can't re-use the rng from generating our ID here
    std::shuffle(pieceQueue.begin(), pieceQueue.end(), rng);

    tracker = Tracker(announceURL, infoHash, peerID, length);
    string trackerResponse = tracker.requestTrackerInfo();

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
    uint8_t pStrLen = protocol.length();
    handshake += (char)pStrLen;
    handshake += protocol;
    uint8_t zero = 0;
    for (size_t i = 0; i < 8; i++) {
        handshake += (char)zero;
    }
    handshake += infoHash + peerID;
}

// This function makes asynchronous calls to connectToPeer(), attempting to open a connection with
// as many peers as possible up to a maximum of 30 connections
// TODO: add server functionality
void Torrent::startDownloading() {
    std::cout << "Seeders: " << numSeeders << std::endl;
    std::cout << "Leechers: " << numLeechers << std::endl;
    std::cout << "interval: " << interval << std::endl;
    std::cout << "Number of peers: " << peerList.size() << std::endl;

    if (peerList.size() == 0) {
        std::cout << "Error: no peers.\n";
        abort();
    }

    // TODO: don't use raw pointers?
    asio::io_context io;
    vector<PeerWireClient *> clients;
    for (size_t i = 0; i < peerList.size(); i++) {
        clients.push_back(new PeerWireClient(io, peerList[i], handshake, i, infoHash, numPieces, pieceQueue));
    }
    io.run();

    // TODO: call destructors?
}