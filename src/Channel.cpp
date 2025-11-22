#include "Channel.hpp"
#include <algorithm> // for std::find

Channel::Channel(const std::string& name, Client* creator)
    : _name(name),
      _topic(""),
      _key(""),
      _inviteOnly(false),
      _topicRestricted(true),
      _userLimit(0) {
    _clients.push_back(creator);
    _operators.push_back(creator);
}

Channel::~Channel() {}

// --- Basic Info ---
const std::string& Channel::getName() const { return _name; }
const std::string& Channel::getTopic() const { return _topic; }
void Channel::setTopic(const std::string& topic) { _topic = topic; }

// --- Member Management ---
bool Channel::addClient(Client* client) {
    if (isClientInChannel(client)) {
        return false; // Already in channel
    }
    _clients.push_back(client);
    return true;
}

void Channel::removeClient(Client* client) {
    removeOperator(client); // Ensure they are removed from operator list as well
    removeInvite(client);   // And from invited list

    std::vector<Client*>::iterator it = std::find(_clients.begin(), _clients.end(), client);
    if (it != _clients.end()) {
        _clients.erase(it);
    }
}


bool Channel::isClientInChannel(Client* client) const {
    return std::find(_clients.begin(), _clients.end(), client) != _clients.end();
}

std::vector<Client*> Channel::getClients() const {
    return _clients;
}


// --- Operator Management ---
bool Channel::isOperator(Client* client) const {
    return std::find(_operators.begin(), _operators.end(), client) != _operators.end();
}

void Channel::addOperator(Client* client) {
    if (isClientInChannel(client) && !isOperator(client)) {
        _operators.push_back(client);
    }
}

void Channel::removeOperator(Client* client) {
    std::vector<Client*>::iterator it = std::find(_operators.begin(), _operators.end(), client);
    if (it != _operators.end()) {
        _operators.erase(it);
    }
}

// --- Mode Management ---
bool Channel::getMode(char mode) const {
    switch (mode) {
        case 'i': return _inviteOnly;
        case 't': return _topicRestricted;
        case 'k': return !_key.empty();
        case 'l': return _userLimit > 0;
        default: return false;
    }
}

void Channel::setMode(char mode, bool value) {
    switch (mode) {
        case 'i': _inviteOnly = value; break;
        case 't': _topicRestricted = value; break;
    }
}

const std::string& Channel::getKey() const { return _key; }
void Channel::setKey(const std::string& key) { _key = key; }
int Channel::getUserLimit() const { return _userLimit; }
void Channel::setUserLimit(int limit) { _userLimit = limit; }

std::string Channel::getModesString() const {
    std::string modes = "+";
    if (_inviteOnly) modes += 'i';
    if (_topicRestricted) modes += 't';
    if (!_key.empty()) modes += 'k';
    if (_userLimit > 0) modes += 'l';
    if (modes == "+") return "";
    return modes;
}

// --- Invite Management ---
void Channel::addInvite(Client* client) {
    if (!isInvited(client)) {
        _invitedUsers.push_back(client);
    }
}

bool Channel::isInvited(Client* client) {
    return std::find(_invitedUsers.begin(), _invitedUsers.end(), client) != _invitedUsers.end();
}

void Channel::removeInvite(Client* client) {
    std::vector<Client*>::iterator it = std::find(_invitedUsers.begin(), _invitedUsers.end(), client);
    if (it != _invitedUsers.end()) {
        _invitedUsers.erase(it);
    }
}
