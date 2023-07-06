#include "bencode/bencode.h"

// Should this be nested in Metainfo?
// Should this be a struct?
class Info {
public:
    int length;
    string name;
    int pieceLength;
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
    // should this be a long long?
    int creationDate;
    string encoding;
    Info infoSection;
    Metainfo(Bencoding *);
};