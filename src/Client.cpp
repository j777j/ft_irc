#include "Client.hpp"

Client::Client(int fd, const std::string& hostname)
    : _fd(fd),
      _hostname(hostname),
      _registrationState(PASS_NEEDED),
      _authenticated(false) {}

Client::~Client() {}

// --- Getters ---
int Client::getFd() const { return _fd; }
const std::string& Client::getNickname() const { return _nickname; }
const std::string& Client::getUsername() const { return _username; }
const std::string& Client::getHostname() const { return _hostname; }
RegistrationState Client::getRegistrationState() const { return _registrationState; }
const std::string& Client::getBuffer() const { return _buffer; }
std::string& Client::getBufferRef() { return _buffer; }
bool Client::isAuthenticated() const { return _authenticated; }


// --- Setters ---
void Client::setNickname(const std::string& nickname) { _nickname = nickname; }
void Client::setUsername(const std::string& username) { _username = username; }
void Client::setRegistrationState(RegistrationState state) { _registrationState = state; }
void Client::appendToBuffer(const char* data, size_t size) { _buffer.append(data, size); }
void Client::clearBuffer(size_t len) { _buffer.erase(0, len); }
void Client::setAuthenticated(bool auth) { _authenticated = auth; }