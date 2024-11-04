#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <sstream>

class Client {
public:
    int socketnum;
    int udpsocketnum;
    int udpPort;
    std::string nickname;
    std::string username;
    std::string realname;
    bool nickIsSet = false;
    bool usernameIsSet = false;
    char userMode = 'f';


    Client() {
        this->socketnum = -1;
        this->nickname = "(No Nickname Set)";

    }

    
    Client(int sockfd, const std::string& nick = "") {
        this->socketnum = sockfd;
        this->nickname = nick;
    }

        
    
    // Method to set nickname
    void setNickname(const std::string& nick) {
        this->nickname = nick;
        
    }

    void setUsername(const std::string& name) {
        this->username = name;
    }

    void setRealName(const std::string& name) {
        this->realname = name;
    }

    void setUDPPort(int tcpPort) {
        this->udpPort = tcpPort + 100;
    }

    void setUDPSockNum(int udpsock) {
        this->udpsocketnum = udpsock;
    }

    int getUDPSocketNumber() const {
        return this->udpsocketnum;
    }

    int getUDPPort() {
        return this->udpPort;
    }

    void setMode(char mode) {
        this->userMode= mode;
    }

    char getMode() const {
        return this->userMode;
    }

    int getSocketNumber() const {
        return this->socketnum;
    }

    // Method to retrieve nickname
    std::string getNickname() const {
        return this->nickname;
    }

    std::string getUsername() const {
        return this->username;
    }
};

class Channel {
    public:
    std::string channelName;
    std::string channelFlag;
    std::string channelTopic;
    std::set<int> channelClientSockets;  

    Channel() {
        this->channelName = "";
        this->channelFlag = "";
    }

    Channel(std::string name, std::string flag, std::string topic) {
        this->channelName = name;
        this->channelFlag = flag;
        this->channelTopic = topic;
    }

    void addClientToChannel(int sock) {
        channelClientSockets.insert(sock);
    }

    bool removeClientFromChannel(int sock) {
         if(channelClientSockets.find(sock) != channelClientSockets.end()) {
        channelClientSockets.erase(sock); 
        return true; }
        else {
            return false;
        }
    }

    bool isClientInChannel(int sock) {
        if(channelClientSockets.find(sock) != channelClientSockets.end()) {
            return true;
        }
        return false;
    }

    std::vector<int> returnClientSocketList() {
        std::vector <int> socknums;
        for(int sock: channelClientSockets) {
            socknums.push_back(sock);
        }
        return socknums;
    }

    std::string getChannelName() {
        return this->channelName;
    }

    std::string getChannelMode() {
        return this->channelFlag;
    }

    std::string getChannelTopic() {
        return this->channelTopic;
    }

    void setChannelTopic(std::string topic) {
        this->channelTopic = topic;
    }

    void setChannelMode(std::string mode) {
        this->channelFlag = mode;
    }

    



};


class ClientManager {
public:
    

    std::map<int, Client> clients;
    std::map<std::string, Channel> channels;

    ClientManager() {
        setChannelsUp();
    }

    void setChannelsUp() {
        channels["#GeNeRaL"] = Channel("#general", "None", "An open channel with no flags");
        channels["#AnOnYmOuS"] = Channel("#anonymous", "a", "An open channel with anonymous flag");
        channels["#PrIvAtE"] = Channel("#private", "p", "A private channel");
        channels["#SeCrEt"] = Channel("#secret", "s", "A secret channel");

    }
    

    void addClient(int sock) {
        clients[sock] = Client(sock);  // Insert a new client with the socket number
        
    }

    void setClientNickname(int sock, const std::string& nickname) {
        if (clients.find(sock) != clients.end()) {
            clients[sock].setNickname(nickname);
            clients[sock].nickIsSet = true;
        }
    }

    void setClientUsername(int sock, const std::string& name) {
        if (clients.find(sock) != clients.end()) {
            clients[sock].setUsername(name);
            clients[sock].usernameIsSet = true;
        }
    }

    void setRealName(int sock, const std::string& name) {
        if (clients.find(sock) != clients.end()) {
            clients[sock].setRealName(name);
            
        }
    }

