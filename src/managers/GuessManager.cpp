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
    req.header("Version", Mod::get()->getVersion().toNonVString());
    if (body.size() != 0) {
        req.bodyJSON(body);
    }
    m_listener.enable();
}

void GuessManager::showError(std::string error, int code) {
    log::error("{}", error);
    auto simple = verboseToSimple(code, error);
    createQuickPopup(
        "GDGuesser Error:",
        simple,
        "Copy Full Error", "OK",
        [this, error](auto, bool btn2) {
            if (btn2) return;
            auto str = fmt::format("{}", error);
            clipboard::write(str);
            Notification::create("Copied to clipboard", NotificationIcon::None)->show();
        }
    );
    safeRemoveLoadingLayer();
}

void GuessManager::cancelCurrentRequest() {
    m_listener.disable();
}

void GuessManager::startNewGame(GameOptions options) {
    auto doTheThing = [this, options]() {
        auto optionsCopy = options;
        safeAddLoadingLayer();
        updateStatusAndLoading(TaskStatus::Start);        
        m_listener.bind([this, options] (web::WebTask::Event* e) {
            if (this->currentLevel) {
                return;
            }

            if (web::WebResponse* res = e->getValue()) {
                if (res->code() != 200) {
                    showError(fmt::format("error starting new round; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string")), res->code());
                    return;
                }
                
                auto jsonRes = res->json();
                if (jsonRes.isErr()) {
                    showError(fmt::format("error getting json: {}", jsonRes.unwrapErr()), 1001);
                    return;
                }
                
                auto json = jsonRes.unwrap();
                
                auto levelIdRes = json["level"].asInt();
                if (levelIdRes.isErr()) {
                    showError(fmt::format("error getting level id: {}", levelIdRes.unwrapErr()), 1101);
                    return;
                }

                auto startGame = [this, levelIdRes, options]() {
                    if (this->realLevel && Mod::get()->getSettingValue<bool>("dont-save-levels")) {
                        GameLevelManager::get()->deleteLevel(this->realLevel);
                        this->realLevel = nullptr;
                    }
                    
                    auto levelId = levelIdRes.unwrap();
                    this->options = options;
                    
                    auto* glm = GameLevelManager::get();
                    glm->m_levelManagerDelegate = this;
                    glm->getOnlineLevels(GJSearchObject::create(SearchType::Search, std::to_string(levelId)));
                    updateStatusAndLoading(TaskStatus::GetLevel);
                };
                

                if (json["gameExists"].asBool().unwrap()) {
                    safeRemoveLoadingLayer();
                    createQuickPopup(
                        "Exit Penalty",
                        "Looks like you exited the game before making a guess.\n<cr>Your total accuracy has dropped.</c>",
                    "OK",
                    nullptr,
                    [startGame, options](auto, bool) {
                        startGame();
                    });
                } else {
                    startGame();
                }
            } else if (e->isCancelled()) {
                showError("request cancelled", 1303);
            }
        });

        auto req = web::WebRequest();
        setupRequest(req, matjson::makeObject({{"options", optionsCopy}}));
        m_listener.setFilter(req.post(fmt::format("{}/start-new-game", getServerUrl())));
    };

    // get total score
    auto getAcc = [this, doTheThing, options](std::string argonToken) {
        // EventListener<web::WebTask> listener;
        m_listener.bind([this, doTheThing, options] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                if (res->code() != 200) {
                    safeRemoveLoadingLayer();
                    showError(fmt::format("received non-200 code: {}, {}", res->code(), res->string().unwrapOr("b")), res->code());
                    return;
                }

                auto jsonRes = res->json();
                if (jsonRes.isErr()) {
                    showError(fmt::format("error getting account json: {}", jsonRes.unwrapErr()), 1002);
                    return;
                }

                auto json = jsonRes.unwrap();

                auto scoreResult = json["user"]["total_score"].asInt();
                if (scoreResult.isErr()) {
                    showError(fmt::format("unable to get score: {}", scoreResult.unwrapErr()), 1106);
                    return;
                }
                auto score = scoreResult.unwrap();
                totalScore = score;

                auto tokenResult = json["token"].asString();
                if (tokenResult.isErr()) {
                    showError(fmt::format("unable to get JWT!!! {}", tokenResult.unwrapErr()), 1102);
                    return;
                }

                auto token = tokenResult.unwrap();
                m_daToken = token;

                doTheThing();
            } else if (e->isCancelled()) {
                safeRemoveLoadingLayer();
                showError("request cancelled", 1303);
            }
        });

        auto req = web::WebRequest();
        auto gm = GameManager::get();
        req.bodyJSON(matjson::makeObject({
            {"icon_id", gm->getPlayerFrame()},
            {"color1", gm->m_playerColor.value()},
            {"color2", gm->m_playerColor2.value()},
            {"color3", gm->m_playerGlow ?
                gm->m_playerGlowColor.value() :
                -1
            },
            {"account_id", GJAccountManager::get()->m_accountID}
        }));
        req.header("Authorization", argonToken);
        req.header("Version", Mod::get()->getVersion().toNonVString());
        m_listener.enable();
        m_listener.setFilter(req.post(fmt::format("{}/login", getServerUrl())));
    };
    
    auto doAuthentication = [this, getAcc, options]() {
        safeAddLoadingLayer();
        
        updateStatusAndLoading(TaskStatus::Authenticate);
        auto res = argon::startAuth([this, getAcc, options](Result<std::string> res) {
            if (!res || res.isErr()) {
                showError(fmt::format("Argon authentication error: {}", res.unwrapErr()), 1301);
                return;
            }

            getAcc(std::move(res).unwrap());
        }, [](argon::AuthProgress progress) {});

        if (!res) {
            if (res.unwrapErr() != "Stage 2 failed (generic error)") {
                showError(fmt::format("{}", res.unwrapErr()), 1301);
            } else {
                showError(fmt::format("{}", res.unwrapErr()), 1302);
            }
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
    safeAddLoadingLayer();

    updateStatusAndLoading(TaskStatus::SubmitGuess);
    m_listener.bind([this, callback, date] (web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (res->code() != 200) {
                showError(fmt::format("error starting submitting guess; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string")), res->code());
                return;
            }

            auto jsonRes = res->json();
            if (jsonRes.isErr()) {
                showError(fmt::format("error getting json: {}", jsonRes.unwrapErr()), 1003);
                return;
            }

            auto json = jsonRes.unwrap();

            auto scoreResult = json["score"].asInt();
            if (scoreResult.isErr()) {
                showError(fmt::format("unable to get score: {}", scoreResult.unwrapErr()), 1103);
                return;
            }
            auto score = scoreResult.unwrap();

            // if all versions aren't selected, your total score will not be impacted
            if (options.versions.size() == 13) {
                totalScore += score;
            }
            
            safeRemoveLoadingLayer();

            log::info("{}", static_cast<int>(json["correctDate"]["year"].asInt().unwrapOr(0)));
            callback(score, {
                .year = static_cast<int>(json["correctDate"]["year"].asInt().unwrapOr(0)),
                .month = static_cast<int>(json["correctDate"]["month"].asInt().unwrapOr(0)),
                .day = static_cast<int>(json["correctDate"]["day"].asInt().unwrapOr(0)),
            }, date);
            this->currentLevel = nullptr;
            this->realLevel = nullptr;
        } else if (e->isCancelled()) {
            safeRemoveLoadingLayer();
            showError("request cancelled", 1303);
        }
    });

    auto req = web::WebRequest();
    setupRequest(req, matjson::makeObject({}));
    m_listener.setFilter(req.post(fmt::format("{}/guess/{}-{}-{}", getServerUrl(), date.year, date.month, date.day)));
}

