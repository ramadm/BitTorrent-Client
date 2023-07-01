#include <string>
#include <fstream>
#include <vector>

class Bencoding {
public:
    virtual std::string toString() = 0;
};

class BencInt : public Bencoding {
public:
    long long val;
    BencInt(long long);
    std::string toString();
};

class BencStr : public Bencoding {
public:
    std::string val;
    BencStr(std::string);
    std::string toString();
};

class BencList : public Bencoding {
public:
    std::vector<Bencoding *> val;
    BencList(std::vector<Bencoding *>);
    std::string toString();
};

class BencDict : public Bencoding {
public:
    std::vector<std::pair<Bencoding *, Bencoding *>> val;
    BencDict(std::vector<std::pair<Bencoding *, Bencoding *>>);
    std::string toString();
};

Bencoding *decodeNextToken(std::ifstream& stream);

Bencoding *decodeInt(std::ifstream& stream);

Bencoding *decodeList(std::ifstream& stream);

Bencoding *decodeDict(std::ifstream& stream);

Bencoding *decodeString(std::ifstream& stream);