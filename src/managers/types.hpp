#pragma once

#include <matjson.hpp>
#include <matjson/reflect.hpp>

enum class DateFormat {
    Normal,
    American,
    Backwards
};

enum class GameMode {
    Normal,
    Hardcore,
    Duels
};

enum class TaskStatus {
    None,
    Authenticate,
    Start,
    GetLevel,
    GetScore,
    GetLeaderboard,
    SubmitGuess,
    EndGame,
    LoadingAccount,
    WaitingForOpponent,
    CreatingLobby,
    JoiningLobby
};

struct LevelDate {
    int year;
    int month;
    int day;
};

struct LeaderboardEntry {
    int account_id;
    std::string username;
    int total_score;
    int icon_id;
    int color1;
    int color2;
    int color3;
    float accuracy;
    int max_score;
    int total_normal_guesses;
    int total_hardcore_guesses;
    int leaderboard_position;
};

struct GuessEntry {
    int level_id;
    int account_id;
    GameMode mode;
    int score;
    LevelDate correct_date;
    LevelDate guessed_date;
    std::string level_name;
    std::string level_creator;
};

struct GuessesResponse {
    std::vector<GuessEntry> entries;
    int page;
    int total_pages;
};

struct GameOptions {
    GameMode mode;
    std::vector<std::string> versions;
};

template <>
struct matjson::Serialize<GameOptions> {
    static geode::Result<GameOptions> fromJson(const matjson::Value& value) {
        GEODE_UNWRAP_INTO(int mode, value["mode"].asInt());
        GEODE_UNWRAP_INTO(std::vector<Value> versionsArr, value["versions"].asArray());

        std::vector<std::string> versions;

        for (auto versionsVal : versionsArr) {
            versions.push_back(versionsVal.asString().unwrap());
        }

        return geode::Ok(GameOptions { .mode = static_cast<GameMode>(mode), .versions = versions });
    }
    static matjson::Value toJson(const GameOptions& options) {
        return matjson::makeObject({
            { "mode", static_cast<int>(options.mode) },
            { "versions", options.versions }
        });
    }
};

struct Duel {
    std::map<std::string, LeaderboardEntry> players;
    std::vector<int> playersReady;
    
    int currentLevelId;
    GameOptions options;
    
    std::string joinCode;
    std::map<std::string, int> scores;
};
