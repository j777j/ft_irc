#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <map>
#include "Client.hpp"

class Channel {
public:
    Channel(const std::string& name, Client* creator);
    ~Channel();

    // Basic Info
    const std::string& getName() const;
    const std::string& getTopic() const;
    void setTopic(const std::string& topic);

    // Member Management
    bool addClient(Client* client);
    void removeClient(Client* client);
    bool isClientInChannel(Client* client) const;
    std::vector<Client*> getClients() const;

    // Operator Management
    bool isOperator(Client* client) const;
    void addOperator(Client* client);
    void removeOperator(Client* client);

    // Mode Management
    bool getMode(char mode) const;
    void setMode(char mode, bool value);
    const std::string& getKey() const;
    void setKey(const std::string& key);
    int getUserLimit() const;
    void setUserLimit(int limit);
    std::string getModesString() const;

    // Invite Management
    void addInvite(Client* client);
    bool isInvited(Client* client);
    void removeInvite(Client* client);


private:
    std::string _name;
    std::string _topic;
    std::string _key; // Password for the channel ('k' mode)
    std::vector<Client*> _clients;
    std::vector<Client*> _operators;
    std::vector<Client*> _invitedUsers; // For +i mode

    // Modes
    bool _inviteOnly; // 'i'
    bool _topicRestricted; // 't'
    int _userLimit; // 'l'

    // Private default constructor
    Channel();
    Channel(const Channel&);
    Channel& operator=(const Channel&);
};

#endif // CHANNEL_HPP
