#include <string>
#include <fstream>
#include <vector>
#include "bencode.h"

// TODO: research unit tests
BencInt::BencInt(long long val) {
    this->val = val;
}

std::string BencInt::toString() {
    return std::to_string(val);
}

BencStr::BencStr(std::string val) {
    this->val = val;
}

std::string BencStr::toString() {
    return "\"" + val + "\"";
}

BencList::BencList(std::vector<Bencoding *> val) {
    this->val = val;
}

std::string BencList::toString() {
    std::string str = "[ ";
    for (int i = 0; i < val.size(); i++) {
        Bencoding *benc = val[i];
        str += benc->toString();
        str += ", ";
    }
    str += "]";
    return str;
}

BencDict::BencDict(std::vector<std::pair<Bencoding *, Bencoding *>> val) {
    this->val = val;
}

std::string BencDict::toString() {
    std::string str = "{ ";
    for (int i = 0; i < val.size(); i++) {
        auto benc = val[i];
        str += benc.first->toString();
        str += ": ";
        str += benc.second->toString();
        str += ", ";
    }
    str += "}";
    return str;
}

Bencoding *decodeNextToken(std::ifstream& stream) {
    char c;
    c = stream.peek();
    switch (c) {
        case EOF:
            exit (-1);
        // integer
        case 'i':
            return decodeInt(stream);
        // list
        case 'l':
            return decodeList(stream);
        // dictionary
        case 'd':
            return decodeDict(stream);
        // byte string
        default:
            return decodeString(stream);
    }
}

Bencoding *decodeInt(std::ifstream& stream) {
    // we can assume the first character is an 'i' and skip it
    stream.get();
    char c;
    std::string asStr = "";
    while (stream.get(c)) {
        if (c == 'e') {
            break;
        }
        if (!isdigit(c) && c != '-') {
            return NULL;
        }
        asStr += c;
    }
    Bencoding *res = new BencInt(stoll(asStr));
    return res;
}

Bencoding *decodeList(std::ifstream& stream) {
    // we can assume first char is an 'l'
    stream.get();
    std::vector<Bencoding *> list;
    while (stream.peek() != 'e') {
        list.push_back(decodeNextToken(stream));
    }
    stream.get();
    Bencoding *res = new BencList(list);
    return res;
}

Bencoding *decodeDict(std::ifstream& stream) {
    // we can assume first char is a 'd'
    stream.get();
    std::vector<std::pair<Bencoding *, Bencoding *>> dict;
    while (stream.peek() != 'e') {
        // assumes dictionary is formatted correctly
        std::pair<Bencoding *, Bencoding *> nextPair;
        nextPair.first = decodeNextToken(stream);
        nextPair.second = decodeNextToken(stream);
        dict.push_back(nextPair);
    }
    stream.get();
    Bencoding *res = new BencDict(dict);
    return res;
}

Bencoding *decodeString(std::ifstream& stream) {
    char c;
    std::string lenAsStr = "";
    while (stream.get(c)) {
        if (c == ':') {
            break;
        }
        if (!isdigit(c)) {
            return NULL;
        }
        lenAsStr += c;
    }

    std::string str = "";
    for (int i = 0; i < stoi(lenAsStr); i++) {
        if (stream.get(c)) {
            str += c;
        }
    }

    Bencoding *res = new BencStr(str);
    return res;
}