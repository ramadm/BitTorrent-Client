all:
	+$(MAKE) -C external/cryptopp/
	make client

client:
	g++ -Wall -Wextra -pedantic -o client -I. -Lexternal/cryptopp src/client.cpp src/bencode/bencode.cpp src/tracker.cpp -lcryptopp

clean:
	rm -f client
