#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <cerrno>
#include <system_error>
#include <fstream>
#include <algorithm>
#include <vector>
#include <set>
#include <ctime>
#include <map>
#include "clients.cpp"
#include <unistd.h>
#include <sstream>
#include <ctime>
#include <sys/sysinfo.h>


#define BACKLOG 10
#define MAXDATASIZE 100


    


void* get_in_addr(struct sockaddr* sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

std::string toCamelCase(const std::string& input)
{
    std::string output;
    bool capitalize = true;

    for (char c : input) {
        if (std::isalpha(c)) {
            if (capitalize) {
                output += std::toupper(c);
            } else {
                output += std::tolower(c);
            }
            capitalize = !capitalize;
        } else {
            output += c;
        }
    }
    return output;
}

void logConnection(const std::string& clientIP)
{
    time_t now = time(nullptr); 
    tm* localTime = localtime(&now); 
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localTime);
    std::cout << "[" << timestamp << "] Connection from: " << clientIP << std::endl;
}

void logDisconnection(const std::string& clientIP)
{
    time_t now = time(nullptr);  
    tm* localTime = localtime(&now); 
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localTime);
    std::cout << "[" << timestamp << "] Client disconnected: " << clientIP << std::endl;
}



int main(int argc, char* argv[])
{
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int yes = 1;

    fd_set socket_list, temp_socket_list; 
    int maxsocknum; 

    ClientManager clientManager;

    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    std::memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    std::string configFileName = argv[1];
    std::string port;
    std::ifstream configFile(configFileName);
    if (!configFile.is_open()) {
        std::cerr << "Error opening configuration file: " << configFileName << std::endl;
        return 1;
    }

    std::string heartbeatInterval;
    std::string statsInterval;
    std::string line;
    while (std::getline(configFile, line)) {
    if (line.substr(0, 9) == "TCP_PORT=") {
        port = line.substr(9);
    } else if (line.substr(0, 19) == "HEARTBEAT_INTERVAL=") {
        heartbeatInterval = line.substr(19);
    } else if (line.substr(0, 15) == "STATS_INTERVAL=") {
        statsInterval = line.substr(15);
    }
}

    

    configFile.close();

   
    int heartbeatIntervalTimer = stoi(heartbeatInterval);
    int statsIntervalTimer = stoi(statsInterval);
    

   


    if (port.empty()) {
        std::cerr << "Port number not found in configuration file!" << std::endl;
        return 1;
    }

    if ((rv = getaddrinfo(nullptr, port.c_str(), &hints, &servinfo)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
        return 1;
    }

    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            std::perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            throw std::system_error(errno, std::generic_category(), "setsockopt");
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            std::perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == nullptr) {
        std::cerr << "server: failed to bind" << std::endl;
        return 1;
    }

    if (listen(sockfd, BACKLOG) == -1) {
        throw std::system_error(errno, std::generic_category(), "listen");
    }


// Clear socket/temp socket list and add listening socket to socket list, initialize max socket number
    FD_ZERO(&socket_list); 
    FD_ZERO(&temp_socket_list);
    FD_SET(sockfd, &socket_list); 
    maxsocknum = sockfd; 

    std::cout << "server: waiting for connections..." << std::endl;

    // Get the current time at the start
   // clock_t lastHeartbeat = clock();
    //const double heartbeatIntervalInSeconds = static_cast<double>(heartbeatIntervalTimer) / CLOCKS_PER_SEC;

    while (true) {
         temp_socket_list = socket_list; // COPY SOCKET LIST TO TEMP LIST

        //SELECT() to make sure multiplexing is going, allows mult. clients to interact
        if (select(maxsocknum + 1, &temp_socket_list, nullptr, nullptr, nullptr) == -1) {
            perror("select");
            exit(4);
        }

        //clock_t currentTime = clock();
        //bool shouldSendHeartbeat = static_cast<double>(currentTime - lastHeartbeat) / CLOCKS_PER_SEC >= heartbeatIntervalInSeconds;
       // if(shouldSendHeartbeat) {
       //     std::cout << "Send heartbeat";
       // }

        // For loop to continously look for data incoming from clients
        for (int sock = 0; sock <= maxsocknum; sock++) {
            //this if handles new connections and calls accept()
            if (FD_ISSET(sock, &temp_socket_list)) { 
                if (sock == sockfd) {
                    
                    sin_size = sizeof their_addr;
                    new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
                    if (new_fd == -1) {
                        perror("accept");
                        continue;
                    }

                    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s);
                    logConnection(s);

                    // Add the new client socket to the socket list and update max sock number
                    FD_SET(new_fd, &socket_list);
                    if (new_fd > maxsocknum) { 
                        maxsocknum = new_fd;
                    }

                    clientManager.addClient(new_fd);
                    
                    

                } else {
                    // This else handles incoming data from client, call recv function
                    char buf[MAXDATASIZE];
                     
                    int numbytes;

                    if ((numbytes = recv(sock, buf, sizeof buf, 0)) <= 0) {    //if recv gets nothing, close connection
                        
                        if (numbytes == 0) {
                            inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s);
                            logDisconnection(s);
                        } else {
                            perror("recv");
                        }
                        close(sock); // Close the socket
                        FD_CLR(sock, &socket_list); // Remove from socket list
                         // Remove from client manager(ADD THIS)
                    } else {                                                        //if recv gets data, start looking for commands entered                                        
                        
                        buf[numbytes] = '\0';
                        std::string receivedMsg(buf);
                        std::string camelCaseMsg = toCamelCase(receivedMsg);
                        

                        struct sockaddr_storage clientAddr;
                        socklen_t addrLen = sizeof(clientAddr);

                        //GET GIVEN TCP ADDRESS AND SAVE UDP ADDRESS TO CLIENT MANAGER

                         if (getpeername(sock, (struct sockaddr*)&clientAddr, &addrLen) == -1) {
                        perror("getpeername");
                        // If getpeername fails, do not proceed further
                         } else {
                        int clientPort;
                       if (clientAddr.ss_family == AF_INET) {
                         // IPv4
                         struct sockaddr_in* ipv4 = (struct sockaddr_in*)&clientAddr;
                          clientPort = ntohs(ipv4->sin_port);  
                          } else if (clientAddr.ss_family == AF_INET6) {
                         // IPv6
                        struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)&clientAddr;
                        clientPort = ntohs(ipv6->sin6_port); 
                          } 

        
                        //set UDP port for user, using TCP port + 100
                        clientManager.setUDPPort(sock, clientPort);
                     }


                     



// HANDLE NICK COMMAND
                        if (camelCaseMsg.find("NiCk") == 0) {
                            if(camelCaseMsg != "NiCk" && camelCaseMsg != "NiCk ") {
                            std::string nickname = receivedMsg.substr(5);
                            if(isalpha(nickname[0]) && std::all_of(nickname.begin() + 1, nickname.end(), ::isalnum)) {
                            if (clientManager.isNicknameTaken(nickname)) {
                                std::string serverResponse = "433 ERR_NICKNAMEINUSE\n" + nickname + " :Nickname is already in use";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                            } else {
                                clientManager.setClientNickname(sock, nickname);
                                std::string serverResponse = "Nickname changed";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                            }
                            }
                            else {
                                std::string serverResponse = "432 ERR_ERRONEUSNICKNAME\n" + nickname + " :Erroneous nickname";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0); 
                            }
                            }
                            else {
                               std::string serverResponse = "431 ERR_NONICKNAMEGIVEN\n:No nickname given";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0); 
                            }
                        }

//HANDLE USER COMMAND
                        if (camelCaseMsg.find("UsEr") == 0) {
                            if(clientManager.isNicknameSet(sock)) {
                                 //implement here Parameters: <user> <mode> <unused> <realname>
                                 if(camelCaseMsg != "UsEr" && camelCaseMsg != "UsEr ") {
                                     std::istringstream readin(receivedMsg.substr(5));
                                    std::string username;
                                    std::string mode;
                                    std::string unused;
                                    std::string realname;
                                    std::string whitespace = " ";
                                 if (readin >> username >> mode >> unused >> std::ws && std::getline(readin, realname)) {
                                    if(clientManager.isUsernameTaken(username)) {
                                        std::string serverResponse = "462 ERR_ALREADYREGISTRED \n:Unauthorized command (already registered)";
                                        send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }
                                    else {
                                    
                                    if(mode == "*" || std::isdigit(mode[0])) {
                                       
                                        if(realname[0] != ':' || realname.empty()) {
                                            std::string serverResponse = "Error in realname, must start with : and be first + last name";
                                            send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                        }
                                        else {
                                            //if not a digit and not *
                                            if(mode != "*" && !(std::isdigit(mode[0]))) {
                                                std::string serverResponse = "501 (ERR_UMODEUNKNOWNFLAG) \n:Unknown user mode flag.";
                                                send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                            }
                            
                                            else if(mode == "*") {
                                
                                                //set username
                                            clientManager.setClientUsername(sock, username);
                                            //set realname w/o ":"
                                            clientManager.setRealName(sock, realname.substr(1));
                                            //registered success code
                                            std::string serverResponse = "001 RPL_WELCOME\nWelcome to the Internet Relay Network\n" + clientManager.getClientNickname(sock) + "!" + clientManager.getClientUsername(sock) + "@" +std::string(hostname);
                                            send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                            }
                                            else if(std::stoi(mode) == 0) {
                                
                                                //set username
                                            clientManager.setClientUsername(sock, username);
                                            //set realname w/o :
                                            clientManager.setRealName(sock, realname.substr(1));
                                            //registered success code
                                            std::string serverResponse = "001 RPL_WELCOME\nWelcome to the Internet Relay Network\n" + clientManager.getClientNickname(sock) + "!" + clientManager.getClientUsername(sock) + "@" +std::string(hostname);
                                            send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                            }
                                            else if(std::stoi(mode) == 4) {
                                                //set MODE HERE
                                                clientManager.setMode(sock, 'w');
                                                //set username
                                            clientManager.setClientUsername(sock, username);
                                            //set realname w/o :
                                            clientManager.setRealName(sock, realname.substr(1));
                                            //registered success code
                                            std::string serverResponse = "001 RPL_WELCOME\nWelcome to the Internet Relay Network\n" + clientManager.getClientNickname(sock) + "!" + clientManager.getClientUsername(sock) + "@" +std::string(hostname);
                                            send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                            }
                                            else if(std::stoi(mode) == 8) {
                                                //set MODE HERE
                                                clientManager.setMode(sock, 'i');
                                                //set username
                                            clientManager.setClientUsername(sock, username);
                                            //set realname w/o :
                                            clientManager.setRealName(sock, realname.substr(1));
                                            //registered success code
                                            std::string serverResponse = "001 RPL_WELCOME\nWelcome to the Internet Relay Network\n" + clientManager.getClientNickname(sock) + "!" + clientManager.getClientUsername(sock) + "@" +std::string(hostname);
                                            send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                            }
                                            else {
                                                std::string serverResponse = "501 (ERR_UMODEUNKNOWNFLAG) \n:Unknown user mode flag.";
                                                send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                            }
                                            
                                        }

                                    }
                                    else {
                                        std::string serverResponse = "501 (ERR_UMODEUNKNOWNFLAG) \n:Unknown user mode flag.";
                                        send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }
                                    
                                    }
                                }
                                else {
                                    std::string serverResponse = "461 ERR_NEEDMOREPARAMS \n<USER> :Not enough parameters";
                                    send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                }

                                    
                                 }
                                 else {
                                    std::string serverResponse = "461 ERR_NEEDMOREPARAMS \n<USER> :Not enough parameters";
                                    send(sock, serverResponse.c_str(), serverResponse.size(), 0); 
                                 }
                            }
                            else {
                                std::string serverResponse = "Please use NICK command before USER command";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0); 
                            }
                        }

