#pragma once

enum class DateFormat {
    Normal,
    American,
    Backwards
};

enum class GameMode {
    Normal,
    Hardcore
};

enum class TaskStatus {
    None,
    Authenticate,
    Start,
    GetLevel,
    GetScore,
    GetLeaderboard,
    SubmitGuess,
    EndGame
};