    void setMode(int sock, char mode) {
        if (clients.find(sock) != clients.end()) {
            clients[sock].setMode(mode);
            
        }
    }

    char getClientMode(int sock) {
        if (clients.find(sock) != clients.end()) {
            return clients[sock].getMode();
        }
        else {
            return 'f';
        }
    }

    char getDifferentClientsMode(std::string& nickname) {
        for(const auto& pair : clients) {
            Client client = pair.second;
            if(client.getNickname() == nickname) {
                return client.getMode();
            }
        }
        return 'f';
    }

    std::string getClientNickname(int sock) {
        if (clients.find(sock) != clients.end()) {
            return clients[sock].getNickname();
        }
        return "Not set yet";  // Return empty string if client not found
    }

    std::string getClientUsername(int sock) {
        if (clients.find(sock) != clients.end()) {
            return clients[sock].getUsername();
        }
        return "Not set yet";  // Return empty string if client not found
    }

    bool isNicknameTaken(const std::string& nick) const {
        for (const auto& pair : clients) {
            if (pair.second.getNickname() == nick) {
                return true;
            }
        }
        return false;
    }

    bool isUsernameTaken(const std::string& name) const {
        for (const auto& pair : clients) {
            if (pair.second.getUsername() == name) {
                return true;
            }
        }
        return false;
    }

    bool isNicknameSet(int sock) {
        if(clients.find(sock) != clients.end()) {
            return clients[sock].nickIsSet;
        }
        return false;
    }

    int getClientCount()  {
        return clients.size();
    }

   std::vector<int> getClientSocketNumbers() {
    std::vector <int> socknums;
    for (const auto& pair : clients) {
           socknums.push_back(pair.second.socketnum);
        }
        return socknums;
   }

   int getActiveUsers() {
    int count = 0;
    for (const auto& pair : clients) {
           if(pair.second.getMode() != 'a') {
                ++count;
           }
        }
        return count;
   }

   std::vector<int> getClientUDPPortNumbers() {
    std::vector <int> portnums;
    for (const auto& pair : clients) {
            if(pair.second.getMode() != 'a') {
           portnums.push_back(pair.second.udpPort); }
        }
        return portnums;
   }

   int getClientSocketNumber(std::string username) {
        int socketnum;
        for(const auto& pair : clients) {
            if(pair.second.getNickname() == username) {
                return pair.second.getSocketNumber();
            }
        }
        return 1000;
   }

   void removeClient(int sock) {
        if (clients.find(sock) != clients.end()) {
            clients[sock].setNickname("(No Nickname Set)");
            
        }
    }

    bool addClientToChannel(int sock, std::string& channelname) {
        if(channels.find(channelname) != channels.end()) {
            channels[channelname].addClientToChannel(sock);
            return true;
        }
        else{
           return false; 
        } 
    }

    bool removeClientFromChannel(int sock, std::string& channelname) {
        if (channels.find(channelname) != channels.end()) {
        return channels[channelname].removeClientFromChannel(sock);
    } else {
        return false; 
    }
    }

    bool doesChannelExist(std::string& channelname) {
        if (channels.find(channelname) != channels.end()) {
            return true;
        }
        else {
            return false;
        }
    }

    std::vector<std::string> getClientsInChannel(std::string& channelname) {
        std::vector<std::string> nicknames;
        std::vector<int> clientSockets = channels[channelname].returnClientSocketList();
         for (int sock : clientSockets) {
            if (clients.find(sock) != clients.end()) {
                if(clients[sock].getMode() != 'i') {
                nicknames.push_back(clients[sock].getNickname()); }
            }
        }
      
        return nicknames;


    }

    std::vector<std::string> getClientsInChannelWithInvisible(std::string& channelname) {
        std::vector<std::string> nicknames;
        std::vector<int> clientSockets = channels[channelname].returnClientSocketList();
         for (int sock : clientSockets) {
            if (clients.find(sock) != clients.end()) {
                nicknames.push_back(clients[sock].getNickname()); 
            }
        }
      
        return nicknames;


    }