//HANDLE MODE COMMAND
                    if (camelCaseMsg.find("MoDe") == 0) {
                        if(camelCaseMsg != "MoDe" && camelCaseMsg != "MoDe ") {
                            std::istringstream readin(receivedMsg.substr(5));
                            std::string nickname;
                            std::string mode;
                            if (readin >> nickname) {
                            if(nickname[0] != '#') {
                            if(nickname == clientManager.getClientNickname(sock)) {
                                 if (readin >> mode) {
                               if(mode[0] == '-' || '+') {
                                    if(mode[0] == '+') {
                                    if(mode[1] == 'a' || mode[1] == 'i' || mode[1] == 'w') {
                                        clientManager.setMode(sock, mode[1]);
                                        std::string serverResponse = "User mode changed successfully";
                                    send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }
                                    else {
                                std::string serverResponse = "501 (ERR_UMODEUNKNOWNFLAG) \n:Unknown user mode flag.";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }

                                    }
                                    else {
                                        if(mode[1] == clientManager.getClientMode(sock)) {
                                        clientManager.setMode(sock, 'f');
                                        std::string serverResponse = "User mode removed successfully";
                                    send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }
                                    else {
                                std::string serverResponse = "501 (ERR_UMODEUNKNOWNFLAG) \n:Unknown user mode flag.";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }
                                    }
                                    
                               }
                                else {
                                    std::string serverResponse = "Mode must begin with +/-";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0); 
                                }

                                 }

                                 else {
                                    std::string serverResponse = "221 RPL_UMODEIS\nCurrent user mode is: ";
                                    char usermode = clientManager.getClientMode(sock);
                                    if(usermode == 'f') {
                                        std::string usermodewarn = "Not Set";
                                        serverResponse += usermodewarn;
                                    }
                                    else {
                                        serverResponse += usermode;
                                    }
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0); 
                                 }

                            }
                            else {
                                 std::string serverResponse = "502 ERR_USERSDONTMATCH \nCannot change mode for other users";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0); 
                            }

                            }
                            else {  //IF GIVEN NAME IS A CHANNEL(starts w #)
                                std::istringstream readin(receivedMsg.substr(5));
                                std::string givenChannel;
                                readin >> givenChannel;
                                std::string camelChannel = toCamelCase(givenChannel);
                                if(clientManager.doesChannelExist(camelChannel)) {
                                    if (readin >> mode) {
                                       
                                        if(mode[0] == '-' || '+') {
                                    if(mode[0] == '+') {
                                    if(mode[1] == 'a' || mode[1] == 's' || mode[1] == 'p') {
                                        std::string s(1, mode[1]);
                                        clientManager.changeChannelMode(camelChannel, s);
                                        std::string serverResponse = "Channel mode changed successfully";
                                    send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }
                                    else {
                                std::string serverResponse = "501 (ERR_UMODEUNKNOWNFLAG) \n:Unknown user mode flag.";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }

                                    }
                                    else {
                                        std::string s(1, mode[1]);
                                        if(s == clientManager.getSavedChannelMode(camelChannel)) {
                                        std::string baseMode = "None ";
                                        clientManager.changeChannelMode(camelChannel, baseMode);
                                        std::string serverResponse = "Mode removed successfully";
                                    send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }
                                    else {
                                std::string serverResponse = "501 (ERR_UMODEUNKNOWNFLAG) \n:Unknown user mode flag.";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }
                                    }
                                    
                               }
                                    }
                                    else {
                                        std::string serverResponse = "324 RPL_CHANNELMODEIS\n" + givenChannel + ": " + clientManager.getSavedChannelMode(camelChannel);
                                        send(sock, serverResponse.c_str(), serverResponse.size(), 0);
              
                                    }
                                }
                                else {
                                    std::string serverResponse = "Channel doesnt exist";
                                    send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                }
                            }

                            }
                            else {
                                std::string serverResponse = "461 ERR_NEEDMOREPARAMS \n<MODE> :Not enough parameters";
                                    send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                            }
                        }
                        else {
                            std::string serverResponse = "461 ERR_NEEDMOREPARAMS \n<MODE> :Not enough parameters";
                            send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                        }
                    }

