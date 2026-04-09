#pragma once

#include "ServerBase.h"
#include <sys/select.h>
#include <netinet/in.h>
#include <vector>

class SelectServer : public ServerBase
{
public:
    bool init(std::uint16_t port) override;
    bool run() override;
    void stop() override;

private:
    int listen_fd_ = -1;
    fd_set master_set_;
    int max_fd_ = -1;
    bool running_ = false;
    std::vector<int> clients_;

    void acceptClient();
    void handleClient(int fd);
    void closeClient(int fd);
    void updateMaxFd();
    void cleanup();
};