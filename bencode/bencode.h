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

Bencoding *parseInt(std::ifstream&);
Bencoding *parseStr(std::ifstream&);
Bencoding *parseList(std::ifstream&);
Bencoding *parseDict(std::ifstream&);
Bencoding *parse(std::ifstream&);