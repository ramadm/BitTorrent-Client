#include "bencode/bencode.h"
class Torrent {
public:
    int numSeeders;
    int numLeechers;
    int interval;   // wait time between sending requests
    int minAnnounceInterval;    // min wait time between client announcing
    vector<string> peerList;
    string infoHash;
    string peerID;
    string handshake;

    Torrent(string, string, string);
    void startDownloading();
    void cancelDownload();
};