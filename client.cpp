#include <iostream>
#include <filesystem>
#include <fstream>
#include "bencode/bencode.h"

#define STR_USAGE "Usage: ./client <file.torrent>"

void printUsage(void) {
    std::cerr << STR_USAGE << std::endl;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printUsage();
        return -1;
    }
    
    // test file has .torrent extension
    std::filesystem::path p(argv[1]);
    std::filesystem::path ext = p.extension();
    if (ext.empty() || ext.compare(".torrent") != 0) {
        printUsage();
        return -1;
    }

    // open torrent metainfo file and print contents
    std::ifstream metainfo(p.c_str());
    Bencoding *benc; 
    while(metainfo.peek() != EOF) {
        benc = decodeNextToken(metainfo);
        std::cout << benc->toString();
        delete benc;
    }
    metainfo.close();

    return 0;
}