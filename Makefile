all: packages
	make client

packages:
	sudo apt-get install libcurl4-openssl-dev
	+$(MAKE) -C external/cryptopp

client:
	g++ -Wall -Wextra -pedantic -o client -I. -Lexternal/cryptopp src/client.cpp src/bencode/bencode.cpp src/tracker.cpp src/torrent.cpp -lcryptopp -lcurl

clean:
	rm -f client
