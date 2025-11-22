// This file is included at the end of Server.cpp
// It contains the implementation of all IRC command handlers.

// --- Command Handlers ---

void Server::cmdPass(Client* client, const std::vector<std::string>& args) {
    if (client->isAuthenticated()) {
        sendNumericReply(client, "462", ":You may not reregister");
        return;
    }
    if (args.empty()) {
        sendNumericReply(client, "461", "PASS :Not enough parameters");
        return;
    }
    if (args[0] == _password) {
        client->setAuthenticated(true);
        if (client->getRegistrationState() == PASS_NEEDED) {
            client->setRegistrationState(NICK_USER_NEEDED);
        }
    } else {
        sendNumericReply(client, "464", ":Password incorrect");
    }
}

void Server::cmdNick(Client* client, const std::vector<std::string>& args) {
    if (!client->isAuthenticated()) {
        sendNumericReply(client, "451", ":You have not registered (PASSWORD required)");
        return;
    }
    if (args.empty() || args[0].empty()) {
        sendNumericReply(client, "431", ":No nickname given");
        return;
    }
    const std::string& newNick = args[0];
    if (findClientByNick(newNick)) {
        sendNumericReply(client, "433", newNick + " :Nickname is already in use");
        return;
    }
    // Add more validation for nickname characters if needed

    client->setNickname(newNick);
    // Check for registration completion
    if (client->getRegistrationState() == NICK_USER_NEEDED && !client->getUsername().empty()) {
         client->setRegistrationState(REGISTERED);
         sendNumericReply(client, "001", ":Welcome to the IRC Network " + client->getNickname());
    }
}


void Server::cmdUser(Client* client, const std::vector<std::string>& args) {
    if (!client->isAuthenticated()) {
        sendNumericReply(client, "451", ":You have not registered (PASSWORD required)");
        return;
    }
     if (client->getRegistrationState() == REGISTERED) {
        sendNumericReply(client, "462", ":You may not reregister");
        return;
    }
    if (args.size() < 4) {
        sendNumericReply(client, "461", "USER :Not enough parameters");
        return;
    }

    client->setUsername(args[0]);
    
    if (client->getRegistrationState() == NICK_USER_NEEDED && !client->getNickname().empty()) {
        client->setRegistrationState(REGISTERED);
        sendNumericReply(client, "001", ":Welcome to the IRC Network " + client->getNickname());
    }
}

void Server::cmdPrivmsg(Client* client, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        sendNumericReply(client, "461", "PRIVMSG :Not enough parameters");
        return;
    }

    const std::string& target = args[0];
    const std::string& message = args[1];
    
    std::string full_message = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " PRIVMSG " + target + " :" + message;

    if (target[0] == '#') { // To a channel
        std::map<std::string, Channel*>::iterator it = _channels.find(target);
        if (it != _channels.end()) {
            if(it->second->isClientInChannel(client))
            {
                std::vector<Client*> clients = it->second->getClients();
                for (size_t i = 0; i < clients.size(); ++i) {
                    if (clients[i] != client) {
                        sendReply(clients[i], full_message);
                    }
                }
            } else {
                 sendNumericReply(client, "404", target + " :Cannot send to channel");
            }
        } else {
            sendNumericReply(client, "403", target + " :No such channel");
        }
    } else { // To a user
        Client* destClient = findClientByNick(target);
        if (destClient) {
            sendReply(destClient, full_message);
        } else {
            sendNumericReply(client, "401", target + " :No such nick/channel");
        }
    }
}

