#include "GuessManager.hpp"
#include <ui/layers/LevelLayer.hpp>

#include <argon/argon.hpp>

#include <Geode/cocos/support/base64.h>
#include <Geode/cocos/support/zip_support/ZipUtils.h>

// stolen from https://github.com/GlobedGD/globed2/blob/main/src/util/gd.cpp#L11
void reorderDownloadedLevel(GJGameLevel* level) {
    // thank you cvolton :D
    // this is needed so the level appears at the top of the saved list (unless Manual Level Order is enabled)

    auto* levels = GameLevelManager::get()->m_onlineLevels;

    bool putAtLowest = GameManager::get()->getGameVariable("0084");

    int idx = 0;
    for (const auto& [k, level] : CCDictionaryExt<::gd::string, GJGameLevel*>(levels)) {
        if (putAtLowest) {
            idx = std::min(idx, level->m_levelIndex);
        } else {
            idx = std::max(idx, level->m_levelIndex);
        }
    }

    if (putAtLowest) {
        idx -= 1;
    } else {
        idx += 1;
    }

    level->m_levelIndex = idx;
}

void GuessManager::updateStatusAndLoading(TaskStatus status) {
    taskStatus = status;
    if (loadingOverlay) loadingOverlay->updateStatus(status);
}

const std::string GuessManager::getServerUrl() {
    auto str = Mod::get()->getSettingValue<std::string>("server-url");
    if (str.ends_with("/")) {
        if (str.empty()) {
            log::error("server URL is empty... uh oh");
            return "";
        }
        str.pop_back();
    }
    if (str.empty()) {
        log::error("server URL is empty... uh oh");
    }

    return str;
}

DateFormat GuessManager::getDateFormat() {
    auto str = Mod::get()->getSettingValue<std::string>("date-format");
    log::debug("str: {}", str);
    if (str.empty()) {
        log::error("date-format is empty, resetting to default");
        Mod::get()->getSetting("date-format")->reset();
    }

    if (str.compare("MM/DD/YYYY") == 0)
        return DateFormat::American;
    else if (str.compare("YYYY/MM/DD") == 0)
        return DateFormat::Backwards;
    else
        return DateFormat::Normal;
}

void GuessManager::setupRequest(web::WebRequest& req, matjson::Value body) {
    req.header("Authorization", m_daToken);
    body.set("account_id", GJAccountManager::get()->m_accountID);
    req.bodyJSON(body);
}

