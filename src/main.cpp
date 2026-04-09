#include <iostream>
#include <csignal>
#include <memory>
#include <cstdlib>
#include "SelectServer.h"

std::unique_ptr<ServerBase> g_server;

void signalHandler(int signum)
{
    std::cout << "\nInterrupt signal (" << signum << ") received.\n";
    if (g_server)
        g_server->stop();
}

int main(int argc, char* argv[])
{
    uint16_t port = 8080;
    if (argc == 2)
        port = static_cast<uint16_t>(std::atoi(argv[1]));

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    g_server = std::make_unique<SelectServer>();
    if (!g_server->init(port))
    {
        std::cerr << "Failed to initialize server\n";
        return 1;
    }

    g_server->run();
    std::cout << "Server stopped.\n";

    return 0;
}