void GuessManager::endGame(bool pendingGuess) {
    auto doTheThing = [this]() {

        safeAddLoadingLayer();

        updateStatusAndLoading(TaskStatus::EndGame);

        m_listener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                if (res->code() != 200 && res->code() != 404) {
                    showError(fmt::format("error ending game; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string")), res->code());
                    return;
                }
                
                if (this->realLevel && Mod::get()->getSettingValue<bool>("dont-save-levels")) {
                    GameLevelManager::get()->deleteLevel(this->realLevel);
                    this->realLevel = nullptr;
                }
                currentLevel = nullptr;
                
                safeRemoveLoadingLayer();
                
                auto layer = CreatorLayer::create();
                auto scene = CCScene::create();
                scene->addChild(layer);
                CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
            } else if (e->isCancelled()) {
                safeRemoveLoadingLayer();
                showError("request cancelled", 1303);
            }
        });

        auto req = web::WebRequest();
        setupRequest(req, matjson::makeObject({}));
        m_listener.setFilter(req.post(fmt::format("{}/endGame", getServerUrl())));
    };

    std::string warning = pendingGuess && options.versions.size() == 13 ? "\n(This round will <cr>count as a loss</c>!)" : "";
    createQuickPopup(
        "End Game?",
        "Are you sure you want to <cr>end the game</c>?" + warning,
        "No", "Yes",
        [this, doTheThing, pendingGuess](auto, bool btn2) {
            if (!btn2) return;
            doTheThing();
        }
    );
}