//HANDLE QUIT COMMAND
                        if (camelCaseMsg.find("QuIt") == 0) {
                            std::string serverResponse;
                            if(camelCaseMsg != "QuIt" && camelCaseMsg != "QuIt ") {
                                std::string exitMsg;
                                std::istringstream readin(receivedMsg.substr(5));
                                std::getline(readin, exitMsg);
                                std::string nickname = clientManager.getClientNickname(sock);
                                serverResponse = nickname + " has quit IRC server\n" + "\""+ exitMsg + "\"";
                                
                            }

                            else {

                                
                                std::string nickname = clientManager.getClientNickname(sock);
                                serverResponse = nickname + " has quit IRC server";
                                
                                
                            }

                            
                            clientManager.removeClient(sock);
                            send(sock, serverResponse.c_str(), serverResponse.size(), 0); 
                            close(sock);
                            FD_CLR(sock, &socket_list);
                            
                                // IMPLEMENT KEEP USER/REALNAME-> try to add it back from map when a user reconnects, when disconnected, still registered
                        }

//JOIN CHANNEL COMMAND
                        if(camelCaseMsg.find("JoIn") == 0) {

                            std::string serverResponse;

                            if(camelCaseMsg != "JoIn" && camelCaseMsg != "JoIn ")  {
                                std::string channelname = camelCaseMsg.substr(5);
                              if(clientManager.isNicknameSet(sock)) {
                                if(clientManager.addClientToChannel(sock, channelname)){
                                    serverResponse = "332 RPL_TOPIC\n" + clientManager.getClientNickname(sock) + " " + clientManager.getSavedChannelName(channelname) + ": " + clientManager.getSavedChannelTopic(channelname) + "\nYou are connected!\n" + "Connected Users: ";
                                    std::vector<std::string> clientList = clientManager.getClientsInChannel(channelname);
                                    for(std::string nickname : clientList) {
                                        serverResponse += nickname + " ";
                                    }

                                    std::vector<std::string> nicknames = clientManager.getClientsInChannelWithInvisible(channelname);
                                    std::string nickname = clientManager.getClientNickname(sock);
                                    std::string leavingAlert = nickname + "!" + hostname + "\nJOIN " + receivedMsg.substr(5);
                                    for(std::string name : nicknames) {
                                        int channelMemberSock = clientManager.getClientSocketNumber(name);

                                       if(clientManager.getClientNickname(channelMemberSock) != nickname) {
                                        send(channelMemberSock, leavingAlert.c_str(), leavingAlert.size(), 0);  }

                                    }
                                    
                                }
                                else {
                                    serverResponse = "403 (ERR_NOSUCHCHANNEL): No such channel exists";
                                    
                                }
                            }
                            else {
                                serverResponse = "Need to use NICK command before joining a channel";
                            }
                            }
                            else {
                                serverResponse = "461 ERR_NEEDMOREPARAMS \n<JOIN> :Not enough parameters";
                            }

                            send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                        }

