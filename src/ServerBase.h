#pragma once

#include <cstdint>

class ServerBase
{
public:
    virtual ~ServerBase() = default;

    virtual bool init(std::uint16_t port) = 0;
    virtual bool run() = 0;
    virtual void stop() = 0;
};