void GuessManager::startNewGame(GameOptions options) {
    // get total score
    auto getAcc = [this]() {
        // EventListener<web::WebTask> listener;
        updateStatusAndLoading(TaskStatus::GetScore);
        m_listener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                
                if (res->code() != 200) {
                    log::debug("received non-200 code: {}", res->code());
                    return;
                }
                
                auto jsonRes = res->json();
                if (jsonRes.isErr()) {
                    log::error("error getting account json: {}", jsonRes.err());
                    return;
                }
                
                auto json = jsonRes.unwrap();
                
                auto scoreResult = json["total_score"].asInt();
                if (scoreResult.isErr()) {
                    log::error("unable to get score");
                    return;
                }

                auto score = scoreResult.unwrap();
                totalScore = score;
            } else if (e->isCancelled()) {
                if (loadingOverlay) loadingOverlay->removeMe();
                loadingOverlay = nullptr;
                log::error("request cancelled");
            }
        });
        
        auto req = web::WebRequest();
        auto gm = GameManager::get();
        setupRequest(req, matjson::makeObject({
            {"icon_id", gm->getPlayerFrame()},
            {"color1", gm->m_playerColor.value()},
            {"color2", gm->m_playerColor2.value()},
            {"color3", gm->m_playerGlow ?
                gm->m_playerGlowColor.value() :
                -1
            },
        }));
        m_listener.setFilter(req.post(fmt::format("{}/account", getServerUrl())));
    };

    auto doTheThing = [this, options, getAcc]() {
        if(!loadingOverlay) {
            loadingOverlay = LoadingOverlayLayer::create();
            loadingOverlay->addToScene();
        }
        updateStatusAndLoading(TaskStatus::Start);
        
        m_listener.bind([this, options, getAcc] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                if (res->code() != 200) {
                    log::error("error starting new round; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string"));
                    return;
                }
                
                auto jsonRes = res->json();
                if (jsonRes.isErr()) {
                    log::error("error getting json: {}", jsonRes.err());
                    return;
                }
                
                auto json = jsonRes.unwrap();
                
                auto levelIdRes = json["level"].asInt();
                if (levelIdRes.isErr()) {
                    log::error("error getting level id: {}", levelIdRes.err());
                    return;
                }

                auto startGame = [this, levelIdRes, options, getAcc]() {
                    if (this->realLevel && Mod::get()->getSettingValue<bool>("dont-save-levels")) {
                        GameLevelManager::get()->deleteLevel(this->realLevel);
                        this->realLevel = nullptr;
                    }
                    
                    auto levelId = levelIdRes.unwrap();
                    this->options = options;
                    
                    auto* glm = GameLevelManager::get();
                    glm->m_levelManagerDelegate = this;
                    glm->getOnlineLevels(GJSearchObject::create(SearchType::Search, std::to_string(levelId)));
                    getAcc();
                    updateStatusAndLoading(TaskStatus::GetLevel);
                };
                

                if (json["gameExists"].asBool().unwrap()) {
                    if (loadingOverlay) loadingOverlay->removeMe();
                    createQuickPopup(
                        "Exit Penalty",
                        "Looks like you exited the game before making a guess.\n<cr>Your total accuracy has dropped.</c>",
                    "Understood",
                    nullptr,
                    [startGame](auto, bool) {
                        GuessManager::get().applyPenalty(startGame);
                    });
                } else {
                    startGame();
                }
            } else if (e->isCancelled()) {
                if (loadingOverlay) loadingOverlay->removeMe();
                loadingOverlay = nullptr;
                log::error("request cancelled");
            }
        });

        auto req = web::WebRequest();
        setupRequest(req, matjson::makeObject({{"options", options}}));
        m_listener.setFilter(req.post(fmt::format("{}/start-new-game", getServerUrl())));
    };
    
    auto doAuthentication = [this, doTheThing]() {
        if(!loadingOverlay) {
            loadingOverlay = LoadingOverlayLayer::create();
            loadingOverlay->addToScene();
        }
        
        updateStatusAndLoading(TaskStatus::Authenticate);
        auto res = argon::startAuth([this, doTheThing](Result<std::string> res) {
            if (!res) {
                FLAlertLayer::create(
                    "Error",
                    fmt::format("Argon authentication error: {}", res.unwrapErr()),
                    "OK"
                )->show();
            }

            m_daToken = std::move(res).unwrap();
            doTheThing();
        }, [](argon::AuthProgress progress) {});

        if (!res) {
            FLAlertLayer::create(
                "Error",
                fmt::format("Argon authentication error: {}", res.unwrapErr()),
                "OK"
            )->show();
        }
    };

    if (!m_daToken.empty()) {
        doTheThing();
    } else {
        if (Mod::get()->getSavedValue<bool>("has-consented")) {
            doAuthentication();
        } else {
            createQuickPopup(
                "Authentication",
                "For <cg>verification purposes</c> this action will <cl>send a message to a bot account</c> and then immediately delete it.\n<cr>If you do not consent to this, press \"Cancel\"</c>",
            "Cancel",
            "Sumbit",
            [doAuthentication](auto alert, bool btn2) {
                if (btn2) {
                    doAuthentication();
                    Mod::get()->setSavedValue<bool>("has-consented", true);
                }
            });
        }

    }
}

void GuessManager::submitGuess(LevelDate date, std::function<void(int score, LevelDate correctDate, LevelDate date)> callback) {
    if(!loadingOverlay) {
        loadingOverlay = LoadingOverlayLayer::create();
        loadingOverlay->addToScene();
    }

    updateStatusAndLoading(TaskStatus::SubmitGuess);
    m_listener.bind([this, callback, date] (web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (res->code() != 200) {
                log::error("error starting submitting guess; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string"));
                return;
            }

            auto jsonRes = res->json();
            if (jsonRes.isErr()) {
                log::error("error getting json: {}", jsonRes.err());
                return;
            }

            auto json = jsonRes.unwrap();

            auto scoreResult = json["score"].asInt();
            if (scoreResult.isErr()) {
                log::error("unable to get score");
                return;
            }
            auto score = scoreResult.unwrap();
            totalScore += score;
            
            if (loadingOverlay) loadingOverlay->removeMe();
            loadingOverlay = nullptr;

            log::info("{}", static_cast<int>(json["correctDate"]["year"].asInt().unwrapOr(0)));
            callback(score, {
                .year = static_cast<int>(json["correctDate"]["year"].asInt().unwrapOr(0)),
                .month = static_cast<int>(json["correctDate"]["month"].asInt().unwrapOr(0)),
                .day = static_cast<int>(json["correctDate"]["day"].asInt().unwrapOr(0)),
            }, date);
        } else if (e->isCancelled()) {
            if (loadingOverlay) loadingOverlay->removeMe();
            loadingOverlay = nullptr;
            log::error("request cancelled");
        }
    });

    auto req = web::WebRequest();
    setupRequest(req, matjson::makeObject({}));
    m_listener.setFilter(req.post(fmt::format("{}/guess/{}-{}-{}", getServerUrl(), date.year, date.month, date.day)));
}

