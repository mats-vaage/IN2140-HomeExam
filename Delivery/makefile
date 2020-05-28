all: clientUdp udpServer 



clientUdp: clientUdp.c 
	gcc -g -o clientUdp clientUdp.c utils.c send_packet.c pgmread.c

udpServer: udpServer.c 
	gcc -g -o udpServer udpServer.c utils.c pgmread.c

clean:
	rm -f *o clientUdp, udpServer
 
debug: CFLAGS += -DDEBUG -g
debug: all


