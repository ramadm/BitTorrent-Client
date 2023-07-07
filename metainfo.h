#include "bencode/bencode.h"
#include <ctime>

// Should this be nested in Metainfo?
// Should this be a struct?
// TODO: multi-file case
// TODO: creationDate should be time_t if it's going to be displayed
class Info {
public:
    int length = 0;
    string name;
    int pieceLength = 0;
    // TODO: how should pieces be formatted?
    string pieces;
    Info(Bencoding *);
    Info() = default;
};

class Metainfo {
public:
    string announce;
    vector<string> announceList;
    string comment;
    string createdBy;
    int creationDate = 0;
    string encoding;
    Info infoSection;
    Metainfo(Bencoding *);
};