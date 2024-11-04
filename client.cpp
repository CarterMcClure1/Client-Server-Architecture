#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>

constexpr size_t MAXDATASIZE = 300;

void* get_in_addr(struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: client client.conf\n";
        return 1;
    }

    // Read config file
    std::string serverIP, serverPort;
    std::ifstream configFile(argv[1]);
    if (!configFile.is_open()) {
        std::cerr << "Error opening config file: " << argv[1] << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        if (line.find("SERVER_IP=") == 0) {
            serverIP = line.substr(10);
        } else if (line.find("SERVER_PORT=") == 0) {
            serverPort = line.substr(12);
        }
    }
    configFile.close();

    if (serverIP.empty() || serverPort.empty()) {
        std::cerr << "Invalid config file format.\n";
        return 1;
    }

    // Set up connection hints
    addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Get address information
    int rv = getaddrinfo(serverIP.c_str(), serverPort.c_str(), &hints, &servinfo);
    if (rv != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
        return 1;
    }

    int sockfd;
    // Loop through results and try to connect
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == nullptr) {
        std::cerr << "client: failed to connect\n";
        return 2;
    }

    // Display connection info
    char s[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof s);
    std::cout << "client: connecting to " << s << std::endl;

    freeaddrinfo(servinfo);


    //After client connects, obtain TCP port it was assigned 
    sockaddr_in localAddr;
    socklen_t addrLen = sizeof(localAddr);
    if (getsockname(sockfd, (struct sockaddr*)&localAddr, &addrLen) == -1) {
        perror("getsockname");
        close(sockfd);
        return 1;
    }

    //get given TCP port, set UDP with fixed offset
    int clientTCPPort = ntohs(localAddr.sin_port);
    int udpPort = clientTCPPort + 100;


    std::vector<int> udpSockets;
    char buf[MAXDATASIZE];
    std::string userInput;

    //SET up UDP socket
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(udpSocket == -1) {
        perror("UDP socket failed to set up");
        close(sockfd);
        return 1;
    }

    //prepare UDP structure with the set offset from given TCP port
    
    struct sockaddr_in udpAddress;
    udpAddress.sin_family = AF_INET; 
    udpAddress.sin_port = htons(udpPort); 
    udpAddress.sin_addr.s_addr = INADDR_ANY; 

    //bind it to udp socket
    if (bind(udpSocket, (struct sockaddr*)&udpAddress, sizeof(udpAddress)) == -1) {
        perror("UDP bind failed");
        close(udpSocket);
        close(sockfd);
        return 1;
    }

    udpSockets.push_back(udpSocket);
    std::cout << "Client bound UDP socket to port: " << udpPort << std::endl;
    



    fd_set tempsocklist;
    int maxsocknumber;

while (true) {
    FD_ZERO(&tempsocklist);
    FD_SET(sockfd, &tempsocklist);          //set up temp socket list
    FD_SET(udpSocket, &tempsocklist);       //add TCP and UDP sockets
    FD_SET(STDIN_FILENO, &tempsocklist);    //set user input

    maxsocknumber = std::max(sockfd, udpSocket);


    // Use select to wait for activity from server or user
    if (select(maxsocknumber + 1, &tempsocklist, nullptr, nullptr, nullptr) == -1) {
        perror("select");
        break;
    }

    // Check for input from user
    if (FD_ISSET(STDIN_FILENO, &tempsocklist)) {
        std::cout << "> ";
        std::getline(std::cin, userInput);
        if (userInput == "exit") {
            break; 
        }

        if (send(sockfd, userInput.c_str(), userInput.size(), 0) == -1) {
            perror("send");
            break;
        }
    }

    // Check for tcp packet from server
    if (FD_ISSET(sockfd, &tempsocklist)) {
        int numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0);
        if (numbytes == -1) {
            perror("recv");
            break;
        } else if (numbytes == 0) {
            std::cout << "Server closed the connection.\n";
            break;
        }

        buf[numbytes] = '\0';
        std::cout << "Server: " << buf  <<  std::endl;
        
    }


    //Check for udp packet from server
     if (FD_ISSET(udpSocket, &tempsocklist)) {
            sockaddr_in senderAddr;
            socklen_t addr_len = sizeof(senderAddr);
            int numbytes = recvfrom(udpSocket, buf, MAXDATASIZE - 1, 0, (struct sockaddr*)&senderAddr, &addr_len);
            if (numbytes == -1) {
                perror("recvfrom");
                break;
            }

            buf[numbytes] = '\0';
            std::cout << "Server(UDP): " << buf << std::endl;
        }
}

    close(udpSocket);
    close(sockfd);
    return 0;
}