//HANDLE PART COMMAND
                         if(camelCaseMsg.find("PaRt") == 0) {

                            std::string serverResponse;

                            if(camelCaseMsg != "PaRt" && camelCaseMsg != "PaRt ")  {
                                std::istringstream getChannelName(camelCaseMsg.substr(5));
                                std::string channelname;
                                getChannelName >> channelname;

                                 if(clientManager.removeClientFromChannel(sock, channelname)){
                                    serverResponse = "You have left the channel";

                                    //ADD PARTING MESSAGE
                                    std::vector<std::string> nicknames = clientManager.getClientsInChannelWithInvisible(channelname);
                                    std::string nickname = clientManager.getClientNickname(sock);
                                    std::istringstream getPartMsg(receivedMsg.substr(5));
                                    std::string ch;
                                    std::string partMsg;
                                    getPartMsg >> ch;
                                    if(getPartMsg >> partMsg) {
                                    std::string leavingAlert = nickname + "!" + hostname + "\nPART " + receivedMsg.substr(5);
                                    for(std::string name : nicknames) {
                                        int channelMemberSock = clientManager.getClientSocketNumber(name);

                                        send(channelMemberSock, leavingAlert.c_str(), leavingAlert.size(), 0);

                                    }
                                    }
                                    else {
                                      std::string leavingAlert = nickname + "!" + hostname + "\nPART " + receivedMsg.substr(5);
                                    for(std::string name : nicknames) {
                                        int channelMemberSock = clientManager.getClientSocketNumber(name);

                                        send(channelMemberSock, leavingAlert.c_str(), leavingAlert.size(), 0);

                                    }  
                                    }
                                    
                                    
                                }
                                else {
                                    serverResponse = "You are not apart of this channel";
                                    
                                }

                            }

                            else {
                                serverResponse = "461 ERR_NEEDMOREPARAMS \n<PART> :Not enough parameters";
                            }

                            send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                        }

