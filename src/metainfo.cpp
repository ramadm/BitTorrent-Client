#include "metainfo.h"
#include <iostream>
#include "external/cryptopp/sha.h"

Info::Info(Bencoding *benc) {
    benc->verifyType(BencDict);
    auto dict = benc->dictData;
    for (size_t i = 0; i < dict.size(); i++) {
        auto entry = dict[i];
        string key = entry.first;
        Bencoding *value = entry.second;
        if (key == "length") {
            value->verifyType(BencInt);
            length = value->intData;
        } else if (key == "name") {
            value->verifyType(BencStr);
            name = value->strData;
        } else if (key == "piece length") {
            value->verifyType(BencInt);
            pieceLength = value->intData;
        } else if (key == "pieces") {
            value->verifyType(BencStr);
            pieces = value->strData;
        }
    }
    // TODO: initalize a structured pieces member
}

// Initialize a Metainfo object from a .torrent file
// Need to verify that this function is exception safe
// I'm worried parse might leave some Bencoding *s floating around, and the ifstream might not get
// closed properly if we throw an exception in parse
Metainfo::Metainfo(std::string filepath) {
//Metainfo::Metainfo(Bencoding *benc) {
    // TODO: test .torrent extension
    std::ifstream stream(filepath);
    Bencoding *benc = parse(stream);
    stream.close();
    benc->verifyType(BencDict);
    auto dict = benc->dictData;
    for (size_t i = 0; i < dict.size(); i++) {
        auto entry = dict[i];
        string key = entry.first;
        Bencoding *value = entry.second;
        if (key == "announce") {
            value->verifyType(BencStr);
            announce = value->strData;
        // announce-list is a list of list of strings containing URLs
        } else if (key == "announce-list") {
            value->verifyType(BencList);
            for (size_t i = 0; i < value->listData.size(); i++) {
                Bencoding *subList = value->listData[i];
                subList->verifyType(BencList);
                for (size_t j = 0; j < subList->listData.size(); j++) {
                    Bencoding *annListEntry = subList->listData[j];
                    annListEntry->verifyType(BencStr);
                    announceList.push_back(annListEntry->strData);
                }
            }
        } else if (key == "created by") {
            value->verifyType(BencStr);
            createdBy = value->strData;
        } else if (key == "comment") {
            value->verifyType(BencStr);
            comment = value->strData;
        } else if (key == "creation date") {
            value->verifyType(BencInt);
            creationDate = value->intData;
        } else if (key == "encoding") {
            value->verifyType(BencStr);
            encoding = value->strData;
        } else if (key == "info") {
            infoSection = Info(value);
            CryptoPP::byte digest[CryptoPP::SHA1::DIGESTSIZE];
            string bencodedInfo = bencode(value);
            // TODO: replace 0 with length of bencodedInfo in bytes;
            size_t infoLen = bencodedInfo.length();
            CryptoPP::SHA1().CalculateDigest(digest, (CryptoPP::byte *)bencodedInfo.c_str(), infoLen);

            // TODO: re-bencode info section -> sha1 hash
        }
    }
}