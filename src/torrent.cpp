#include "torrent.h"
#include <sstream>
#include <iostream>

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
}