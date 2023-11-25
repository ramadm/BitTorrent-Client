# BitTorrent-Client
A small Linux terminal-based BitTorrent client.

The main purpose of this project was to learn about asynchronous networking and the BitTorrent protocol. The bulk of the project is implemented in `peerwireclient.cpp`, where most of the networking work is carried out using the ASIO networking library.

Some info about how the BitTorrent protocol works:
- Files are downloaded in a distributed manner. Your client will connect to multiple peers, each of which will serve the client "pieces" of the full file.
- Info about the file you're trying to download is stored in a `.torrent` file, encoded using the Bencode format.
- The client gets info about the file and a special tracker URL from the `.torrent` file. It gets peer info by sending an HTTP GET request to the tracker.

This client consequently works in 3 stages:
- First, the `.torrent` file is opened and the file info is scraped from it using a custom Bencode parser.
- Second, the client makes a request to the tracker using the cURL library to format a GET request.
- Third, once the tracker has responded with peer info, a `PeerWireClient` object is created for each peer. All peers share a single ASIO `io_context` object which handles asychronous messaging between the client and peers. This is where the actual downloading happens.

Currently the client only supports downloading a single-file torrent from the `.torrent` file. Some future features may include:
- Multi-file downloads
- Multiple concurrent downloads
- Magnet links
- Pausing/restarting torrents

More info on how the BitTorrent protocol works can be found on the wiki:
https://wiki.theory.org/BitTorrentSpecification

# Building
On Ubuntu you can install libcurl with
```
sudo apt-get install libcurl4-openssl-dev
```
After that just run 'make' in the root directory to build the client, and it should automatically
build the other dependencies.

# Running
From the `bin/` directory, you can run the client with
```
./client <file.torrent>
```
to start downloading. The file will be created in the current directory.