void GuessManager::safeAddLoadingLayer() {
    if (!loadingOverlay) {
        loadingOverlay = LoadingOverlayLayer::create();
        loadingOverlay->addToScene();
    } else {
        log::warn("Valid loadinglayer already exists");
    }
}

void GuessManager::safeRemoveLoadingLayer() {
    if (loadingOverlay) {
        loadingOverlay->removeMe();
        loadingOverlay = nullptr;
    }
}

std::vector<LeaderboardEntry> GuessManager::jsonToEntries(std::vector<matjson::Value> json) {
    std::vector<LeaderboardEntry> entries = {};

    for (auto lbEntry : json) {
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
            ENTRY_VALUE(leaderboard_position, int, asInt, INT32_MAX), // INT32_MAX is so that I know if smth goes wrong
        });

        #undef ENTRY_VALUE
    }

    return entries;
}

void GuessManager::getGuesses(int accountID, std::function<void(GuessesResponse)> callback, int page) {
    m_listener.bind([this, callback] (web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (res->code() != 200) {
                showError(fmt::format("error getting guesses; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string")), res->code());
                return;
            }

            auto jsonRes = res->json();
            if (jsonRes.isErr()) {
                showError(fmt::format("error getting json: {}", jsonRes.unwrapErr()), 1004);
                return;
            }

            auto json = jsonRes.unwrap();

            auto guessesResult = json["guesses"].asArray();
            if (guessesResult.isErr()) {
                showError(fmt::format("unable to get guesses: {}", guessesResult.unwrapErr()), 1104);
                return;
            }

            std::vector<GuessEntry> entries = {};

            for (auto lbEntry : guessesResult.unwrap()) {
                #define ENTRY_VALUE(key, return_type, func, default) \
                    .key = static_cast<return_type>(lbEntry[#key].func().unwrapOr(default))

                auto correct_date_split = geode::utils::string::split(lbEntry["correct_date"].asString().unwrapOr(""), "-");
                auto guessed_date_split = geode::utils::string::split(lbEntry["guessed_date"].asString().unwrapOr(""), "-");
                GuessEntry entry = {
                    ENTRY_VALUE(level_id, int, asInt, 0),
                    ENTRY_VALUE(account_id, int, asInt, 0),
                    ENTRY_VALUE(mode, GameMode, asInt, 0),
                    ENTRY_VALUE(score, int, asInt, 0),
                    .correct_date = {
                        .year = geode::utils::numFromString<int>(correct_date_split[0]).unwrapOr(0),
                        .month = geode::utils::numFromString<int>(correct_date_split[1]).unwrapOr(0),
                        .day = geode::utils::numFromString<int>(correct_date_split[2]).unwrapOr(0)
                    },
                    .guessed_date = {
                        .year = geode::utils::numFromString<int>(guessed_date_split[0]).unwrapOr(0),
                        .month = geode::utils::numFromString<int>(guessed_date_split[1]).unwrapOr(0),
                        .day = geode::utils::numFromString<int>(guessed_date_split[2]).unwrapOr(0)
                    },
                    ENTRY_VALUE(level_name, std::string, asString, "-"),
                    ENTRY_VALUE(level_creator, std::string, asString, "-"),
                };

                entries.push_back(entry);

                #undef ENTRY_VALUE
            }

            callback({
                .entries = entries,
                .page = static_cast<int>(json["page"].asInt().unwrapOr(0)),
                .total_pages = static_cast<int>(json["total_pages"].asInt().unwrapOr(0))
            });
        } else if (e->isCancelled()) {
            showError("request cancelled", 1303);
        }
    });

    auto req = web::WebRequest();
    setupRequest(req, matjson::makeObject({}));
    m_listener.setFilter(req.get(fmt::format("{}/guesses/{}?page={}", getServerUrl(), accountID, page))); 
}