void GuessManager::endGame(bool pendingGuess) {
    auto doTheThing = [this]() {

        if(!loadingOverlay) {
            loadingOverlay = LoadingOverlayLayer::create();
            loadingOverlay->addToScene();
        }

        updateStatusAndLoading(TaskStatus::EndGame);

        m_listener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                if (res->code() != 200 && res->code() != 404) {
                    log::error("error ending game; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string"));
                    return;
                }
                
                if (this->realLevel && Mod::get()->getSettingValue<bool>("dont-save-levels")) {
                    GameLevelManager::get()->deleteLevel(this->realLevel);
                    this->realLevel = nullptr;
                }
                currentLevel = nullptr;
                
                if (loadingOverlay) loadingOverlay->removeMe();
                loadingOverlay = nullptr;
                
                auto layer = CreatorLayer::create();
                auto scene = CCScene::create();
                scene->addChild(layer);
                CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
            } else if (e->isCancelled()) {
                if (loadingOverlay) loadingOverlay->removeMe();
                loadingOverlay = nullptr;
                log::error("request cancelled");
            }
        });

        auto req = web::WebRequest();
        setupRequest(req, matjson::makeObject({}));
        m_listener.setFilter(req.post(fmt::format("{}/endGame", getServerUrl())));
    };

    std::string warning = pendingGuess ? "\n(This round will <cr>count as a loss</c>!)" : "";
    createQuickPopup(
        "End Game?",
        "Are you sure you want to <cr>end the game</c>?" + warning,
        "No", "Yes",
        [this, doTheThing, pendingGuess](auto, bool btn2) {
            if (!btn2) return;
            if (pendingGuess) GuessManager::get().applyPenalty(doTheThing);
            else doTheThing();
        }
    );
}

void GuessManager::applyPenalty(std::function<void()> callback) {
    if(!loadingOverlay) {
        loadingOverlay = LoadingOverlayLayer::create();
        loadingOverlay->addToScene();
    }

    m_listener.bind([this, callback] (web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (res->code() != 200 && res->code() != 404) {
                log::error("error applying penalty; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string"));
                return;
            }

            if (loadingOverlay) loadingOverlay->removeMe();
                loadingOverlay = nullptr;

            callback();
        } else if (e->isCancelled()) {
            if (loadingOverlay) loadingOverlay->removeMe();
            log::error("request cancelled");
            if (loadingOverlay) loadingOverlay->removeMe();
        }
    });

    auto req = web::WebRequest();
    setupRequest(req, matjson::makeObject({}));
    m_listener.setFilter(req.post(fmt::format("{}/penalty", getServerUrl())));
}

void GuessManager::getLeaderboard(std::function<void(std::vector<LeaderboardEntry>)> callback) {
    updateStatusAndLoading(TaskStatus::GetLeaderboard);
    m_listener.bind([this, callback] (web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (res->code() != 200) {
                log::error("error getting leaderboards; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string"));
                return;
            }

            auto jsonRes = res->json();
            if (jsonRes.isErr()) {
                log::error("error getting json: {}", jsonRes.err());
                return;
            }

            auto json = jsonRes.unwrap();

            auto lbResult = json.asArray();
            if (lbResult.isErr()) {
                log::error("unable to get score");
                return;
            }
            auto leaderboardJson = lbResult.unwrap();
            std::vector<LeaderboardEntry> entries = {};

            for (auto lbEntry : leaderboardJson) {
                #define ENTRY_VALUE(key, return_type, func, default) \
                    .key = static_cast<return_type>(lbEntry[#key].func().unwrapOr(default))
                
                entries.push_back({
                    ENTRY_VALUE(account_id, int, asInt, 0),
                    ENTRY_VALUE(username, std::string, asString, "Player"),
                    ENTRY_VALUE(total_score, int, asInt, 0),
                    ENTRY_VALUE(icon_id, int, asInt, 0),
                    ENTRY_VALUE(color1, int, asInt, 0),
                    ENTRY_VALUE(color2, int, asInt, 0),
                    ENTRY_VALUE(color3, int, asInt, 0),
                    ENTRY_VALUE(accuracy, float, asDouble, 0.f),
                    ENTRY_VALUE(max_score, int, asInt, 0),
                    ENTRY_VALUE(total_normal_guesses, int, asInt, 0),
                    ENTRY_VALUE(total_hardcore_guesses, int, asInt, 0),
                });

                #undef ENTRY_VALUE
            }

            callback(entries);
        } else if (e->isCancelled()) {
            log::error("request cancelled");
        }
    });

    auto req = web::WebRequest();
    m_listener.setFilter(req.get(fmt::format("{}/leaderboard", getServerUrl())));
}

