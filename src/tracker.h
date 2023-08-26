#include "bencode/bencode.h"
#include <curl/curl.h>

class Tracker {
public:
    // mandatory fields for tracker request
    string announceURL;
    // contains the SHA1 hash of the info dict in raw binary
    string infoHash;
    // represents info about the client trying to connect (us), this is generated on startup
    string peerID;
    // standard port range for BitTorrent is 6881-6889
    int port = 6881;
    long long bytesUploaded = 0;
    long long bytesDownloaded = 0;
    long long length;
    string event = "started";

    // TODO: add response fields we want to store

    Tracker();
    Tracker(string announceURL, string infoHash, string peerID, long long length);
    string requestTrackerInfo();
    static size_t writeCallback(void *, size_t, size_t, void *);
};