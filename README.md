Runnable in Linux

Make sure client.cpp, server.cpp, clients.cpp, server.conf, client.conf, and Makefile are all included in linux directory
Use "make" command to compile files, 
then run ./server server.conf on server terminal
and ./client client.conf on client terminal

if this does not work, then run these commands in this order:
g++ server.cpp -o server
g++ client.cpp -o client
./server server.conf
./client client.conf

since clients.cpp is included in server.cpp file, compiling server.cpp will compile clients.cpp as well
