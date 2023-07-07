#include "metainfo.h"
#include <fstream>
#include <iostream>

int main() {
    std::ifstream stream("ubuntu-23.04-desktop-amd64.iso.torrent");
    Bencoding *benc = parse(stream);
    Metainfo meta(benc);
    std::cout << meta.announce << std::endl;
    for (int i = 0; i < meta.announceList.size(); i++) {
        std::cout << meta.announceList[i] << std::endl;
    }
    std::cout << meta.comment << std::endl;
    std::cout << meta.createdBy << std::endl;
    std::cout << meta.creationDate << std::endl;
    std::cout << meta.encoding << std::endl;
    std::cout << "-- info section --" << std::endl;
    std::cout << meta.infoSection.length << std::endl;
    std::cout << meta.infoSection.name << std::endl;
    std::cout << meta.infoSection.pieceLength << std::endl;

    stream.close();
}