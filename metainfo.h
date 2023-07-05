#include <string>
#include <vector>
#include "bencode/bencode.h"

class Metainfo {
private:
    std::string announce;
    std::vector<std::string> announceList;
    std::string comment;
    std::string createdBy;
    int creationDate;
    std::string encoding;

    //info section
    int pieceLength;
    // TODO: this is a hex-encoded number (or list of numbers), so probably best not to store it as a string.
    std::string pieces;

    bool multiFileMode;
    // Name of the directory if multi-file mode, else file name
    std::string name;
    // length of each file if multi-file mode, else just 1 entry
    std::vector<int> lengths;
    // dest. path of each file if multi-file mode, else empty
    std::vector<std::string> paths;

    // TODO: add any other info if needed

public:
    void initMetainfo(Bencoding *);
};