//HANDLE TOPIC COMMAND
                    if (camelCaseMsg.find("ToPiC") == 0) {
                        std::string serverResponse;
                        if (camelCaseMsg != "ToPiC" && camelCaseMsg != "ToPiC ") {
                         std::istringstream readin(receivedMsg.substr(6));
                         std::string channelName;
                        std::string topicChange;

                        
                     readin >> channelName;
                     std::string fixedChannelName = toCamelCase(channelName);

        
                     if (clientManager.getSavedChannelName(fixedChannelName) != "not found") {
            
                         if (readin >> topicChange) {
                
                              if (topicChange[0] == ':') {
                    
                              std::string newTopicMessage = topicChange.substr(1);

                    
                              if (newTopicMessage.empty()) {
                                std::string empty = "(topic not set)";
                                 clientManager.changeChannelTopic(fixedChannelName, empty);
                                  serverResponse = "Topic set to empty";
                               } else {
                                    std::string word;
                                 while (readin >> word) {
                                      newTopicMessage += " " + word;
                                 }
                               
                               
                              clientManager.changeChannelTopic(fixedChannelName, newTopicMessage);
                               serverResponse = "Topic changed to: " + newTopicMessage;
                            }
                        } else {
                            serverResponse = "Topic must start with ':'";
                       }
                 } else {
                
                        serverResponse = "332 RPL_TOPIC\n" + channelName + ": " + clientManager.getSavedChannelTopic(fixedChannelName);
                   }
             } else {
                    serverResponse = "403 ERR_NOSUCHCHANNEL\n" + channelName + ": No such channel";
              }

                  send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                  } else {
                    serverResponse = "461 ERR_NEEDMOREPARAMS\n<TOPIC> :Not enough parameters";
                 send(sock, serverResponse.c_str(), serverResponse.size(), 0);
               }
            }


