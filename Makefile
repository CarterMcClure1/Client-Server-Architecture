output: server.o client.o
	g++ server.o -o server
	g++ client.o -o client

server.o: server.cpp clients.cpp  
	g++ -c server.cpp

client.o: client.cpp
	g++ -c client.cpp
