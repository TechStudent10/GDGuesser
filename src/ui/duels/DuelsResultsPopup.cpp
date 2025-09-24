#include "DuelsResultsPopup.hpp"
#include "Geode/cocos/actions/CCActionEase.h"
#include "Geode/cocos/actions/CCActionInterval.h"
#include "Geode/cocos/base_nodes/CCNode.h"
#include "Geode/cocos/cocoa/CCObject.h"
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include "ui/duels/DuelsPersistentNode.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/utils/cocos.hpp"

#include <managers/GuessManager.hpp>
#include <managers/net/manager.hpp>
#include <managers/net/client.hpp>
#include <managers/net/server.hpp>

class FinalPlayerStatNode : public CCNode {
public:
    static FinalPlayerStatNode* create(LeaderboardEntry player, int score, int oppScore, int totalScore, LevelDate guessed, bool reduced, bool alignment) {
        auto ret = new FinalPlayerStatNode;
        if (ret->init(player, score, oppScore, totalScore, guessed, reduced, alignment)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
    CCLabelBMFont* scoreLabel;

protected:
    bool init(LeaderboardEntry player, int score, int oppScore, int totalScore, LevelDate guessed, bool reduced, bool alignment) {
        if (!CCNode::init()) return false;

        auto playerIcon = SimplePlayer::create(0);
        auto gm = GameManager::get();
        
        playerIcon->updatePlayerFrame(player.icon_id, IconType::Cube);
        playerIcon->setColor(gm->colorForIdx(player.color1));
        playerIcon->setSecondColor(gm->colorForIdx(player.color2));
        playerIcon->setScale(0.8f);
        
        if (player.color3 == -1) {
            playerIcon->disableGlowOutline();
        } else {
            playerIcon->setGlowOutline(gm->colorForIdx(player.color3));
        }

        this->addChildAtPosition(playerIcon, Anchor::Center, ccp(alignment == false ? -25.f : 25.f, -20.f));

        auto playerName = CCLabelBMFont::create(player.username.c_str(), "goldFont.fnt");
        playerName->setScale(0.55f);
        this->addChildAtPosition(playerName, Anchor::Center, ccp(0.f, 20.f));

        scoreLabel = CCLabelBMFont::create(std::to_string(score > oppScore ? totalScore : totalScore + (oppScore - score)).c_str(), "bigFont.fnt");
        scoreLabel->setScale(0.5f);
        scoreLabel->setAnchorPoint({alignment == false ? 0.f : 1.0f, 0.5f});
        this->addChildAtPosition(scoreLabel, Anchor::Center, ccp(0.f, -20.f));
        float t = std::clamp(score > oppScore ? totalScore : totalScore + (oppScore - score) / 650.f, 0.f, 1.f);
        GLubyte red   = static_cast<GLubyte>((1.f - t) * 255);
        GLubyte green = static_cast<GLubyte>(t * 255);
        scoreLabel->setColor({red, green, 0});


        auto guessedDateLabel = CCLabelBMFont::create(
            GuessManager::get().formatDate(guessed).c_str(),
            "bigFont.fnt"
        );
        guessedDateLabel->setScale(0.30f);
        guessedDateLabel->setColor({ 96, 171, 239 });
        this->addChildAtPosition(guessedDateLabel, Anchor::Center, ccp(0.f, 5.f));
        guessedDateLabel->setID("result-guessed-date-label");

        auto separator = CCSprite::createWithSpriteFrameName("floorLine_001.png");
        separator->setScaleX(0.25f);
        separator->setScaleY(1);
        separator->setOpacity(100);
        this->addChildAtPosition(separator, Anchor::Center, ccp(0.f, -40.f));
        
        return true;
    }
};

DuelsResultsPopup* DuelsResultsPopup::create(DuelResults results) {
    auto ret = new DuelsResultsPopup;
    if (ret->initAnchored(360.f, 235.f, results)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool DuelsResultsPopup::setup(DuelResults results) {
    this->setTitle("Results");

    auto& gm = GuessManager::get();
    auto director = CCDirector::sharedDirector();
    auto size = director->getWinSize();
    
    gm.persistentNode->slideOff();

    m_closeBtn->removeFromParent();

    auto resultBg = cocos2d::extension::CCScale9Sprite::create("square02b_001.png", { 0.0f, 0.0f, 80.0f, 80.0f });
    resultBg->setContentSize({340,148});
    resultBg->setColor(ccc3(0,0,0));
    resultBg->setOpacity(75);
    m_mainLayer->addChildAtPosition(resultBg, Anchor::Center);
    resultBg->setID("result-info-background");

    auto level = CCNode::create();

    auto nameLabel = CCLabelBMFont::create(
        gm.realLevel->m_levelName.c_str(),
        "bigFont.fnt"
    );
    nameLabel->limitLabelWidth(160.f, 0.6f, 0.0f);
    nameLabel->setAnchorPoint({0.f, 0.5f});
    nameLabel->setID("result-name-label");

    auto authorLabel = CCLabelBMFont::create(
        gm.realLevel->m_creatorName.c_str(),
        "goldFont.fnt"
    );
    authorLabel->limitLabelWidth(160.f, 0.5f, 0.0f);
    if (gm.realLevel->m_accountID == 0) {
        authorLabel->setColor({ 90, 255, 255 });
    }
    authorLabel->setAnchorPoint({0.f, 0.5f});
    authorLabel->setID("result-author-label");

    auto difficultySprite = GJDifficultySprite::create(
        gm.getLevelDifficulty(gm.realLevel),
        static_cast<GJDifficultyName>(1)
    );
    difficultySprite->updateFeatureState(gm.getFeaturedState(gm.realLevel));
    difficultySprite->setScale(0.8f);
    difficultySprite->setID("result-difficulty-sprite");

    level->setContentSize({difficultySprite->getScaledContentSize().width + std::max(nameLabel->getScaledContentSize().width, authorLabel->getScaledContentSize().width) + 4.f, difficultySprite->getScaledContentSize().height});
    level->addChildAtPosition(difficultySprite, Anchor::Left, ccp(difficultySprite->getScaledContentSize().width / 2, 0.f));
    level->addChildAtPosition(nameLabel, Anchor::Left, ccp(difficultySprite->getScaledContentSize().width + 6.f, 8.f));
    level->addChildAtPosition(authorLabel, Anchor::Left, ccp(difficultySprite->getScaledContentSize().width + 6.f, -8.f));
    level->setAnchorPoint({0.5f, 0.5f});
    m_mainLayer->addChildAtPosition(level, Anchor::Center, ccp(0.f, 50.f));

    LeaderboardEntry myEntry;
    LeaderboardEntry oppEntry;
    for (auto entry : gm.currentDuel.players) {
        if (entry.second.account_id == GJAccountManager::get()->m_accountID) {
            myEntry = entry.second;
        } else {
            oppEntry = entry.second;
        }
    }

    auto myIdStr = std::to_string(myEntry.account_id);
    auto oppIdStr = std::to_string(oppEntry.account_id);

    auto myNode = FinalPlayerStatNode::create(
        myEntry,
        results.scores[myIdStr],
        results.scores[oppIdStr],
        results.totalScores[myIdStr],
        results.guessedDates[myIdStr],
        results.scores[myIdStr] < results.scores[oppIdStr],
        false
    );
    m_mainLayer->addChildAtPosition(myNode, Anchor::Center, ccp(-110.f, 0.f));

    auto oppNode = FinalPlayerStatNode::create(
        oppEntry,
        results.scores[oppIdStr],
        results.scores[myIdStr],
        results.totalScores[oppIdStr],
        results.guessedDates[oppIdStr],
        results.scores[oppIdStr] < results.scores[myIdStr],
        true
    );
    m_mainLayer->addChildAtPosition(oppNode, Anchor::Center, ccp(110.f, 0.f));

    int myInterval = results.scores[myIdStr] / 45;
    int oppInterval = results.scores[oppIdStr] / 45;

    int myTempScore = 0;
    int oppTempScore = 0;

    auto myRealScore = results.scores[myIdStr];
    auto oppRealScore = results.scores[oppIdStr];

    auto myRealTotalScore = results.totalScores[myIdStr];
    auto oppRealTotalScore = results.totalScores[oppIdStr];

    auto myScoreLabel = CCLabelBMFont::create("0", "bigFont.fnt");
    myScoreLabel->setScale(0.7f);
    m_mainLayer->addChildAtPosition(myScoreLabel, Anchor::Center, ccp(-110.f, -55.f));

    auto oppScoreLabel = CCLabelBMFont::create("0", "bigFont.fnt");
    oppScoreLabel->setScale(0.7f);
    m_mainLayer->addChildAtPosition(oppScoreLabel, Anchor::Center, ccp(110.f, -55.f));

    auto myGhostScoreLabel = CCLabelBMFont::create(fmt::format("{}", myRealScore).c_str(), "bigFont.fnt");
    myGhostScoreLabel->setScale(0.7f);
    myGhostScoreLabel->setOpacity(0);
    m_mainLayer->addChildAtPosition(myGhostScoreLabel, Anchor::Center, ccp(-110.f, -55.f));

    auto oppGhostScoreLabel = CCLabelBMFont::create(fmt::format("{}", oppRealScore).c_str(), "bigFont.fnt");
    oppGhostScoreLabel->setScale(0.7f);
    oppGhostScoreLabel->setOpacity(0);
    m_mainLayer->addChildAtPosition(oppGhostScoreLabel, Anchor::Center, ccp(110.f, -55.f));

    auto sequence = CCSequence::create(
        CCDelayTime::create(1.f),
        CCRepeat::create(
            CCSequence::create(
                CallFuncExt::create([myScoreLabel, oppScoreLabel, myTempScore, oppTempScore, myInterval, oppInterval, oppNode]() mutable {
                    myTempScore = myTempScore + myInterval;
                    oppTempScore = oppTempScore + oppInterval;
                    myScoreLabel->setString(fmt::format("{}", myTempScore).c_str());
                    oppScoreLabel->setString(fmt::format("{}", oppTempScore).c_str());
                }),
                CCDelayTime::create(0.02f),
                nullptr
            ),
            45
        ),
        CallFuncExt::create([myScoreLabel, oppScoreLabel, myRealScore, oppRealScore]() {
            myScoreLabel->setString(fmt::format("{}", myRealScore).c_str());
            oppScoreLabel->setString(fmt::format("{}", oppRealScore).c_str());
        }),
        CCDelayTime::create(0.7f),
        CallFuncExt::create([myRealScore, oppRealScore, myScoreLabel, oppScoreLabel, myGhostScoreLabel, oppGhostScoreLabel, oppNode, myNode, myRealTotalScore, oppRealTotalScore]() {
            if(myRealScore > oppRealScore) {
                auto sequence = CCSequence::create(
                    CCEaseBackOut::create(CCScaleBy::create(0.5f, 1.2f)),
                    CCDelayTime::create(0.3f),
                    CCEaseExponentialIn::create(
                        CCMoveTo::create(
                            0.25f,
                            oppScoreLabel->getPosition()
                        )
                    ),
                    CallFuncExt::create([oppScoreLabel, myRealScore, oppRealScore, oppGhostScoreLabel, oppNode, myNode, oppRealTotalScore]() {
                        oppScoreLabel->setString(fmt::format("{}", myRealScore - oppRealScore).c_str());
                        oppScoreLabel->runAction(CCSequence::create(
                            CCDelayTime::create(0.7f),
                            CCEaseExponentialIn::create(
                                CCMoveBy::create(0.3f, {0, 20})
                            ),
                            nullptr
                        ));
                        oppScoreLabel->runAction(CCSequence::create(
                            CCDelayTime::create(0.8f),
                            CCFadeOut::create(0.2f),
                            nullptr
                        ));
                        oppGhostScoreLabel->runAction(CCSequence::create(
                            CCDelayTime::create(0.8f),
                            CCEaseExponentialIn::create(CCFadeTo::create(0.2, 255/2)),
                            nullptr
                        ));
                        int interval = (myRealScore - oppRealScore) / 10;
                        int tempScore = oppRealTotalScore + (myRealScore - oppRealScore);
                        oppNode->runAction(CCSequence::create(
                            CCDelayTime::create(0.8f),
                            CCRepeat::create(CCSequence::create(
                                CallFuncExt::create([oppNode, interval, tempScore]() mutable {
                                    tempScore = tempScore - interval;
                                    oppNode->scoreLabel->setString(fmt::format("{}", tempScore).c_str());
                                    float t = std::clamp(tempScore / 650.f, 0.f, 1.f);
                                    GLubyte red   = static_cast<GLubyte>((1.f - t) * 255);
                                    GLubyte green = static_cast<GLubyte>(t * 255);
                                    oppNode->scoreLabel->setColor({red, green, 0});
                                }),
                                CCDelayTime::create(0.05f),
                                nullptr
                            ), 10),
                            CallFuncExt::create([oppNode, oppRealTotalScore]() {
                                oppNode->scoreLabel->setString(fmt::format("{}", oppRealTotalScore).c_str());
                            }),
                            nullptr
                        ));
                    }),
                    nullptr
                );
                myScoreLabel->runAction(CCSequence::create(
                    CCDelayTime::create(1.0f),
                    CCFadeOut::create(0.2f),
                    nullptr
                ));
                myScoreLabel->runAction(sequence);
                myGhostScoreLabel->runAction(CCSequence::create(
                    CCDelayTime::create(1.f),
                    CCEaseExponentialIn::create(CCFadeTo::create(0.2, 255/2)),
                    nullptr
                ));
            } else if (oppRealScore > myRealScore) {
                auto sequence = CCSequence::create(
                    CCEaseBackOut::create(CCScaleBy::create(0.5f, 1.2f)),
                    CCDelayTime::create(0.3f),
                    CCEaseExponentialIn::create(
                        CCMoveTo::create(
                            0.25f,
                            myScoreLabel->getPosition()
                        )
                    ),
                    CallFuncExt::create([myScoreLabel, myRealScore, oppRealScore, myGhostScoreLabel, myNode, myRealTotalScore]() {
                        myScoreLabel->setString(fmt::format("{}", oppRealScore - myRealScore).c_str());
                        myScoreLabel->runAction(CCSequence::create(
                            CCDelayTime::create(0.7f),
                            CCEaseExponentialIn::create(
                                CCMoveBy::create(0.3f, {0, 20})
                            ),
                            nullptr
                        ));
                        myScoreLabel->runAction(CCSequence::create(
                            CCDelayTime::create(0.8f),
                            CCFadeOut::create(0.2f),
                            nullptr
                        ));
                        myGhostScoreLabel->runAction(CCSequence::create(
                            CCDelayTime::create(0.8f),
                            CCEaseExponentialIn::create(CCFadeTo::create(0.2, 255/2)),
                            nullptr
                        ));
                        int interval = (oppRealScore - myRealScore) / 10;
                        int tempScore = myRealTotalScore + (oppRealScore - myRealScore);
                        myNode->runAction(CCSequence::create(
                            CCDelayTime::create(0.8f),
                            CCRepeat::create(CCSequence::create(
                                CallFuncExt::create([myNode, interval, tempScore]() mutable {
                                    tempScore = tempScore - interval;
                                    myNode->scoreLabel->setString(fmt::format("{}", tempScore).c_str());
                                    float t = std::clamp(tempScore / 650.f, 0.f, 1.f);
                                    GLubyte red   = static_cast<GLubyte>((1.f - t) * 255);
                                    GLubyte green = static_cast<GLubyte>(t * 255);
                                    myNode->scoreLabel->setColor({red, green, 0});
                                }),
                                CCDelayTime::create(0.05f),
                                nullptr
                            ), 10),
                            CallFuncExt::create([myNode, myRealTotalScore]() {
                                myNode->scoreLabel->setString(fmt::format("{}", myRealTotalScore).c_str());
                            }),
                            nullptr
                        ));
                    }),
                    nullptr
                );
                oppScoreLabel->runAction(CCSequence::create(
                    CCDelayTime::create(1.0f),
                    CCFadeOut::create(0.2f),
                    nullptr
                ));
                oppScoreLabel->runAction(sequence);
                oppGhostScoreLabel->runAction(CCSequence::create(
                    CCDelayTime::create(1.f),
                    CCEaseExponentialIn::create(CCFadeTo::create(0.2, 255/2)),
                    nullptr
                ));
            }
        }),
        nullptr
    );

    this->runAction(sequence);


    auto correctLabel = CCLabelBMFont::create(
        "Correct:",
        "bigFont.fnt"
    );
    correctLabel->setScale(0.4f);
    m_mainLayer->addChildAtPosition(correctLabel, Anchor::Center, ccp(0.f, 10.f));
    correctLabel->setID("result-correct-label");

    auto correctDateLabel = CCLabelBMFont::create(
        gm.formatDate(results.correctDate).c_str(),
        "bigFont.fnt"
    );
    correctDateLabel->setScale(0.35f);
    correctDateLabel->setColor({ 96, 171, 239 });
    m_mainLayer->addChildAtPosition(correctDateLabel, Anchor::Center, ccp(0.f, 0.f));
    correctDateLabel->setID("result-correct-date-label");

    auto readyUpSpr = ButtonSprite::create("Ready Up", "goldFont.fnt", "GJ_button_01.png", 0.9);
    readyUpSpr->setScale(0.8f);
    auto readyUpBtn = CCMenuItemExt::createSpriteExtra(readyUpSpr, [](CCObject*) {
        ToggleReady ev = {};
        auto& nm = NetworkManager::get();
        nm.send(ev);
    });

    auto forfeitSpr = ButtonSprite::create("Forfeit", "goldFont.fnt", "GJ_button_06.png", 0.9);
    forfeitSpr->setScale(0.8f);
    auto forfeitBtn = CCMenuItemExt::createSpriteExtra(forfeitSpr, [](CCObject*) {
        createQuickPopup(
            "End Game?",
            "Are you sure you want to <cr>forfeit the duel</c>?",
            "No", "Yes",
            [](auto, bool btn2) {
                if (!btn2) return;
                Forfeit ev = {};
                auto& nm = NetworkManager::get();
                nm.send(ev);
            }
        );
    });
    
    auto nextRoundMenu = CCMenu::create();
    nextRoundMenu->setContentWidth(m_mainLayer->getContentWidth());
    nextRoundMenu->addChild(forfeitBtn);
    nextRoundMenu->addChild(readyUpBtn);

    nextRoundMenu->setLayout(
        RowLayout::create()
    );

    m_mainLayer->addChildAtPosition(nextRoundMenu, Anchor::Bottom, ccp(0.f, 25.f));

    auto& nm = NetworkManager::get();
    // nm.on<DuelUpdated>([this](DuelUpdated info) {
    //     updatePlayers(info.duel);
    //     auto& gm = GuessManager::get();
    //     gm.currentDuel = info.duel;
    // });
    nm.on<RoundStarted>([this](RoundStarted event) {
        auto& gm = GuessManager::get();
        gm.persistentNode->slideOn();
        gm.startGame(event.levelId, event.options);
    });

    return true;
}

void DuelsResultsPopup::onClose(CCObject* sender) {}
