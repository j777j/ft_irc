#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

void signalHandler(int signum) {
    (void)signum;
    // In a real server, you would set a global flag to true
    // and the main loop would check this flag to shutdown gracefully.
    std::cout << "\nInterrupt signal received. Shutting down server." << std::endl;
    // For now, we just exit.
    exit(0);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        long port = std::strtol(argv[1], NULL, 10);
        // Add more robust port validation here (e.g., check range 1024-65535)
        
        Server server(static_cast<int>(port), argv[2]);
        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