void GuessManager::syncScores() {
    if (!currentLevel || !realLevel) return;

    // thanks cvolton for the help!
    auto playLayer = PlayLayer::get();
    int sessionAttempts = playLayer->m_attempts;
    int oldAttempts = realLevel->m_attempts.value();
    realLevel->handleStatsConflict(currentLevel);
    realLevel->m_attempts = sessionAttempts + oldAttempts;
    realLevel->m_orbCompletion = currentLevel->m_orbCompletion;
    GameManager::get()->reportPercentageForLevel(realLevel->m_levelID, realLevel->m_normalPercent, realLevel->isPlatformer());
}

void GuessManager::loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) {
    auto* glm = GameLevelManager::get();
    glm->m_levelManagerDelegate = nullptr;

    if (p0->count()) {
        glm->m_levelDownloadDelegate = this;
        glm->downloadLevel(static_cast<GJGameLevel*>(p0->objectAtIndex(0))->m_levelID, false);
    }
}

void GuessManager::loadLevelsFailed(char const* p0, int p1) {
    log::error("unable to load level: {}, {}", p1, p0);
}

void GuessManager::levelDownloadFinished(GJGameLevel* level) {
    auto* glm = GameLevelManager::get();
    glm->m_levelDownloadDelegate = nullptr;

    reorderDownloadedLevel(level);

    realLevel = level;

    if (loadingOverlay) loadingOverlay->removeMe();
    loadingOverlay = nullptr;

    this->currentLevel = GJGameLevel::create();
    this->currentLevel->copyLevelInfo(level);
    this->currentLevel->m_levelName = "???????";
    this->currentLevel->m_creatorName = "GDGuesser";
    auto levelDecoded = decodeBase64(this->currentLevel->m_levelString);
    this->currentLevel->m_levelString = encodeBase64(levelDecoded + "1,3854,2,-1000,3,1000,135,1;");
    // auto layer = LevelInfoLayer::create(level, false);
    auto layer = LevelLayer::create();
    auto scene = CCScene::create();
    scene->addChild(layer);
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
}


void GuessManager::levelDownloadFailed(int x) {
    log::warn("could not fetch level, code {}", x);

    if (loadingOverlay) loadingOverlay->removeMe();
    loadingOverlay = nullptr;
}

int GuessManager::getLevelDifficulty(GJGameLevel* level) {
    if (level->m_autoLevel) return -1;
    auto diff = level->m_difficulty;

    if (level->m_ratingsSum != 0)
        diff = static_cast<GJDifficulty>(level->m_ratingsSum / 10);

    if (level->m_demon > 0) {
        switch (level->m_demonDifficulty) {
            case 3: return 7;
            case 4: return 8;
            case 5: return 9;
            case 6: return 10;
            default: return 6;
        }
    }

    switch (diff) {
        case GJDifficulty::Easy: return 1;
        case GJDifficulty::Normal: return 2;
        case GJDifficulty::Hard: return 3;
        case GJDifficulty::Harder: return 4;
        case GJDifficulty::Insane: return 5;
        case GJDifficulty::Demon: return 6;
        default: return 0;
    }
}

GJFeatureState GuessManager::getFeaturedState(GJGameLevel* level) {
    if (level->m_isEpic == 3) {
        return GJFeatureState::Mythic;
    } else if (level->m_isEpic == 2) {
        return GJFeatureState::Legendary;
    } else if (level->m_isEpic == 1) {
        return GJFeatureState::Epic;
    } else if (level->m_featured > 0) {
        return GJFeatureState::Featured;
    } else {
        return GJFeatureState::None;
    }
}

std::string GuessManager::decodeBase64(const std::string& input) {
    return ZipUtils::decompressString(input, false, 0);
}

std::string GuessManager::encodeBase64(const std::string& input) {
    return ZipUtils::compressString(input, false, 0);
}

std::string GuessManager::statusToString(TaskStatus status) {
    switch (status) {
        case TaskStatus::Authenticate:
            return "Authenticating\nWith Argon"; break;
        
        case TaskStatus::Start:
            return "Starting"; break;
        
        case TaskStatus::GetLevel:
            return "Downloading Level"; break;
        
        case TaskStatus::GetScore:
            return "Getting Score"; break;
        
        case TaskStatus::GetLeaderboard:
            return "Getting Leaderboard"; break;
        
        case TaskStatus::SubmitGuess:
            return "Submitting Guess"; break;

        case TaskStatus::EndGame:
            return "Ending Game"; break;
        
        default:
            return "Unknown"; break;
    }

    return "Unknown";
}