void Server::cmdJoin(Client* client, const std::vector<std::string>& args) {
    if (args.empty()) {
        sendNumericReply(client, "461", "JOIN :Not enough parameters");
        return;
    }
    const std::string& channelName = args[0];

    if (channelName[0] != '#') {
        sendNumericReply(client, "403", channelName + " :No such channel");
        return;
    }

    Channel* channel;
    bool isNewChannel = false;
    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    if (it == _channels.end()) {
        channel = new Channel(channelName, client);
        _channels[channelName] = channel;
        isNewChannel = true;
    } else {
        channel = it->second;
    }

    // Mode checks for existing channels
    if (!isNewChannel) {
        if (channel->getMode('i')) {
            if (!channel->isInvited(client)) {
                sendNumericReply(client, "473", channelName + " :Cannot join channel (+i)");
                return;
            }
        }
        if (channel->getMode('k') && (args.size() < 2 || args[1] != channel->getKey())) {
             sendNumericReply(client, "475", channelName + " :Cannot join channel (+k)");
            return;
        }
        if (channel->getMode('l') && (int)channel->getClients().size() >= channel->getUserLimit()) {
            sendNumericReply(client, "471", channelName + " :Cannot join channel (+l)");
            return;
        }

        if (!channel->addClient(client)) {
            // This case should ideally not be hit frequently now, but good to keep for robustness
            std::cerr << "Error: Client " << client->getNickname() << " failed to add to existing channel " << channelName << std::endl;
            return;
        }
    }
    
    // removeInvite must be called regardless of new/existing channel
    channel->removeInvite(client); 
    
    std::string join_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " JOIN :" + channelName;
    std::vector<Client*> clients = channel->getClients();
    for (size_t i = 0; i < clients.size(); ++i) {
       sendReply(clients[i], join_msg);
    }

    if (!channel->getTopic().empty()) {
        sendNumericReply(client, "332", channelName + " :" + channel->getTopic());
    } else {
        sendNumericReply(client, "331", channelName + " :No topic is set");
    }

    std::string names_list;
    for (size_t i = 0; i < clients.size(); ++i) {
        if(channel->isOperator(clients[i])) names_list += "@";
        names_list += clients[i]->getNickname();
        if (i < clients.size() - 1) names_list += " ";
    }
    sendNumericReply(client, "353", "= " + channelName + " :" + names_list);
    sendNumericReply(client, "366", channelName + " :End of /NAMES list");
}

void Server::cmdPart(Client* client, const std::vector<std::string>& args) {
    if (args.empty()) {
        sendNumericReply(client, "461", "PART :Not enough parameters");
        return;
    }
    const std::string& channelName = args[0];
    std::string reason = args.size() > 1 ? args[1] : "Leaving";

    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    if (it == _channels.end()) {
        sendNumericReply(client, "403", channelName + " :No such channel");
        return;
    }
    
    Channel* channel = it->second;
    if (!channel->isClientInChannel(client)) {
        sendNumericReply(client, "442", channelName + " :You're not on that channel");
        return;
    }

    std::string part_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " PART " + channelName + " :" + reason;
    std::vector<Client*> clients = channel->getClients();
     for (size_t i = 0; i < clients.size(); ++i) {
       sendReply(clients[i], part_msg);
    }

    channel->removeClient(client);

    if (channel->getClients().empty()) {
        _channels.erase(it);
        delete channel;
    }
}


void Server::cmdTopic(Client* client, const std::vector<std::string>& args) {
    if (args.empty()) {
        sendNumericReply(client, "461", "TOPIC :Not enough parameters");
        return;
    }
    const std::string& channelName = args[0];

    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    if (it == _channels.end()) {
        sendNumericReply(client, "403", channelName + " :No such channel");
        return;
    }

    Channel* channel = it->second;
    if (!channel->isClientInChannel(client)) {
        sendNumericReply(client, "442", channelName + " :You're not on that channel");
        return;
    }

    if (args.size() == 1) { // View topic
        if (!channel->getTopic().empty()) {
            sendNumericReply(client, "332", channelName + " :" + channel->getTopic());
        } else {
            sendNumericReply(client, "331", channelName + " :No topic is set");
        }
    } else { // Set topic
        if (channel->getMode('t') && !channel->isOperator(client)) {
            sendNumericReply(client, "482", channelName + " :You're not channel operator");
            return;
        }
        const std::string& newTopic = args[1];
        channel->setTopic(newTopic);
        
        std::string topic_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " TOPIC " + channelName + " :" + newTopic;
        std::vector<Client*> clients = channel->getClients();
        for (size_t i = 0; i < clients.size(); ++i) {
            sendReply(clients[i], topic_msg);
        }
    }
}

