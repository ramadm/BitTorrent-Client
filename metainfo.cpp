#include "metainfo.h"
#include <iostream>

Info::Info(Bencoding *benc) {
    benc->verifyType(BencDict);
    auto dict = benc->dictData;
    for (int i = 0; i < dict.size(); i++) {
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

// Initialize a Metainfo class from the Bencoded dictionary in the torrent file
Metainfo::Metainfo(Bencoding *benc) {
    benc->verifyType(BencDict);
    auto dict = benc->dictData;
    for (int i = 0; i < dict.size(); i++) {
        auto entry = dict[i];
        string key = entry.first;
        Bencoding *value = entry.second;
        if (key == "announce") {
            value->verifyType(BencStr);
            announce = value->strData;
        // announce-list is a list of list of strings containing URLs
        } else if (key == "announce-list") {
            value->verifyType(BencList);
            for (int i = 0; i < value->listData.size(); i++) {
                Bencoding *subList = value->listData[i];
                subList->verifyType(BencList);
                for (int j = 0; j < subList->listData.size(); j++) {
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
        }
    }
}

