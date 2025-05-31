#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <ui/layers/LoadingOverlayLayer.hpp>
#include "Enums.hpp"

using namespace geode::prelude;

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

// gets and stores the current level,
// as well as handles the game state.
class GuessManager: public LevelDownloadDelegate, public LevelManagerDelegate {
protected:
    GuessManager() {}

    EventListener<web::WebTask> m_listener;

    // DashAuth token
    std::string m_daToken;

    // callbacks
    void levelDownloadFinished(GJGameLevel* level) override;
    void levelDownloadFailed(int x) override;
    
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) override;
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) override {
        this->loadLevelsFinished(p0, p1, 0);
    }

    void loadLevelsFailed(char const* p0, int p1) override;
    void loadLevelsFailed(char const* p0) override {
        this->loadLevelsFailed(p0, 0);
    }

    void setupRequest(web::WebRequest& req, matjson::Value body);
public:
    // real level downloaded from the servers
    // (used to handle stats conflicts)
    Ref<GJGameLevel> realLevel;
    
    // actual level being guessed
    // (using data copied from realLevel using GJGameLevel::copyLevelInfo)
    Ref<GJGameLevel> currentLevel;
    
    LoadingOverlayLayer* loadingOverlay = nullptr;
    TaskStatus taskStatus = TaskStatus::None;
    
    int totalScore = 0;
    
    GameOptions options;
    
    void startNewGame(GameOptions options);
    void endGame(bool pendingGuess);
    void applyPenalty(std::function<void()> callback);
    void submitGuess(LevelDate date, std::function<void(int score, LevelDate correctDate, LevelDate date)> callback);
    
    void updateStatusAndLoading(TaskStatus status);
    void syncScores();

    void safeRemoveLoadingLayer();
    void safeAddLoadingLayer();

    void showError(std::string error);

    void getLeaderboard(std::function<void(std::vector<LeaderboardEntry>)> callback);
    void getAccount(int accountID, std::function<void(LeaderboardEntry)> callback);
    void getGuesses(int accountID, std::function<void(GuessesResponse)> callback, int page = 0);

    const std::string getServerUrl();
    DateFormat getDateFormat();
    int getLevelDifficulty(GJGameLevel* level);
    GJFeatureState getFeaturedState(GJGameLevel* level);

    std::string decodeBase64(const std::string& input);
    std::string encodeBase64(const std::string& input);

    std::string statusToString(TaskStatus status);
    std::vector<LeaderboardEntry> jsonToEntries(std::vector<matjson::Value>);

    std::string formatDate(LevelDate);

    static GuessManager& get() {
        static GuessManager instance;
        return instance; 
    }
};
