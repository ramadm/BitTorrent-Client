#include "metainfo.h"

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
        } else if (key == "announce-list") {
            value->verifyType(BencList);
            vector<string> annList;
            // Get the raw strings from the list of Bencoded strings
            for (int i = 0; i < value->listData.size(); i++) {
                value->listData[i]->verifyType(BencStr);
                annList.push_back(value->listData[i]->strData);
            }
            announceList = annList;
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

