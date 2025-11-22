#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include "Client.hpp"
#include "Channel.hpp"

class Server {
public:
    Server(int port, const std::string& password);
    ~Server();

    void run();

private:
    // Server Info
    int _port;
    std::string _password;
    int _serverSocket;
    std::string _serverName;
    time_t _startTime;

    // Client/Channel Management
    std::map<int, Client*> _clients;
    std::map<std::string, Channel*> _channels;
    std::vector<struct pollfd> _pollfds;

    // Core Loop
    void setup();
    void mainLoop();
    void handleNewConnection();
    void handleClientData(int clientFd);
    void removeClient(int clientFd);

    // Command Processing
    void processCommand(Client* client, const std::string& message);

    // Command Handlers
    void cmdPass(Client* client, const std::vector<std::string>& args);
    void cmdNick(Client* client, const std::vector<std::string>& args);
    void cmdUser(Client* client, const std::vector<std::string>& args);
    void cmdPrivmsg(Client* client, const std::vector<std::string>& args);
    void cmdJoin(Client* client, const std::vector<std::string>& args);
    void cmdPart(Client* client, const std::vector<std::string>& args);
    void cmdTopic(Client* client, const std::vector<std::string>& args);
    void cmdKick(Client* client, const std::vector<std::string>& args);
    void cmdInvite(Client* client, const std::vector<std::string>& args);
    void cmdMode(Client* client, const std::vector<std::string>& args);
    void cmdQuit(Client* client, const std::vector<std::string>& args);


    // Utility
    void sendReply(Client* client, const std::string& reply);
    void sendNumericReply(Client* client, const std::string& code, const std::string& message);
    Client* findClientByNick(const std::string& nick);

    // Private constructor/assignment
    Server();
    Server(const Server&);
    Server& operator=(const Server&);
};

#endif // SERVER_HPP