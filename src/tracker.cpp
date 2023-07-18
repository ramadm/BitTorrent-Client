#include "tracker.h"

Tracker::Tracker(Bencoding *metainfo) {
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
            CryptoPP::HexEncoder encoder;
            encoder.Attach(new CryptoPP::StringSink(infoHash));
            encoder.Put(digest, sizeof(digest));
            encoder.MessageEnd();
        }
        // Add whatever other info is needed...
    }
}