    int getNumClientsInChannel(std::string& channelname) {
        int count = 0;
        std::vector<int> clientSockets = channels[channelname].returnClientSocketList();
         for (int sock : clientSockets) {
            if (clients.find(sock) != clients.end()) {
                if(clients[sock].getMode() != 'i') {
                ++count; }
            }
        }
      
        return count;


    }

    bool isClientinChannel(std::string& nickname, std::string& channelname) {

        std::vector<int> clientSockets = channels[channelname].returnClientSocketList();
        for(int sock : clientSockets) {
            if (clients.find(sock) != clients.end()) { 
                if(clients[sock].getNickname() == nickname) {
                    return true;
                }
        }

    }
    return false;
    }

    std::string getSavedChannelName(std::string& channelname) {
        if(channels.find(channelname) != channels.end()) {
            return channels[channelname].getChannelName();
        }
        else {
            return "not found";
        }
    }

    std::string getSavedChannelTopic(std::string& channelname) {
        if(channels.find(channelname) != channels.end()) {
            return channels[channelname].getChannelTopic();
        }
        else {
            return ": No Topic Found";
        }
    }

    std::string getSavedChannelMode(std::string& channelname) {
        if(channels.find(channelname) != channels.end()) {
            return channels[channelname].getChannelMode();
        }
        else {
            return ": No Mode Set";
        }
    }

    bool changeChannelTopic(std::string& channelname, std::string& topic) {
        if(channels.find(channelname) != channels.end()) {
            channels[channelname].setChannelTopic(topic);
            return true;
        }
        else {
            return false;
        }
    }

    std::string listChannels() {
        std::stringstream listMessage;
        listMessage << "322 RPL_LIST\n";
        
        for (const auto& pair : channels) {
             Channel channel = pair.second;
            std::string channelName = channel.getChannelName();
            std::string topic = channel.getChannelTopic();
            std::string mode = channel.getChannelMode();
            int userCount = channel.channelClientSockets.size();
            //int userCount = getNumClientsInChannel(channelName);

            if(mode != "s") {
            listMessage  << channelName << " " << userCount << " users" << " :" << topic << "\n"; }
        }
        
        listMessage << "323 RPL_LISTEND :End of LIST";
        return listMessage.str();
    }

    std::string listActiveChannels() {
        std::stringstream listMessage;
        listMessage << "";
        
        
        for (const auto& pair : channels) {
             Channel channel = pair.second;
            std::string channelName = channel.getChannelName();
            std::string topic = channel.getChannelTopic();
            std::string mode = channel.getChannelMode();
            int userCount = channel.channelClientSockets.size();
            //int userCount = getNumClientsInChannel(channelName);

            if(mode != "s" && userCount != 0) {
            listMessage  << channelName << ": " << userCount << " users" << "\n"; }
        }
        
        if(listMessage.str() != "") {
        return listMessage.str(); }
        else {
            return "No channels are active right now...try joining one!";
        }
    }

    std::string listSpecificChannel(std::string& channelname) {
        std::stringstream listMessage;
        listMessage << "322 RPL_LIST\n";
        
        if(channels.find(channelname) != channels.end()) {
            std::string channelName = channels[channelname].getChannelName();
            std::string topic = channels[channelname].getChannelTopic();
            int userCount = channels[channelname].channelClientSockets.size();
            //int userCount = getNumClientsInChannel(channelName);

            listMessage  << channelName << " " << userCount << " users" << " :" << topic << "\n";
        }
        else {
            return "Channel not found";
        }
        
        listMessage << "323 RPL_LISTEND :End of LIST";
        return listMessage.str();
    }
   
    bool changeChannelMode(std::string& channelname, std::string& mode) {
        if(channels.find(channelname) != channels.end()) {
            channels[channelname].setChannelMode(mode);
            return true;
        }
        else {
            return false;
        }
    }

    void setUDPPort(int sock, int tcpPort) {
        if (clients.find(sock) != clients.end()) {
            clients[sock].setUDPPort(tcpPort);
            
        }
    }

    int getClientUDPPort(int sock) {
        if (clients.find(sock) != clients.end()) {
            return clients[sock].getUDPPort();
        }
        return 1000;  
    }

    

};
