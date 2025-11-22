#include "Server.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <algorithm>

// --- Helper Functions ---
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// --- Constructor/Destructor ---
Server::Server(int port, const std::string& password)
    : _port(port), _password(password), _serverSocket(-1), _serverName("irc.42.fr") {
    _startTime = time(NULL);
}

Server::~Server() {
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        delete it->second;
    }
    if (_serverSocket != -1) {
        close(_serverSocket);
    }
}

// --- Core Server Logic ---
void Server::run() {
    setup();
    mainLoop();
}

void Server::setup() {
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket < 0) throw std::runtime_error("Failed to create socket");

    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("Failed to set socket options");

    if (fcntl(_serverSocket, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("Failed to set socket to non-blocking");

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(_port);

    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
        throw std::runtime_error("Failed to bind socket");

    if (listen(_serverSocket, 10) < 0)
        throw std::runtime_error("Failed to listen on socket");

    struct pollfd pfd;
    pfd.fd = _serverSocket;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);

    std::cout << "Server listening on port " << _port << std::endl;
}

void Server::mainLoop() {
    while (true) {
        if (poll(&_pollfds[0], _pollfds.size(), -1) < 0) {
            throw std::runtime_error("Poll failed");
        }

        if (_pollfds[0].revents & POLLIN) {
            handleNewConnection();
        }

        for (size_t i = _pollfds.size() - 1; i > 0; --i) {
            if (_pollfds[i].revents & POLLIN) {
                handleClientData(_pollfds[i].fd);
            }
             if (_pollfds[i].revents & (POLLHUP | POLLERR)) {
                removeClient(_pollfds[i].fd);
            }
        }
    }
}

// --- Connection Handling ---
void Server::handleNewConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) return;

    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
        close(clientFd);
        return;
    }
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, client_ip, INET_ADDRSTRLEN);

    Client* newClient = new Client(clientFd, std::string(client_ip));
    _clients[clientFd] = newClient;

    struct pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);

    std::cout << "New connection from " << client_ip << " on fd " << clientFd << std::endl;
}

void Server::handleClientData(int clientFd) {
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        removeClient(clientFd);
        return;
    }

    Client* client = _clients[clientFd];
    client->appendToBuffer(buffer, bytesRead);

    std::string& clientBuffer = client->getBufferRef(); // Assuming Client class can return a reference
    size_t pos;
    while ((pos = clientBuffer.find('\n')) != std::string::npos) {
        std::string message = clientBuffer.substr(0, pos);
        
        // If there's a '\r' before the '\n', remove it from the message
        if (!message.empty() && message[message.length() - 1] == '\r') {
            message.erase(message.length() - 1);
        }

        // Update the client's buffer by removing the processed command and the '\n'
        client->clearBuffer(pos + 1);
        
        if (!message.empty()) {
            processCommand(client, message);
        }
        // The loop will continue if there are more commands in the buffer
    }
}


void Server::removeClient(int clientFd) {
    Client* client = _clients[clientFd];
    if (!client) return;

    // Remove from all channels
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        if (it->second->isClientInChannel(client)) {
            it->second->removeClient(client);
             if (it->second->getClients().empty()) {
                delete it->second;
                _channels.erase(it);
            }
        }
    }

    // Remove from pollfds
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].fd == clientFd) {
            _pollfds.erase(_pollfds.begin() + i);
            break;
        }
    }

    std::cout << "Client " << client->getNickname() << " (fd: " << clientFd << ") disconnected." << std::endl;

    close(clientFd);
    delete client;
    _clients.erase(clientFd);
}


// --- Command Processing ---
void Server::processCommand(Client* client, const std::string& message) {
    std::cout << "FD(" << client->getFd() << ") C: " << message << std::endl;
    
    std::string command;
    std::vector<std::string> args;
    std::string trailing;
    
    size_t space_pos = message.find(' ');
    if (space_pos != std::string::npos) {
        command = message.substr(0, space_pos);
        std::string rest = message.substr(space_pos + 1);
        
        size_t colon_pos = rest.find(':');
        if (colon_pos != std::string::npos) {
            trailing = rest.substr(colon_pos + 1);
            rest = rest.substr(0, colon_pos);
        }
        
        std::stringstream ss(rest);
        std::string arg;
        while (ss >> arg) {
            args.push_back(arg);
        }
        if (!trailing.empty()) {
            args.push_back(trailing);
        }
    } else {
        command = message;
    }
    
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    if (command == "PASS") cmdPass(client, args);
    else if (command == "NICK") cmdNick(client, args);
    else if (command == "USER") cmdUser(client, args);
    else if (command == "QUIT") cmdQuit(client, args);
    else if (client->getRegistrationState() != REGISTERED) {
         sendNumericReply(client, "451", ":You have not registered");
         return;
    }
    else if (command == "PRIVMSG") cmdPrivmsg(client, args);
    else if (command == "JOIN") cmdJoin(client, args);
    else if (command == "PART") cmdPart(client, args);
    else if (command == "TOPIC") cmdTopic(client, args);
    else if (command == "KICK") cmdKick(client, args);
    else if (command == "INVITE") cmdInvite(client, args);
    else if (command == "MODE") cmdMode(client, args);
    else {
        sendNumericReply(client, "421", command + " :Unknown command");
    }
}

// --- Command Implementations ---
#include "Commands.cpp" // Splitting implementations into a separate file for clarity

// --- Utility Functions ---
void Server::sendReply(Client* client, const std::string& reply) {
    std::string full_reply = reply + "\r\n";
    std::cout << "FD(" << client->getFd() << ") S: " << full_reply;
    send(client->getFd(), full_reply.c_str(), full_reply.length(), 0);
}

void Server::sendNumericReply(Client* client, const std::string& code, const std::string& message) {
    std::string reply = ":" + _serverName + " " + code + " " + client->getNickname() + " " + message;
    sendReply(client, reply);
}

Client* Server::findClientByNick(const std::string& nick) {
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNickname() == nick) {
            return it->second;
        }
    }
    return NULL;
}