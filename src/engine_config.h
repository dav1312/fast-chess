#pragma once

#include <string>
#include <tuple>
#include <vector>

struct TimeControl
{
    uint64_t moves = 0;
    uint64_t time = 0;
    uint64_t increment = 0;
};

struct EngineConfiguration
{
    // engine name
    std::string name;
    // Path to engine
    std::string dir;

    // Engine binary name
    std::string cmd;

    // Custom args that should be sent
    std::string args;

    // UCI options
    std::vector<std::pair<std::string, std::string>> options;

    // time control for the engine
    TimeControl tc;

    // Node limit for the engine
    uint64_t nodes = 0;

    // Ply limit for the engine
    uint64_t plies = 0;
};