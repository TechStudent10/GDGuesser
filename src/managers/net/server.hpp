#pragma once

#include <matjson/reflect.hpp>
#include "../types.hpp"

#define NAME(name) static constexpr const char* EVENT_NAME = name;

struct DuelCreated {
    NAME("duel created")

    std::string joinCode;
};

struct DuelJoined {
    NAME("duel joined")

    int status;
};

struct DuelUpdated {
    NAME("duel updated")
    
    Duel duel;
};

struct RoundStarted {
    NAME("round started")

    GameOptions options;
    int levelId;
};

struct DuelResults {
    NAME("duel results")

    std::unordered_map<std::string, int> scores;
    std::unordered_map<std::string, int> totalScores;
    std::unordered_map<std::string, LevelDate> guessedDates;
    LevelDate correctDate;
};

struct ReceiveDuel {
    NAME("receive duel")

    Duel duel;
};

struct DuelEnded {
    NAME("duel ended")

    Duel duel;
    int winner;
    int loser;
};

#undef NAME
