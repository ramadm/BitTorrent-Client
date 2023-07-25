#include "bencode/bencode.h"
class Torrent {
public:
    int numSeeders;
    int numLeechers;
    int interval;   // wait time between sending requests
    int minAnnounceInterval;    // min wait time between client announcing
    vector<string> peerList;

    // TODO: add the tracker info needed to maintain a download

    Torrent(string trackerResponse);
    void startDownloading();
    void cancelDownload();
};