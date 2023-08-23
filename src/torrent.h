#include "bencode/bencode.h"
#include "tracker.h"

class Torrent {
public:
    Bencoding *metainfo;
    Tracker tracker;

    string announceURL;
    long long length;
    int numSeeders;
    int numLeechers;
    int interval;   // wait time between sending requests
    int minAnnounceInterval;    // min wait time between client announcing
    vector<string> peerList;
    string infoHash;
    string peerID;
    string handshake;

    Torrent(Bencoding *minfo);
    void startDownloading();
};