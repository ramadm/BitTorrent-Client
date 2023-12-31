#include <iostream>
#include <filesystem>
#include <fstream>
#include "torrent.h"

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

    Bencoding *metainfo;
    try {
        std::ifstream stream(argv[1]);
        metainfo = parse(stream);
    } catch (std::invalid_argument const& e) {
        // TODO: handle exception
    }

    // TODO: nest in try-catch
    Torrent torrent(metainfo);
    torrent.startDownloading();
    return 0;
}