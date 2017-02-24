all:	EchoClient	EchoServer	RPCServer	RPCClient
PHONY: all 
EchoClient: EchoClient.c
	gcc -o EchoClient EchoClient.c

EchoServer: EchoServer.c
	gcc -o EchoServer EchoServer.c

RPCServer: RPCServer.c
	gcc -o RPCServer RPCServer.c

RPCClient: RPCClient.c
	gcc -o RPCClient RPCClient.c

clean:
	rm EchoServer EchoClient RPCServer RPCClient
	