all: packages
	make client

packages:
	+$(MAKE) -C external/cryptopp

client:
	g++ -o bin/client -I. -Iexternal/asio-1.28.0/include -Lexternal/cryptopp src/client.cpp src/bencode/bencode.cpp src/tracker.cpp src/torrent.cpp src/peerwire/peerwireclient.cpp -lcryptopp -lcurl -DASIO_STANDALONE -std=c++17

client-debug:
	g++ -Wall -Wextra -ggdb -pedantic -o bin/client -I. -Iexternal/asio-1.28.0/include -Lexternal/cryptopp src/client.cpp src/bencode/bencode.cpp src/tracker.cpp src/torrent.cpp src/peerwire/peerwireclient.cpp -lcryptopp -lcurl -DASIO_STANDALONE -std=c++17

clean:
	rm -f bin/*