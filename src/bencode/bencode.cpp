#include "bencode.h"
#include <iostream>

// TODO: Must be designed for safe use with exceptions

Bencoding::Bencoding(long long val) {
    type = BencInt;
    intData = val;
}

Bencoding::Bencoding(string val) {
    type = BencStr;
    strData = val;
}

Bencoding::Bencoding(vector<Bencoding *> val) {
    type = BencList;
    listData = val;
}

Bencoding::Bencoding(vector<pair<string, Bencoding *>> val) {
    type = BencDict;
    dictData = val;
}

// TODO: It may be preferable to have getters
void Bencoding::verifyType(BencType correctType) {
    if (type != correctType) { throw std::invalid_argument("Incorrect data type."); }
}

string Bencoding::toString() {
    switch (type) {
        case BencInt:
            return (std::to_string(intData));
        case BencStr:
            return "\"" + strData + "\"";
        case BencList:
        {
            string listAsStr = "[ ";
            for (size_t i = 0; i < listData.size(); i++) {
                listAsStr += listData[i]->toString();
                listAsStr += ", ";
            }
            listAsStr += "]";
            return listAsStr;
        }
        case BencDict:
        {
            string dictAsStr = "{ ";
            for (size_t i = 0; i < dictData.size(); i++) {
                dictAsStr += "\"" + dictData[i].first + "\"";
                dictAsStr += ": ";
                dictAsStr += dictData[i].second->toString();
                dictAsStr += ", ";
            }
            dictAsStr += "}";
            return dictAsStr;
        }
    }
    return "";
}

Bencoding *parseInt(std::istream& stream) {
    // skip 'i'
    long long val;
    stream.get();
    char c;
    string asStr = "";
    while (stream.get(c)) {
        if (c == 'e') { break; }
        asStr += c;
    }
    try {
        val = std::stoll(asStr);
    // TODO: If the try-catch was around the call to parse, we could continue with execution and
    // tell the user the file was formatted incorrectly!
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what();
        abort();
    }
    return new Bencoding(val);
}

Bencoding *parseStr(std::istream& stream) {
    char c;
    string lengthAsStr = "";
    while (stream.get(c)) {
        if (!isdigit(c)) {
            break;
        }
        lengthAsStr += c;
    }
    if (c != ':') {
        throw std::invalid_argument("Invalid string format.");
    }
    string val = "";
    int length = stoi(lengthAsStr);
    for (int i = 0; i < length; i++) {
        if (!stream.get(c)) { break; }
        val += c;
    }
    return new Bencoding(val);
}

Bencoding *parseList(std::istream& stream) {
    // skip 'l'
    stream.get();
    vector<Bencoding *> val;
    while (stream.peek()) {
        if (stream.peek() == 'e') { break; }
        val.push_back(parse(stream));
    }
    stream.get();
    return new Bencoding(val);
}

Bencoding *parseDict(std::istream& stream) {
    // skip 'd'
    stream.get();
    vector<pair<string, Bencoding *>> dict;
    while (stream.peek()) {
        if (stream.peek() == 'e') { break; }
        Bencoding *bencKey = parse(stream);
        if (bencKey->type != BencStr) {
            throw std::invalid_argument("Non-string data cannot be dictionary keys.");
        }
        Bencoding *value = parse(stream);
        pair<string, Bencoding *> kvPair;
        kvPair.first = bencKey->strData;
        kvPair.second = value;
        dict.push_back(kvPair);
    }
    stream.get();
    return new Bencoding(dict);
}

/**
 * Parse a Bencoded file and returns a Bencoding object. Throws invalid_argument exception if file
 * is incorrectly formatted.
*/
Bencoding *parse(std::istream& stream) {
    char c = stream.peek();
    switch (c) {
        case 'i':
            return parseInt(stream);
        case 'l':
            return parseList(stream);
        case 'd':
            return parseDict(stream);
        // string
        default:
            if (!isdigit(c)) {
                throw std::invalid_argument("Not a valid start to Bencoded sequence.");
            }
            return parseStr(stream);
    }
}

string bencode(Bencoding *benc) {
    switch(benc->type) {
        case BencInt:
        {   
            string val = "i";
            val += std::to_string(benc->intData);
            val += 'e';
            return val;
        }
        case BencStr:
        {
            string val = std::to_string(benc->strData.length());
            val += ':';
            val += benc->strData;
            return val;
        }
        case BencList:
        {
            string val = "l";
            for (size_t i = 0; i < benc->listData.size(); i++) {
                val += bencode(benc->listData[i]);
            }
            val += 'e';
            return val;
        }
        case BencDict:
        {
            string val = "d";
            for (size_t i = 0; i < benc->dictData.size(); i++) {
                string key = benc->dictData[i].first;
                val += std::to_string(key.length());
                val += ':';
                val += key;
                val += bencode(benc->dictData[i].second);
            }
            val += 'e';
            return val;
        }
    }
    return "";
}