#include "tracker.h"
#include <iostream>
#include <random>
#include <sstream>

Tracker::Tracker(Bencoding *metainfo) {
    // initialize a peer ID
    std::random_device dev;
    std::mt19937 rng(dev());
    // random 12 character long sequence of numbers
    std::uniform_int_distribution<std::mt19937::result_type> dist12char(100000000000, 999999999999);
    peerID = "-rd0001-" + std::to_string(dist12char(rng));

    // Populate fields with data from metainfo file
    metainfo->verifyType(BencDict);
    for (size_t i = 0; i < metainfo->dictData.size(); i++) {
        string key = metainfo->dictData[i].first;
        Bencoding *value = metainfo->dictData[i].second;
        if (key == "announce") {
            value->verifyType(BencStr);
            announceURL = value->strData;
        } else if (key == "info") {
            // Get the info_hash param, which is a sha1 hash of the info dict from metainfo file
            string infoStr = bencode(value);
            size_t infoLen = infoStr.length();
            CryptoPP::byte digest[CryptoPP::SHA1::DIGESTSIZE];
            CryptoPP::SHA1().CalculateDigest(digest, (CryptoPP::byte *)infoStr.c_str(), infoLen);  
            infoHash += (char *)digest;
        } else if (key == "length") {
            value->verifyType(BencInt);
            length = value->intData;
        }
        // Add whatever other info is needed...
    }
}

size_t Tracker::writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Attempts to get peer list by sending an HTTP GET request to the announceURL
void Tracker::getPeerList() {

    // format the URL for a GET request
    string requestURL = announceURL + '?';
    requestURL += "info_hash=";
    // urlencode the info hash
    char *urlInfoHash = curl_easy_escape(NULL, infoHash.c_str(), infoHash.length());
    requestURL += urlInfoHash;
    curl_free(urlInfoHash);
    requestURL += "&peer_id=";
    requestURL += peerID;
    requestURL += "&port=6881";
    requestURL += "&uploaded=0";
    requestURL += "&downloaded=0";
    requestURL += "&left=";
    requestURL += std::to_string(length);
    requestURL += "&compact=1";
    requestURL += "&event=started";

    // init a handle for a single file transfer (the request)
    CURL *curl = curl_easy_init();
    // the buffer to write data
    string buffer;
    CURLcode res;

    if (curl) {


        // set options
        curl_easy_setopt(curl, CURLOPT_URL, requestURL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

        // send the request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) { std::cout << "Error occurred in curl_easy_perform().\n"; }
        
        curl_easy_cleanup(curl);
        std::istringstream ss(buffer);
        std::cout << parse(ss)->toString() << std::endl;
    }

    // TODO: Edit GET request to the proper tracker request
    // TODO: Edit writeCallback() to fill tracker class data
}