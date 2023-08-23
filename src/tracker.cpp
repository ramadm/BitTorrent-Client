#include "tracker.h"
#include <iostream>
#include <random>
#include <sstream>
#include <unistd.h>

Tracker::Tracker() {
    announceURL = "";
    infoHash = "";
    peerID = "";
    length = 0;
}

Tracker::Tracker(string announceURL, string infoHash, string peerID, long long length) {
    this->announceURL = announceURL;
    this->peerID = peerID;
    this->infoHash = infoHash;
    this->length = length;
}

size_t Tracker::writeCallback(void *data, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)data, size * nmemb);
    return size * nmemb;
}

// Attempts to get peer list by sending an HTTP GET request to the announceURL
string Tracker::requestTrackerInfo() {

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
        if (res != CURLE_OK) { 
            // TODO: sometimes connection with the tracker fails and I'm not sure why
            // Running client again usually fixes it
            abort();
        }
        
        curl_easy_cleanup(curl);
    }
    return buffer;
}