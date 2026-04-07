#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

constexpr uint16_t PORT = 8080;
constexpr int BACKLOG = 5;
constexpr size_t BUFFER_SIZE = 1024;

int main()
{
    // 1. Socket created
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    // 2. Allow address reuse (to avoid timeout after server restart)
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "setsocketopt() failed: " << std::strerror(errno) << "\n";
        close(listen_fd);
        return 1;
    }

    // 3. Bind socket to address
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    server_addr.sin_port = htons(PORT);

    if (bind(listen_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        std::cerr << "bind() failed: " << std::strerror(errno) << "\n";
        close(listen_fd);
        return 1;
    }

    // 4. Switching socket to listening mode
    if (listen(listen_fd, BACKLOG) < 0)
    {
        std::cerr << "listen() failed: " << std::strerror(errno) << "\n";
        close(listen_fd);
        return 1;
    }

    std::cout << "[BLOCKING SERVER] Listening on port " << PORT << "\n";

    // 5. Accept 1 client and serv them until disconnect
    while (1)
    {
        std::cout << "Waiting for a client to connect...\n";

        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);

        // server block
        int client_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
        
        if (client_fd <0)
        {
            std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        std::cout << "Client connected from " << client_ip << ":" << ntohs(client_addr.sin_port) << "\n";

        // 6. Serv 1 client
        char buffer[BUFFER_SIZE]{};
        while (true)
        {
            // server block
            ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_received < 0)
            {
                std::cerr << "recv() error: " << std::strerror(errno) << "\n";
                break;
            } else if (bytes_received == 0)
            {
                std::cout << "Client closed connection\n";
                break;
            }

            // server block
            ssize_t bytes_sent = send(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_sent < 0)
            {
                std::cerr << "send() error: " << std::strerror(errno) << "\n";
                break;
            }
        }

        close(client_fd);
        std::cout << "Client disconnected, ready for next client\n";
    }

    close(listen_fd);

    return 0;
}