#include "DuelsWinLayer.hpp"
#include <Geode/binding/GameManager.hpp>
#include <managers/GuessManager.hpp>

class FinalPlayerStatNode : public CCNode {
public:
    static FinalPlayerStatNode* create(LeaderboardEntry player, bool winner) {
        auto ret = new FinalPlayerStatNode;
        if (ret->init(player, winner)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
protected:
    bool init(LeaderboardEntry player, bool winner) {
        if (!CCNode::init()) return false;

        auto trophyIcon = CCSprite::createWithSpriteFrameName(
            winner ? "rankIcon_1_001.png" : "rankIcon_top50_001.png"
        );
        trophyIcon->setScale(0.8f);
        this->addChildAtPosition(trophyIcon, Anchor::Center, ccp(0.f, 50.f));

        auto playerIcon = SimplePlayer::create(0);
        auto gam = GameManager::get();
        
        playerIcon->updatePlayerFrame(player.icon_id, IconType::Cube);
        playerIcon->setColor(gam->colorForIdx(player.color1));
        playerIcon->setSecondColor(gam->colorForIdx(player.color2));
        
        if (player.color3 == -1) {
            playerIcon->disableGlowOutline();
        } else {
            playerIcon->setGlowOutline(gam->colorForIdx(player.color3));
        }

        this->addChildAtPosition(playerIcon, Anchor::Center, ccp(0.f, 15.f));

        auto playerName = CCLabelBMFont::create(player.username.c_str(), "goldFont.fnt");
        playerName->setScale(0.5f);
        if (!winner) {
            playerName->setTexture(CCTextureCache::get()->addImage("silverFont.png"_spr, false));
        }
        this->addChildAtPosition(playerName, Anchor::Center, ccp(0.f, -10.f));

        auto& gm = GuessManager::get();

        auto scoreLabel = CCLabelBMFont::create(fmt::format("Final Health: {}", gm.currentDuel.scores[std::to_string(player.account_id)]).c_str(), "bigFont.fnt");
        scoreLabel->setScale(0.4f);
        this->addChildAtPosition(scoreLabel, Anchor::Center, ccp(0.f, -30.f));

        return true;
    }
};

class RepeatingBackground : public CCNode {
public:
    static RepeatingBackground* create(float speed = 1.f) {
        auto ret = new RepeatingBackground;
        if (ret->init(speed)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    void pulseTo(ccColor3B color, float enterDuration, float staticDuration, float exitDuration) {
        for (auto bg : bgs) {
            bg->runAction(
                CCSequence::create(
                    CCEaseIn::create(
                        CCTintTo::create(enterDuration, color.r, color.g, color.b),
                        0.5f
                    ),
                    CCDelayTime::create(staticDuration),
                    CCEaseOut::create(
                        CCTintTo::create(exitDuration, COLOR.r, COLOR.g, COLOR.b),
                        0.5f
                    ),
                    nullptr
                )
            );
        }
    }
protected:
    std::vector<Ref<CCSprite>> bgs;
    float speed;

    static constexpr ccColor3B COLOR = {
        0,
        0,
        255
    };

    bool init(float speed) {
        if (!CCNode::init()) return false;

        this->speed = speed;
        
        auto director = CCDirector::sharedDirector();
        auto winSize = director->getWinSize();

        for (int i = 0; i < 3; i++) {
            auto bg = CCSprite::create("game_bg_01_001.png");
            bg->setScale(
                winSize.height / bg->getContentHeight()
            );

            bg->setColor(COLOR);
            bg->setPosition({ i * bg->getScaledContentWidth(), 0 });
            bg->setAnchorPoint({ 0, 0 });

            this->addChild(bg);
            bgs.push_back(bg);
        }

        this->schedule(schedule_selector(RepeatingBackground::updateBG));

        return true;
    }

    void updateBG(float dt) {
        CCSprite* exiting = nullptr;
        float farthest = 0;
        for (auto bg : bgs) {
            // dt * speed * width / 12 (shoutout dankmeme01 for this formula)
            float moveByX = dt * speed * bg->getScaledContentWidth() / 12.f;
            float newX = bg->getPositionX() - moveByX;
            
            if (newX <= -bg->getScaledContentWidth()) {
                exiting = bg;
            }

            if (newX > farthest) {
                farthest = newX;
            }

            bg->setPositionX(newX);
        }
        if (exiting) {
            exiting->setPositionX(farthest + exiting->getScaledContentWidth());
        }
    }

};

DuelsWinLayer* DuelsWinLayer::create(LeaderboardEntry winner, LeaderboardEntry loser) {
    auto ret = new DuelsWinLayer;
    if (ret->init(winner, loser)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

CCScene* DuelsWinLayer::scene(LeaderboardEntry winner, LeaderboardEntry loser) {
    auto layer = DuelsWinLayer::create(winner, loser);
    auto scene = CCScene::create();
    scene->addChild(layer);
    return scene;
}

bool DuelsWinLayer::init(LeaderboardEntry winner, LeaderboardEntry loser) {
    if (!CCLayer::init()) return false;

    auto director = CCDirector::sharedDirector();
    auto winSize = director->getWinSize();

    this->winner = winner;
    this->loser = loser;

    auto fmodEngine = FMODAudioEngine::sharedEngine();
    fmodEngine->stopAllMusic(false);
    auto system = fmodEngine->m_system;

    FMOD::Channel* channel;
    FMOD::Sound* sound;

    system->createSound((Mod::get()->getResourcesDir() / "duels-end.mp3").string().c_str(), FMOD_DEFAULT, nullptr, &sound);
    system->playSound(sound, nullptr, false, &channel);
    
    auto bg = RepeatingBackground::create();
    this->addChild(bg);

    auto winLabel = CCLabelBMFont::create(
        fmt::format("{} wins!", winner.username).c_str(), "goldFont.fnt"
    );
    winLabel->setPosition(winSize / 2);
    winLabel->setScale(0.f);
    this->addChild(winLabel);

    auto finalStatsNode = CCNode::create();
    finalStatsNode->setPosition({
        winSize.width / 2,
        2000
    });

    auto finalStatsNodeBg = cocos2d::extension::CCScale9Sprite::create("square02b_001.png", { 0.0f, 0.0f, 80.0f, 80.0f });
    finalStatsNodeBg->setContentSize({340,148});
    finalStatsNodeBg->setColor(ccc3(0,0,0));
    finalStatsNodeBg->setOpacity(75);
    finalStatsNode->addChild(finalStatsNodeBg);

    auto winnerStatsNode = FinalPlayerStatNode::create(winner, true);
    finalStatsNode->addChildAtPosition(winnerStatsNode, Anchor::Center, ccp(-85.f, 0.f));

    auto loserStatsNode = FinalPlayerStatNode::create(loser, false);
    finalStatsNode->addChildAtPosition(loserStatsNode, Anchor::Center, ccp(85.f, 0.f));

    auto separator = CCSprite::createWithSpriteFrameName("floorLine_001.png");
    separator->setScaleX(0.3f);
    separator->setScaleY(1);
    separator->setOpacity(100);
    separator->setRotation(90);
    separator->setID("duels-separator");
    finalStatsNode->addChildAtPosition(separator, Anchor::Center, ccp(0.f, 0.f));

    this->addChild(finalStatsNode);

    auto thxLabel = CCLabelBMFont::create("Thanks for playing!", "goldFont.fnt");
    thxLabel->setPosition({
        winSize.width / 2,
        2000
    });
    thxLabel->setScale(0.8f);
    this->addChild(thxLabel);

    auto& gm = GuessManager::get();

    if (gm.realLevel && Mod::get()->getSettingValue<bool>("dont-save-levels")) {
        GameLevelManager::get()->deleteLevel(gm.realLevel);
        gm.realLevel = nullptr;
    }
    gm.currentLevel = nullptr;

    auto continueBtn = CCMenuItemExt::createSpriteExtra(
        ButtonSprite::create("Continue"),
        [channel](auto) {
            channel->stop();

            auto layer = CreatorLayer::create();
            auto scene = CCScene::create();
            scene->addChild(layer);
            CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));

            GameManager::get()->fadeInMenuMusic();
        }
    );

    auto continueMenu = CCMenu::create();
    continueMenu->setContentSize(continueBtn->getContentSize());
    continueMenu->addChild(continueBtn);
    continueMenu->setPosition({
        winSize.width / 2,
        -2000
    });
    this->addChild(continueMenu);

    auto animation = CCSequence::create(
        CCDelayTime::create(1.75f),
        CallFuncExt::create([this, winLabel, bg]() {
            bg->pulseTo(
                {
                    255,
                    255,
                    255
                },
                0,
                0,
                0.5f
            );
            winLabel->runAction(
                CCEaseBackInOut::create(
                    CCScaleTo::create(0.5f, 1.f)
                )
            );
            // constant pulse
            this->runAction(
                CCRepeatForever::create(
                    CCSequence::create(
                        CallFuncExt::create([bg]() {
                            bg->pulseTo(
                                {
                                    50,
                                    50,
                                    255
                                },
                                0.f,
                                0.f,
                                0.5f
                            );
                        }),
                        CCDelayTime::create(1.1f),
                        nullptr
                    )
                )
            );
        }),
        CCDelayTime::create(3.5f),
        CallFuncExt::create([winLabel, finalStatsNode, winSize]() {
            winLabel->runAction(
                CCEaseBackInOut::create(
                    CCScaleTo::create(0.5f, 0.f)
                )
            );

            finalStatsNode->runAction(
                CCSequence::create(
                    CCDelayTime::create(0.65f),
                    CCEaseExponentialOut::create(
                        CCMoveTo::create(
                            0.5f,
                            winSize / 2
                        )
                    ),
                    nullptr
                )
            );
        }),
        CCDelayTime::create(4.f),
        CallFuncExt::create([thxLabel, continueMenu, winSize]() {
            thxLabel->runAction(
                CCEaseExponentialOut::create(
                    CCMoveTo::create(
                        0.5f,
                        {
                            winSize.width / 2,
                            winSize.height / 2 + 95
                        }
                    )
                )
            );

            continueMenu->runAction(
                CCEaseExponentialOut::create(
                    CCMoveTo::create(
                        0.5f,
                        {
                            winSize.width / 2,
                            winSize.height / 2 - 95
                        }
                    )
                )
            );
        }),
        nullptr
    );
    this->runAction(animation);
    gm.safeRemoveLoadingLayer();


    return true;
}