void GuessManager::getLeaderboard(std::function<void(std::vector<LeaderboardEntry>)> callback) {
    safeAddLoadingLayer();
    updateStatusAndLoading(TaskStatus::GetLeaderboard);
    m_listener.bind([this, callback] (web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (res->code() != 200) {
                showError(fmt::format("error getting leaderboards; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string")), res->code());
                return;
            }

            auto jsonRes = res->json();
            if (jsonRes.isErr()) {
                showError(fmt::format("error getting json: {}", jsonRes.unwrapErr()), 1005);
                return;
            }

            auto json = jsonRes.unwrap();

            auto lbResult = json.asArray();
            if (lbResult.isErr()) {
                showError(fmt::format("unable to get scores: {}", lbResult.unwrapErr()), 1105);
                return;
            }
            auto leaderboardJson = lbResult.unwrap();
            safeRemoveLoadingLayer();
            callback(jsonToEntries(leaderboardJson));
        } else if (e->isCancelled()) {
            showError("request cancelled", 1303);
        }
    });

    auto req = web::WebRequest();
    setupRequest(req, matjson::makeObject({}));
    m_listener.setFilter(req.get(fmt::format("{}/leaderboard", getServerUrl())));
}

void GuessManager::getAccount(std::function<void(LeaderboardEntry)> callback, int accountID, std::string username) {
    safeAddLoadingLayer();
    updateStatusAndLoading(TaskStatus::LoadingAccount);
    m_listener.bind([this, callback] (web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            safeRemoveLoadingLayer();
            if (res->code() != 200 && res->code() != 404) {
                showError(fmt::format("error getting account; http code: {}, error: {}", res->code(), res->string().unwrapOr("unable to get error string")), res->code());
                return;
            }

            if (res->code() == 404) {
                LeaderboardEntry dummy {
                    0,
                    "Unknown",
                    0,
                    0,
                    0,
                    0,
                    0,
                    0.f,
                    0,
                    0,
                    0,
                    -1
                };
                callback(dummy);
                return;
            }

            auto jsonRes = res->json();
            if (jsonRes.isErr()) {
                showError(fmt::format("error getting json: {}", jsonRes.unwrapErr()), 1006);
                return;
            }

            auto json = jsonRes.unwrap();
            callback(jsonToEntries({ json })[0]);
        } else if (e->isCancelled()) {
            showError("request cancelled", 1303);
        }
    });

    auto req = web::WebRequest();
    std::string endpoint = "";
    if (accountID != 0) {
        endpoint = "account";
    } else if (!username.empty()) {
        endpoint = "accountByName";
    } else {
        log::error("there must either be an accountID or username while getting an account.");
        showError("no accountID or username provided while getting an account", 1107);
        return;
    }
    
    setupRequest(req, matjson::makeObject({}));
    log::info("{}/{}/{}", getServerUrl(), endpoint, (accountID == 0) ? username : std::to_string(accountID));
    m_listener.setFilter(req.get(fmt::format("{}/{}/{}", getServerUrl(), endpoint, (accountID == 0) ? username : std::to_string(accountID))));
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
    showError(fmt::format("unable to load level: {}, {}", p1, p0), 1201);
}

