#include "bencode/bencode.h"
#include "external/cryptopp/sha.h"
#include "external/cryptopp/hex.h"

class Tracker {
public:
    // mandatory fields for tracker request
    string announceURL;
    // this is a SHA1 hash of the info dict from the metainfo file
    // currently it is a hex-encoded string representation
    string infoHash;
    string peerID;
    int port;
    int bytesUploaded;
    int bytesDownloaded;
    int bytesLeft;
    int compact;
    int noPeerID;
    string event;

    // TODO: add response fields we want to store

    Tracker(Bencoding *);
};