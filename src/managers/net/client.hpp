#pragma once

#include <matjson/reflect.hpp>
#include "../types.hpp"

#define NAME(name) static constexpr const char* EVENT_NAME = name;

struct CreateDuel {
    NAME("create duel")

    GameOptions options;
};

struct JoinDuel {
    NAME("join duel")

    std::string joinCode;
};

struct ToggleReady {
    NAME("toggle ready")
};

struct SubmitGuess {
    NAME("submit guess")

    LevelDate date;
};

struct GetDuel {
    NAME("get duel")
};

struct OpponentGuessed {
    NAME("opponent guessed")
};

struct Forfeit {
    NAME("forfeit")
};

#undef NAME