void GuessManager::levelDownloadFinished(GJGameLevel* level) {
    auto* glm = GameLevelManager::get();
    glm->m_levelDownloadDelegate = nullptr;

    reorderDownloadedLevel(level);
    safeRemoveLoadingLayer();

    this->realLevel = level;

    this->currentLevel = GJGameLevel::create();
    this->currentLevel->copyLevelInfo(level);

    this->currentLevel->m_levelName = "???????";
    this->currentLevel->m_creatorName = "GDGuesser";
    this->currentLevel->m_levelType = GJLevelType::Saved;

    auto levelDecoded = decodeBase64(this->currentLevel->m_levelString);
    this->currentLevel->m_levelString = encodeBase64(levelDecoded + "1,3854,2,-1000,3,1000,135,1;");

    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, LevelLayer::scene()));
}


void GuessManager::levelDownloadFailed(int x) {
    showError(fmt::format("could not fetch level, code {}", x), 1202);
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

    return 0;
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

        case TaskStatus::LoadingAccount:
            return "Loading Account"; break;

        default:
            return "Unknown"; break;
    }

    return "Unknown";
}

std::string GuessManager::verboseToSimple(int id, std::string error) {
    switch (id) {
        case -1:
            return "Unable to connect to GDGuesser server. Please try again later."; break;
        case 1001:
            return "Server returned invalid GDGuesser round."; break;
        case 1002:
            return "Server returned invalid account."; break;
        case 1003:
            return "Server returned invalid round results."; break;
        case 1004:
            return "Server returned invalid guess history."; break;
        case 1005:
            return "Server returned invalid leaderboard."; break;
        case 1006:
            return "Server returned invalid account stats."; break;
        case 1101:
            return "Unable to get level ID."; break;
        case 1102:
            return "Unable to get Argon auth token."; break;
        case 1103:
            return "Unable to get round score."; break;
        case 1104:
            return "Unable to get guess history."; break;
        case 1105:
            return "Unable to get leaderboard scores."; break;
        case 1106:
            return "Unable to get total score."; break;
        case 1107:
            return "Not logged into a Geometry Dash account"; break;
        case 1201:
            return "Unable to load level."; break;
        case 1202:
            return "Unable to download level."; break;
        case 1301:
            return error.substr(0, 200); break; //argon has clear enough error messages (except the stage 2 one) that in most cases passing it through is fine
        case 1302:
            return "Failed to send authentication message. Try deleting some of your sent GD messages."; break;
        case 1303:
            return "Request cancelled."; break;
        
    }

    return error.substr(0, 200);
}

std::string GuessManager::formatDate(LevelDate date) {
    switch (GuessManager::get().getDateFormat()) {
        case DateFormat::American: return fmt::format("{:02d}/{:02d}/{:04d}", date.month, date.day, date.year); break;
        case DateFormat::Backwards: return fmt::format("{:04d}/{:02d}/{:02d}", date.year, date.month, date.day); break;
        default: return fmt::format("{:02d}/{:02d}/{:04d}", date.day, date.month, date.year);
    }
}

std::string GuessManager::formatNumberWithCommas(int number) {
    std::string numStr = std::to_string(number);
    int insertPosition = numStr.length() - 3;

    while (insertPosition > 0) {
        numStr.insert(insertPosition, ",");
        insertPosition -= 3;
    }

    return numStr;
}