//HANDLE LIST COMMAND
                if(camelCaseMsg.find("LiSt") == 0) {
                    std::string serverResponse;
                    if(camelCaseMsg == "LiSt" || camelCaseMsg == "LiSt ") {
                        std::string channelList = clientManager.listChannels();
                        serverResponse = channelList;

                        
                    }
                    else {
                        std::string listedChannel = camelCaseMsg.substr(5);
                        if(clientManager.listSpecificChannel(listedChannel) != "Channel not found") {
                            serverResponse = clientManager.listSpecificChannel(listedChannel);
                        }
                        else {
                            serverResponse = "403 ERR_NOSUCHCHANNEL\n" + receivedMsg.substr(5) + " : No such channel";
                        }

                    }


                    send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                }

//HANDLE NAMES COMMAND
                if(camelCaseMsg.find("NaMeS") == 0) {
                    std::string serverResponse;
                    if(camelCaseMsg == "NaMeS" || camelCaseMsg == "NaMeS ") {
                        serverResponse = "461 ERR_NEEDMOREPARAMS\n<NAMES> :Not enough parameters";
                    }
                    else {
                        
                        std::string givenChannel = receivedMsg.substr(6);
                        std::string camelChannel = toCamelCase(givenChannel);
                        
                        

                        if(clientManager.doesChannelExist(camelChannel)) {
                            std::vector<std::string> clientList = clientManager.getClientsInChannel(camelChannel);
                            std::string mode = clientManager.getSavedChannelMode(camelChannel);
                            if(mode == "s") {
                                serverResponse = "353 RPL_NAMREPLY\n@ " + givenChannel + "\n";
                            }
                            else if(mode == "p") {
                                serverResponse = "353 RPL_NAMREPLY\n* " + givenChannel + "\n";
                            }
                            else {
                               serverResponse = "353 RPL_NAMREPLY\n= " + givenChannel + "\n"; 
                            }
                            for(std::string name : clientList) {
                                serverResponse += "@" + name + " ";
                            }
                            serverResponse += "\n366 RPL_ENDOFNAMES\n" + givenChannel + " :End of names list";
                        }
                        else {
                            serverResponse = "403 ERR_NOSUCHCHANNEL\n" + givenChannel + " : No such channel";
                        }
                        

                    }


                    send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                }
