#include "DuelsPopup.hpp"

#include <managers/GuessManager.hpp>
#include <managers/net/manager.hpp>
#include <managers/net/client.hpp>
#include <managers/net/server.hpp>

#include <ui/duels/DuelsPersistentNode.hpp>
#include <ui/duels/DuelsWinLayer.hpp>

class PlayerNode : public CCNode {
public:
    static PlayerNode* create(LeaderboardEntry player, bool ready) {
        auto ret = new PlayerNode;
        if (ret->init(player, ready)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
protected:
    bool init(LeaderboardEntry player, bool ready) {
        if (!CCNode::init()) return false;

        auto playerIcon = SimplePlayer::create(0);
        auto gm = GameManager::get();
        
        playerIcon->updatePlayerFrame(player.icon_id, IconType::Cube);
        playerIcon->setColor(gm->colorForIdx(player.color1));
        playerIcon->setSecondColor(gm->colorForIdx(player.color2));
        
        if (player.color3 == -1) {
            playerIcon->disableGlowOutline();
        } else {
            playerIcon->setGlowOutline(gm->colorForIdx(player.color3));
        }

        this->addChildAtPosition(playerIcon, Anchor::Center, ccp(0.f, 25.f));

        auto playerName = CCLabelBMFont::create(player.username.c_str(), "goldFont.fnt");
        playerName->setScale(0.5f);
        this->addChildAtPosition(playerName, Anchor::Center);

        auto scoreLabel = CCLabelBMFont::create(fmt::format("Score: {}", player.total_score).c_str(), "bigFont.fnt");
        scoreLabel->setScale(0.3f);
        this->addChildAtPosition(scoreLabel, Anchor::Center, ccp(0.f, -15.f));

        auto accuracyStat = (float)player.total_score / (float)player.max_score * 100.f;
        auto accuracyLabel = CCLabelBMFont::create(player.max_score > 0 ? fmt::format("Accuracy: {:.1f}%", (float)player.total_score / (float)player.max_score * 100.f).c_str() : "Accuracy: ???", "bigFont.fnt");
        accuracyLabel->setScale(0.3f);
        this->addChildAtPosition(accuracyLabel, Anchor::Center, ccp(0.f, -25.f));

        auto readyLabel = CCLabelBMFont::create(
            ready ? "Ready" : "Waiting...",
            "goldFont.fnt"
        );
        readyLabel->setScale(0.4f);
        readyLabel->setColor(
            {
                static_cast<GLubyte>(ready ? 0 : 255),
                static_cast<GLubyte>(ready ? 255 : 0),
                0
            }
        );
        this->addChildAtPosition(readyLabel, Anchor::Center, ccp(0.f, -40.f));

        return true;
    }
};

DuelsPopup* DuelsPopup::create() {
    auto ret = new DuelsPopup;
    if (ret->initAnchored(360.f, 225.f)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool DuelsPopup::setup() {
    this->setTitle("Duels vs. amoung pequeno");

    auto bg = cocos2d::extension::CCScale9Sprite::create("square02b_001.png", { 0.0f, 0.0f, 80.0f, 80.0f });
    bg->setContentSize({340,148});
    bg->setColor(ccc3(0,0,0));
    bg->setOpacity(75);
    m_mainLayer->addChildAtPosition(bg, Anchor::Center, ccp(0.f, 0.f));
    bg->setID("duels-info-background");

    auto separator = CCSprite::createWithSpriteFrameName("floorLine_001.png");
    separator->setScaleX(0.3f);
    separator->setScaleY(1);
    separator->setOpacity(100);
    separator->setRotation(90);
    separator->setID("duels-separator");
    m_mainLayer->addChildAtPosition(separator, Anchor::Center, ccp(0.f, 0.f));

    auto& gm = GuessManager::get();

    auto leftSpinner = LoadingSpinner::create(50.f);
    leftSpinner->setID("left-duels-spinner");
    m_mainLayer->addChildAtPosition(leftSpinner, Anchor::Center, ccp(-85.f, 0.f));

    auto rightSpinner = LoadingSpinner::create(50.f);
    rightSpinner->setID("right-duels-spinner");
    m_mainLayer->addChildAtPosition(rightSpinner, Anchor::Center, ccp(85.f, 0.f));

    auto spr = ButtonSprite::create("Ready Up");
    spr->setScale(0.75f);
    readyBtn = CCMenuItemExt::createSpriteExtra(
        spr, [](auto) {
            log::debug("this will toggle your ready state n' stuff");

            auto& nm = NetworkManager::get();
            ToggleReady ev = {};
            nm.send(ev);
        }
    );

    auto copySpr = ButtonSprite::create("Copy Code", "goldFont.fnt", "GJ_button_05.png", 1.f);
    copySpr->setScale(0.75f);
    auto copyBtn = CCMenuItemExt::createSpriteExtra(
        copySpr, [gm](auto) {
            geode::utils::clipboard::write(gm.duelJoinCode);
            Notification::create("Copied to clipboard!")->show();
        }
    );

    auto menu = CCMenu::create();
    menu->addChild(readyBtn);
    menu->addChild(copyBtn);
    menu->setLayout(
        RowLayout::create()
    );
    m_mainLayer->addChildAtPosition(menu, Anchor::Bottom, ccp(0.f, 20.f));

    auto& nm = NetworkManager::get();
    auto handlerIdx = nm.on<DuelUpdated>([this](DuelUpdated info) {
        updatePlayers(info.duel);
        auto& gm = GuessManager::get();
        gm.currentDuel = info.duel;
    });
    nm.on<RoundStarted>([this, handlerIdx](RoundStarted event) {
        auto& gm = GuessManager::get();
        gm.startGame(event.levelId, event.options);

        log::debug("starting, now trying to persist");
        
        if (!gm.persistentNode) {
            auto& nm = NetworkManager::get();
            nm.unbind<DuelUpdated>(handlerIdx);
    
            log::debug("unbound");
            
            auto persistentNode = DuelsPersistentNode::create();
            persistentNode->persist();
            gm.persistentNode = persistentNode;

            nm.on<DuelEnded>([](DuelEnded event) {
                auto& gm = GuessManager::get();
                gm.currentDuel = event.duel;
                gm.persistentNode->forget();
                gm.persistentNode = nullptr;
    
                CCDirector::sharedDirector()->pushScene(
                    CCTransitionFade::create(.5f, DuelsWinLayer::scene(
                        event.duel.players[std::to_string(event.winner)],
                        event.duel.players[std::to_string(event.loser)]
                    ))
                );

                auto& nm = NetworkManager::get();
                nm.disconnect();
            });
        }

        log::debug("persisted");

    });

    return true;
}

void DuelsPopup::updatePlayers(Duel duel) {
    if (getChildByIDRecursive("myNode")) myNode->removeFromParent();
    if (getChildByIDRecursive("oppNode")) oppNode->removeFromParent();
    if (auto label = getChildByIDRecursive("waiting-label")) label->removeFromParent();

    LeaderboardEntry myEntry;
    LeaderboardEntry oppEntry = { .account_id = 0 };
    for (auto entry : duel.players) {
        if (entry.second.account_id == GJAccountManager::get()->m_accountID) {
            myEntry = entry.second;
        } else {
            oppEntry = entry.second;
        }
    }

    if (getChildByIDRecursive("left-duels-spinner")) getChildByIDRecursive("left-duels-spinner")->removeFromParent();
    if (getChildByIDRecursive("right-duels-spinner")) getChildByIDRecursive("right-duels-spinner")->removeFromParent();

    myNode = PlayerNode::create(
        myEntry,
        // c++ is dumb
        std::find(duel.playersReady.begin(), duel.playersReady.end(), myEntry.account_id) != duel.playersReady.end()
    );
    myNode->setID("myNode");
    m_mainLayer->addChildAtPosition(myNode, Anchor::Center, ccp(-85.f, 0.f));

    if (oppEntry.account_id != 0) {
        oppNode = PlayerNode::create(
            oppEntry,
            // c++ is dumb
            std::find(duel.playersReady.begin(), duel.playersReady.end(), oppEntry.account_id) != duel.playersReady.end()
        );
        oppNode->setID("oppNode");
        m_mainLayer->addChildAtPosition(oppNode, Anchor::Center, ccp(85.f, 0.f));
        this->setTitle(fmt::format("Duels vs. {}", oppEntry.username));
    } else {
        auto label = CCLabelBMFont::create(
            "Waiting for P2....",
            "bigFont.fnt"
        );
        label->setOpacity(0.75 * 255);
        label->setScale(0.4f);
        label->setID("waiting-label");
        m_mainLayer->addChildAtPosition(label, Anchor::Center, ccp(85.f, 0.f));
        oppNode = nullptr;
        this->setTitle("Duels vs. ??????");
    }
}

void DuelsPopup::onClose(CCObject* sender) {
    geode::createQuickPopup(
        "Quit",
        "Are you sure you want to <cr>leave</c> the duel?",
        "No", "Yes",
        [this, sender](auto, bool btn2) {
            if (!btn2) return;

            auto& nm = NetworkManager::get();
            // server automatically handles leaving
            // we just need to disconnect :D
            nm.disconnect();
            Popup<>::onClose(sender);
        }
    );
}
