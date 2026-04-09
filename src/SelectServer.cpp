#include "SelectServer.h"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <arpa/inet.h>

bool SelectServer::init(std::uint16_t port)
{
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0)
    {
        std::cerr << "[SELECT] socket() failed: "
                  << std::strerror(errno) << "\n";
        return false;
    }

    int opt = 1;
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "[SELECT] setsockopt() failed: "
                 << std::strerror(errno) << "\n";
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        std::cerr << "[SELECT] bind() failed: "
                  << strerror(errno) << "\n";
        close(listen_fd_);
        return false;
    }

    if (listen(listen_fd_, SOMAXCONN) < 0)
    {
        std::cerr << "[SELECT]  listen() failed: "
                  << std::strerror(errno) << "\n";
        close(listen_fd_);
        return false;
    }

    FD_ZERO(&master_set_);
    FD_SET(listen_fd_, &master_set_);
    max_fd_ = listen_fd_;

    std::cout << "[SELECT] Listening on port "
              << port << " (FD_SETSIZE=" << FD_SETSIZE
              << ")\n";

    return true;
}

bool SelectServer::run()
{
    running_ = true;
    timeval tv{1, 0};

    while (running_)
    {
        fd_set read_fds = master_set_;

        int activity = select(max_fd_ + 1, &read_fds, nullptr, nullptr, &tv);
        if (activity < 0)
        {
            if (errno == EINTR)
                continue;

            std::cerr << "[SELECT] select() failed: "
                      << std::strerror(errno) << "\n";
            break;
        } else if (activity == 0)
        {
            continue;
        }

        for (int fd = 0; fd <= max_fd_; ++fd)
        {
            if (!FD_ISSET(fd, &read_fds))
                continue;

            if (fd == listen_fd_)
            {
                acceptClient();
            } else
            {
                handleClient(fd);
            }
        }
    }

    cleanup();
    return true;
}

void SelectServer::stop()
{
    running_ = false;
}

void SelectServer::acceptClient()
{
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_fd < 0)
    {
        std::cerr << "[SELECT] accept() failed: "
                  << std::strerror(errno) << "\n";
        return;
    }

    if (client_fd >= FD_SETSIZE)
    {
        std::cerr << "[SELECT] WARNING: client_fd " << client_fd
                  << " >= FD_SETSIZE (" << FD_SETSIZE
                  << "), closing connection (select limitation)\n";
        close(client_fd);
        return;
    }

    FD_SET(client_fd, &master_set_);
    if (client_fd > max_fd_)
        max_fd_ = client_fd;

    clients_.push_back(client_fd);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
    std::cout << "[SELECT] New client " << ip_str
              << ":" << ntohs(client_addr.sin_port)
              << " (fd=" << client_fd << "), total clients: "
              << clients_.size() << "\n";
}

void SelectServer::handleClient(int fd)
{
    char buffer[1024];
    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
    if (bytes < 0)
    {
        std::cerr << "[SELECT] recv() error on fd " << fd << ": "
                  << std::strerror(errno) << "\n";
        closeClient(fd);
    } else if (bytes == 0) // Connection closed by client
    {
        std::cout << "[SELECT] Client fd " << fd << "  disconnected\n";
        closeClient(fd);
    } else
    {
        ssize_t sent = send(fd, buffer,  bytes, 0);
        if (sent < 0)
        {
            std::cerr << "[SELECT] send() error on fd " << fd << ":"
                      << std::strerror(errno) << "\n";
            closeClient(fd);
        }
    }
}

void SelectServer::closeClient(int fd)
{
    close(fd);
    FD_CLR(fd, &master_set_);

    auto it = std::find(clients_.begin(), clients_.end(), fd);
    if (it != clients_.end())
        clients_.erase(it);

    if (fd == max_fd_)
        updateMaxFd();

    std::cout << "[SELECT] Client fd " << fd
              << " closed, remaining clients: "
              << clients_.size() << "\n";
}

void SelectServer::updateMaxFd()
{
    max_fd_ = listen_fd_;
    for (int fd : clients_)
    {
        if (fd > max_fd_)
            max_fd_ = fd;
    }
}

void SelectServer::cleanup()
{
    for (int fd :  clients_)
    {
        close(fd);
    }

    clients_.clear();
    
    if (listen_fd_ != -1)
        close(listen_fd_);

    FD_ZERO(&master_set_);
    max_fd_ = -1;
}