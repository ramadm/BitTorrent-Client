#include "bencode/bencode.h"
#include "tracker.h"
#include <deque>

class Torrent {
public:
    Bencoding *metainfo;
    Tracker tracker;

    string fileName;
    string announceURL;
    long long length;
    long long pieceLength;
    size_t numPieces;
    int numSeeders;
    int numLeechers;
    int interval;   // wait time between sending requests
    int minAnnounceInterval;    // min wait time between client announcing
    vector<string> peerList;
    string infoHash;
    string peerID;
    string handshake;

    // hashes of the pieces
    string pieces;
    // randomly ordered queue of /indices/ of pieces
    std::deque<uint32_t> pieceQueue;


    Torrent(Bencoding *minfo);
    void startDownloading();
};