#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

enum RegistrationState {
    PASS_NEEDED,
    NICK_USER_NEEDED,
    REGISTERED
};

class Client {
public:
    Client(int fd, const std::string& hostname);
    ~Client();

    // Getters
    int getFd() const;
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    const std::string& getHostname() const;
    RegistrationState getRegistrationState() const;
    const std::string& getBuffer() const;
    std::string& getBufferRef();
    bool isAuthenticated() const;

    // Setters
    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username);
    void setRegistrationState(RegistrationState state);
    void appendToBuffer(const char* data, size_t size);
    void clearBuffer(size_t len);
    void setAuthenticated(bool auth);


private:
    int _fd;
    std::string _nickname;
    std::string _username;
    std::string _realname;
    std::string _hostname;
    RegistrationState _registrationState;
    std::string _buffer; // Buffer for incoming data
    bool _authenticated;

    Client();
    Client(const Client&);
    Client& operator=(const Client&);
};

#endif // CLIENT_HPP