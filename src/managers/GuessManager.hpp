#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <ui/layers/LoadingOverlayLayer.hpp>
#include "types.hpp"

using namespace geode::prelude;

class DuelsPersistentNode;

// gets and stores the current level,
// as well as handles the game state.
class GuessManager: public LevelDownloadDelegate, public LevelManagerDelegate {
protected:
    GuessManager() {}

    // JWT
    std::string m_token;

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

    std::string duelJoinCode;
    Duel currentDuel;

    DuelsPersistentNode* persistentNode = nullptr;
    
    void authenticate(std::function<void()> cb);

    void startNewGame(GameOptions options);
    void endGame(bool pendingGuess);
    void applyPenalty(std::function<void()> callback);
    void submitGuess(LevelDate date, std::function<void(int score, LevelDate correctDate, LevelDate date)> callback);

    void startGame(int levelId, GameOptions options);
    void submitGuessDuel(LevelDate date, std::function<void(int score, LevelDate correctDate, LevelDate date)> callback);
    
    void updateStatusAndLoading(TaskStatus status);
    void syncScores();

    void safeRemoveLoadingLayer();
    void safeAddLoadingLayer();

    void showError(std::string error, int code);

    void getLeaderboard(std::function<void(std::vector<LeaderboardEntry>)> callback);
    void getAccount(std::function<void(LeaderboardEntry)> callback, int accountID = 0, std::string username = "");
    void getGuesses(int accountID, std::function<void(GuessesResponse)> callback, int page = 0);

    void createDuel(std::function<void()> cb);
    void joinDuel(std::string code, std::function<void()> cb);

    const std::string getServerUrl();
    DateFormat getDateFormat();
    int getLevelDifficulty(GJGameLevel* level);
    GJFeatureState getFeaturedState(GJGameLevel* level);

    std::string decodeBase64(const std::string& input);
    std::string encodeBase64(const std::string& input);

    std::string statusToString(TaskStatus status);
    std::string verboseToSimple(int id, std::string error);
    std::vector<LeaderboardEntry> jsonToEntries(std::vector<matjson::Value>);

    std::string formatDate(LevelDate date);
    std::string formatNumberWithCommas(int number);

    void connectToMP();

    static GuessManager& get() {
        static GuessManager instance;
        return instance; 
    }
};
