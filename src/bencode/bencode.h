#ifndef BENCODE
#define BENCODE

#include <vector>
#include <string>
#include <fstream>
#include <variant>

using std::string;
using std::vector;
using std::pair;

enum BencType {
    BencInt,
    BencStr,
    BencList,
    BencDict
};

class Bencoding {
public:
    BencType type;
    long long intData;
    string strData;
    // TODO: consider using std::unique_ptr for vectors
    vector<Bencoding *> listData;
    vector<pair<string, Bencoding *>> dictData;

    Bencoding(long long);
    Bencoding(string);
    Bencoding(vector<Bencoding *>);
    Bencoding(vector<pair<string, Bencoding *>>);
    string toString();
    void verifyType(BencType);
};

Bencoding *parseInt(std::istream&);
Bencoding *parseStr(std::istream&);
Bencoding *parseList(std::istream&);
Bencoding *parseDict(std::istream&);
Bencoding *parse(std::istream&);

Bencoding *parseInt(string);
Bencoding *parseStr(string);
Bencoding *parseList(string);
Bencoding *parseDict(string);
Bencoding *parse(string);

string bencode(Bencoding *);

#endif