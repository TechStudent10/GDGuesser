#include "GuessManager.hpp"
#include <ui/layers/LevelLayer.hpp>

#include <argon/argon.hpp>

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
        m_listener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                if (res->code() != 200) {
                    log::debug("received non-200 code: {}", res->code);
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
                if (m_loadingOverlay) m_loadingOverlay->removeMe();
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
        m_loadingOverlay = LoadingOverlayLayer::create();
        m_loadingOverlay->addToScene();

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

                auto levelId = levelIdRes.unwrap();
                this->options = options;

                auto* glm = GameLevelManager::get();
                glm->m_levelManagerDelegate = this;
                glm->getOnlineLevels(GJSearchObject::create(SearchType::Search, std::to_string(levelId)));
                getAcc();
            } else if (e->isCancelled()) {
                if (m_loadingOverlay) m_loadingOverlay->removeMe();
                log::error("request cancelled");
            }
        });

        auto req = web::WebRequest();
        setupRequest(req, matjson::makeObject({{"options", options}}));
        m_listener.setFilter(req.post(fmt::format("{}/start-new-game", getServerUrl())));
    };
    
    auto doAuthentication = [this, doTheThing]() {
        auto notif = Notification::create(
            "Currently authenticating with Argon.\nThis will take 5-10 seconds. Please wait...",
            NotificationIcon::Loading,  
            0.f);
        notif->show();

        auto res = argon::startAuth([this, doTheThing, notif](Result<std::string> res) {
            if (!res) {
                FLAlertLayer::create(
                    "Error",
                    fmt::format("Argon authentication error: {}", res.unwrapErr()),
                    "OK"
                )->show();
                notif->hide();
            }

            m_daToken = std::move(res).unwrap();
            notif->hide();
            doTheThing();
        }, [](argon::AuthProgress progress) {});

        if (!res) {
            FLAlertLayer::create(
                "Error",
                fmt::format("Argon authentication error: {}", res.unwrapErr()),
                "OK"
            )->show();
            notif->hide();
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
    m_loadingOverlay = LoadingOverlayLayer::create();
    m_loadingOverlay->addToScene();

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
            
            if (m_loadingOverlay) m_loadingOverlay->removeMe();
            log::info("{}", static_cast<int>(json["correctDate"]["year"].asInt().unwrapOr(0)));
            callback(score, {
                .year = static_cast<int>(json["correctDate"]["year"].asInt().unwrapOr(0)),
                .month = static_cast<int>(json["correctDate"]["month"].asInt().unwrapOr(0)),
                .day = static_cast<int>(json["correctDate"]["day"].asInt().unwrapOr(0)),
            }, date);
        } else if (e->isCancelled()) {
            if (m_loadingOverlay) m_loadingOverlay->removeMe();
            log::error("request cancelled");
        }
    });

    auto req = web::WebRequest();
    setupRequest(req, matjson::makeObject({}));
    m_listener.setFilter(req.post(fmt::format("{}/guess/{}-{}-{}", getServerUrl(), date.year, date.month, date.day)));
}

void GuessManager::endGame() {
    auto doTheThing = [this]() {
        m_listener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                if (res->code() != 200 && res->code() != 404) {
                    log::error("error ending game; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string"));
                    return;
                }

                currentLevel = nullptr;
                
                if (m_loadingOverlay) m_loadingOverlay->removeMe();
                m_loadingOverlay = nullptr;

                auto layer = CreatorLayer::create();
                auto scene = CCScene::create();
                scene->addChild(layer);
                CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
            } else if (e->isCancelled()) {
                if (m_loadingOverlay) m_loadingOverlay->removeMe();
                log::error("request cancelled");
            }
        });

        auto req = web::WebRequest();
        setupRequest(req, matjson::makeObject({}));
        m_listener.setFilter(req.post(fmt::format("{}/endGame", getServerUrl())));
    };

    createQuickPopup(
        "End Game?",
        "Are you sure you want to <cr>end the game</c>?",
        "No", "Yes",
        [this, doTheThing](auto, bool btn2) {
            if (!btn2) return;

            m_loadingOverlay = LoadingOverlayLayer::create();
            m_loadingOverlay->addToScene();

            doTheThing();
        }
    );
}

void GuessManager::getLeaderboard(std::function<void(std::vector<LeaderboardEntry>)> callback) {
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
                entries.push_back({
                    .account_id = static_cast<int>(lbEntry["account_id"].asInt().unwrapOr(0)),
                    .username = lbEntry["username"].asString().unwrapOr("Player"),
                    .total_score = static_cast<int>(lbEntry["total_score"].asInt().unwrapOr(0)),
                    .icon_id = static_cast<int>(lbEntry["icon_id"].asInt().unwrapOr(0)),
                    .color1 = static_cast<int>(lbEntry["color1"].asInt().unwrapOr(0)),
                    .color2 = static_cast<int>(lbEntry["color2"].asInt().unwrapOr(0)),
                    .color3 = static_cast<int>(lbEntry["color3"].asInt().unwrapOr(0)),
                    .accuracy = static_cast<float>(lbEntry["accuracy"].asDouble().unwrapOr(0.f)),
                });
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

    if (m_loadingOverlay) m_loadingOverlay->removeMe();
    m_loadingOverlay = nullptr;

    this->currentLevel = GJGameLevel::create();
    this->currentLevel->copyLevelInfo(level);
    this->currentLevel->m_levelName = "???????";
    this->currentLevel->m_creatorName = "GDGuesser";
    // auto layer = LevelInfoLayer::create(level, false);
    auto layer = LevelLayer::create();
    auto scene = CCScene::create();
    scene->addChild(layer);
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
}


void GuessManager::levelDownloadFailed(int x) {
    log::warn("could not fetch level, code {}", x);

    if (m_loadingOverlay) m_loadingOverlay->removeMe();
    m_loadingOverlay = nullptr;
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
