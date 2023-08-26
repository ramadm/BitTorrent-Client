#include "bencode/bencode.h"
#include "tracker.h"

class Torrent {
public:
    Bencoding *metainfo;
    Tracker tracker;

    string fileName;
    string announceURL;
    long long length;
    long long pieceLength;
    int numSeeders;
    int numLeechers;
    int interval;   // wait time between sending requests
    int minAnnounceInterval;    // min wait time between client announcing
    vector<string> peerList;
    string infoHash;
    string peerID;
    string handshake;
    // TODO: Consider changing the way this is stored for performance reasons
    string pieces;

    Torrent(Bencoding *minfo);
    void startDownloading();
};