//HANDLE PRIVMSG COMMAND

                if(camelCaseMsg.find("PrIvMsG") == 0) {
                    std::string serverResponse;
                    if(camelCaseMsg == "PrIvMsG" || camelCaseMsg == "PrIvMsG ") {
                        serverResponse = "461 ERR_NEEDMOREPARAMS\n<NAMES> :Not enough parameters";
                        send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                    }
                    else {
                        std::istringstream readin(receivedMsg.substr(8));
                         std::string username;
                        std::string intendedMsg;

                     readin >> username;
                     if(username[0] != '#') {
                        if(clientManager.isNicknameTaken(username)) {
                            if(clientManager.getDifferentClientsMode(username) != 'a') {
                            if(readin >> intendedMsg) {
                                
                                    
                                    
                                    serverResponse = ":" + clientManager.getClientNickname(sock) + "!" + hostname + "\n" + receivedMsg;
                                    
                                    int receipentSocketNum = clientManager.getClientSocketNumber(username);
                                    send(receipentSocketNum, serverResponse.c_str(), serverResponse.size(), 0);
                                    
                                    
                                
                            }
                            else{
                                serverResponse = "No message given";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                            }
                            }
                            else {
                                serverResponse = "Client you are trying to message is currently away";
                                send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                            }
                        }
                        else {
                            serverResponse = "401 ERR_NOSUCHNICK\n" + username + " :No such nick/channel";
                            send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                        }

                     }

                        //PRIVMSG To channel, check if priv or secret, must be a member of channel, also cant send to away users
                     else {
                         std::istringstream readin(receivedMsg.substr(8));
                                std::string givenChannel;
                                std::string givenMsg;
                                std::string nickname = clientManager.getClientNickname(sock);
                                readin >> givenChannel;
                                std::string camelChannel = toCamelCase(givenChannel);
                                 if(clientManager.doesChannelExist(camelChannel)) {
                                    if((clientManager.getSavedChannelMode(camelChannel) == "p" || clientManager.getSavedChannelMode(camelChannel) == "s") && !(clientManager.isClientinChannel(nickname, camelChannel))) {
                                        serverResponse = "Cannot message a private/secret channel if not apart of it";
                                     send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }
                                    else {
                                    if (readin >> givenMsg) {
                                        std::vector<std::string> nicknames = clientManager.getClientsInChannelWithInvisible(camelChannel);
                                        std::string nickname = clientManager.getClientNickname(sock);
                                    std::string privMsgToChannel = ":" + nickname + "!" + hostname + "\n " + receivedMsg;
                                    for(std::string name : nicknames) {
                                        int channelMemberSock = clientManager.getClientSocketNumber(name);
                                        if(clientManager.getClientMode(channelMemberSock) != 'a' && clientManager.getClientSocketNumber(nickname) != channelMemberSock) {
                                        send(channelMemberSock, privMsgToChannel.c_str(), privMsgToChannel.size(), 0); }

                                    }


                                    }
                                    else {
                                        serverResponse = "No message given";
                                     send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                    }

                                    }
                                 }
                                 else {
                                    serverResponse = "403 ERR_NOSUCHCHANNEL\n" + username + " :No such channel";
                                    send(sock, serverResponse.c_str(), serverResponse.size(), 0);
                                 }
                     }

                    }


                    
                }



                //check UDP
                if (camelCaseMsg == "ChEcK") {
    
        
        int UDP = clientManager.getClientUDPPort(sock);
        std::string serverResponse = "Heartbeat: " + heartbeatInterval;
        send(sock, serverResponse.c_str(), serverResponse.size(), 0);

    }

   if (camelCaseMsg == "HeArTbEaT") {
    struct sockaddr_in client_udp_addr;
    // Create a UDP socket
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        perror("Failed to create UDP socket");
       
    }

    memset(&client_udp_addr, 0, sizeof(client_udp_addr));
  

    // Calculate the client's UDP port based on the client’s TCP port + offset
    int clientUDPPort = clientManager.getClientUDPPort(sock);  // Retrieve the client's TCP port
    std::cout << "clientUDPPort is: " << clientUDPPort;
    std::cout << "client UDP Socket is: " << udpSocket;
    client_udp_addr.sin_family = AF_INET;
    client_udp_addr.sin_port = htons(clientUDPPort);
    client_udp_addr.sin_addr.s_addr = inet_addr(s); 


     std::string udpMessage = "Hello active user! This is your heartbeat message:\nConnected Users List: ";
    std::vector<int> allClients = clientManager.getClientSocketNumbers();
    for(int client: allClients) {
        if(clientManager.getClientMode(client) != 'a') {
       std::string nickname = clientManager.getClientNickname(client);
       if(nickname == "") {
        nickname = "(User w/o set nickname)";
        udpMessage += nickname + " ";
       }
       else {
        udpMessage += nickname + " ";
       }
        }
    }
    

    // Attempt to send the message over UDP
    ssize_t sentBytes = sendto(udpSocket, udpMessage.c_str(), udpMessage.size(), 0,
                               (struct sockaddr*)&client_udp_addr, sizeof(client_udp_addr));
    if (sentBytes == -1) {
    perror("Failed to send UDP message");
} else {
    std::cout << "Sent " << sentBytes << " bytes to client on UDP port " << clientUDPPort << std::endl;
}

    
    close(udpSocket);
    }


 if (camelCaseMsg == "StAtIsTiCs") {
    
    struct sockaddr_in client_udp_addr;
    // Create a UDP socket
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        perror("Failed to create UDP socket");
       
    }

    memset(&client_udp_addr, 0, sizeof(client_udp_addr));
  

    // Calculate the client's UDP port based on the client’s TCP port + offset
    int clientUDPPort = clientManager.getClientUDPPort(sock);  // Retrieve the client's TCP port
    std::cout << "clientUDPPort is: " << clientUDPPort;
    std::cout << "client UDP Socket is: " << udpSocket;
    client_udp_addr.sin_family = AF_INET;
    client_udp_addr.sin_port = htons(clientUDPPort);
    client_udp_addr.sin_addr.s_addr = inet_addr(s); 


     std::string udpMessage = "Hello active user! This is your server statistics message:\nNumber of Active Users:\n";
    
    int userCount = clientManager.getActiveUsers();
   

    udpMessage += std::to_string(userCount) + "\nNumber of Active Channels(Have users in them):\n";
    udpMessage += clientManager.listActiveChannels() + "\n";

    struct sysinfo sys_info;
        if (sysinfo(&sys_info) == 0) {
            udpMessage += "CPU Usage: " + std::to_string(sys_info.loads[0] / 65536.0) + "%\n"; }
            
    

    // Attempt to send the message over UDP
    ssize_t sentBytes = sendto(udpSocket, udpMessage.c_str(), udpMessage.size(), 0,
                               (struct sockaddr*)&client_udp_addr, sizeof(client_udp_addr));
    if (sentBytes == -1) {
    perror("Failed to send UDP message");
} else {
    std::cout << "Sent " << sentBytes << " bytes to client on UDP port " << clientUDPPort << std::endl;
}

    
    close(udpSocket);
    }
   
                






                        
       }



                }
            }
        }
    }

    return 0;
}

                           