void Server::cmdKick(Client* client, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        sendNumericReply(client, "461", "KICK :Not enough parameters");
        return;
    }
    const std::string& channelName = args[0];
    const std::string& targetNick = args[1];
    std::string reason = args.size() > 2 ? args[2] : "Kicked";

    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    if (it == _channels.end()) {
        sendNumericReply(client, "403", channelName + " :No such channel");
        return;
    }
    Channel* channel = it->second;

    if (!channel->isOperator(client)) {
        sendNumericReply(client, "482", channelName + " :You're not channel operator");
        return;
    }
    
    Client* targetClient = findClientByNick(targetNick);
    if (!targetClient || !channel->isClientInChannel(targetClient)) {
        sendNumericReply(client, "441", targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    std::string kick_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " KICK " + channelName + " " + targetNick + " :" + reason;
    std::vector<Client*> clients = channel->getClients();
    for (size_t i = 0; i < clients.size(); ++i) {
        sendReply(clients[i], kick_msg);
    }

    channel->removeClient(targetClient);

    if (channel->getClients().empty()) {
        _channels.erase(it);
        delete channel;
    }
}

void Server::cmdInvite(Client* client, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        sendNumericReply(client, "461", "INVITE :Not enough parameters");
        return;
    }
    const std::string& targetNick = args[0];
    const std::string& channelName = args[1];

    Client* targetClient = findClientByNick(targetNick);
    if (!targetClient) {
        sendNumericReply(client, "401", targetNick + " :No such nick/channel");
        return;
    }

    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    if (it == _channels.end()) {
        sendNumericReply(client, "403", channelName + " :No such channel");
        return;
    }
    Channel* channel = it->second;

    if (channel->getMode('i') && !channel->isOperator(client)) {
        sendNumericReply(client, "482", channelName + " :You're not channel operator");
        return;
    }
    if (channel->isClientInChannel(targetClient)) {
        sendNumericReply(client, "443", targetNick + " " + channelName + " :is already on channel");
        return;
    }

    channel->addInvite(targetClient);
    
    sendNumericReply(client, "341", channelName + " " + targetNick);
    std::string invite_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " INVITE " + targetNick + " :" + channelName;
    sendReply(targetClient, invite_msg);
}

void Server::cmdMode(Client* client, const std::vector<std::string>& args) {
     if (args.empty()) {
        sendNumericReply(client, "461", "MODE :Not enough parameters");
        return;
    }
    const std::string& target = args[0];

    if (target[0] != '#') {
        // User mode changes are not supported in this basic server
        sendNumericReply(client, "502", ":Users MODE is not supported");
        return;
    }
    
    std::map<std::string, Channel*>::iterator it = _channels.find(target);
    if (it == _channels.end()) {
        sendNumericReply(client, "403", target + " :No such channel");
        return;
    }
    Channel* channel = it->second;

    if (args.size() == 1) { // Display channel modes
        sendNumericReply(client, "324", channel->getName() + " " + channel->getModesString());
        return;
    }

    if (!channel->isOperator(client)) {
        sendNumericReply(client, "482", target + " :You're not channel operator");
        return;
    }

    // Mode changes logic
    std::string modeStr = args[1];
    bool add = true;
    size_t arg_idx = 2;

    for (size_t i = 0; i < modeStr.length(); ++i) {
        char c = modeStr[i];
        if (c == '+') add = true;
        else if (c == '-') add = false;
        else {
            switch(c) {
                case 'i': channel->setMode('i', add); break;
                case 't': channel->setMode('t', add); break;
                case 'k':
                    if (arg_idx < args.size()) {
                        if (add) channel->setKey(args[arg_idx++]);
                        else channel->setKey("");
                    }
                    break;
                case 'o':
                    if (arg_idx < args.size()) {
                        Client* targetClient = findClientByNick(args[arg_idx++]);
                        if(targetClient && channel->isClientInChannel(targetClient)) {
                            if (add) channel->addOperator(targetClient);
                            else channel->removeOperator(targetClient);
                        }
                    }
                    break;
                case 'l':
                    if (add) {
                         if (arg_idx < args.size()) channel->setUserLimit(std::atoi(args[arg_idx++].c_str()));
                    } else {
                        channel->setUserLimit(0);
                    }
                    break;
            }
        }
    }
     std::string mode_msg = ":" + client->getNickname() + " MODE " + channel->getName() + " " + args[1];
     if (args.size() > 2) mode_msg += " " + args[2];
     std::vector<Client*> clients = channel->getClients();
     for(size_t i = 0; i < clients.size(); i++) {
         sendReply(clients[i], mode_msg);
     }
}

void Server::cmdQuit(Client* client, const std::vector<std::string>& args) {
    std::string quit_message = args.empty() ? "Client Quit" : args[0];
    
    std::string quit_broadcast = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " QUIT :Quit: " + quit_message;

    std::vector<Channel*> channels_to_cleanup;
     for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        if (it->second->isClientInChannel(client)) {
            std::vector<Client*> clients = it->second->getClients();
            for(size_t i=0; i < clients.size(); i++)
            {
                if(clients[i]!= client)
                     sendReply(clients[i], quit_broadcast);
            }
            it->second->removeClient(client);
            if (it->second->getClients().empty()) {
                channels_to_cleanup.push_back(it->second);
            }
        }
    }

    for (size_t i = 0; i < channels_to_cleanup.size(); ++i) {
        _channels.erase(channels_to_cleanup[i]->getName());
        delete channels_to_cleanup[i];
    }
    
    removeClient(client->getFd());
}