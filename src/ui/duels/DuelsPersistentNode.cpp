#include "DuelsPersistentNode.hpp"

#include <managers/GuessManager.hpp>

class PlayerScoreNode : public CCNode {
public:
    static PlayerScoreNode* create(LeaderboardEntry player, int health) {
        auto ret = new PlayerScoreNode;
        if (ret->init(player, health)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    void setGuessed(bool guessed) {
        if (guessed) {
            scoreLabel->setColor(
                {
                    0,
                    255,
                    0
                }
            );
        } else {
            scoreLabel->setColor(
                {
                    255,
                    255,
                    255
                }
            );
        }
    }

    void setHealth(int health) {
        scoreLabel->setString(std::to_string(health).c_str());
    }
protected:
    CCLabelBMFont* scoreLabel;
    bool init(LeaderboardEntry player, int health) {
        if (!CCNode::init()) return false;

        this->setContentWidth(50.f);

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

        this->addChildAtPosition(playerIcon, Anchor::Left);

        scoreLabel = CCLabelBMFont::create(fmt::format("{}", health).c_str(), "bigFont.fnt");
        scoreLabel->setScale(0.55f);
        this->addChildAtPosition(scoreLabel, Anchor::Right);

        return true;
    }
};

bool DuelsPersistentNode::init() {
    if (!CCNode::init()) return false;

    auto director = CCDirector::sharedDirector();
    auto screenSize = director->getWinSize();

    auto bg = cocos2d::extension::CCScale9Sprite::create("square02b_001.png", { 0.0f, 0.0f, 80.0f, 80.0f });
    bg->setContentSize({ 220, 190 });
    bg->setColor(ccc3(0,0,0));
    bg->setOpacity(75);
    this->addChildAtPosition(bg, Anchor::Center);

    this->setPosition({
        screenSize.width,
        screenSize.height
    });

    auto& nm = NetworkManager::get();
    GetDuel ev = {};
    nm.send(ev);
    log::debug("sent");

    nm.on<ReceiveDuel>([this](ReceiveDuel event) {
        log::debug("recing");
        updateDuel(event.duel);
        log::debug("recv'd");
    });

    nm.on<DuelUpdated>([this](DuelUpdated event) {
        log::debug("update start");
        updateDuel(event.duel);
        log::debug("updated");
        GuessManager::get().currentDuel = event.duel;
    });

    nm.on<OpponentGuessed>([this](auto) {
        oppNode->setGuessed(true);

        if (PlayLayer::get()) {
            auto sequence = CCSequence::create(
                CCCallFunc::create(this, callfunc_selector(DuelsPersistentNode::slideOn)),
                CCDelayTime::create(3.f),
                CCCallFunc::create(this, callfunc_selector(DuelsPersistentNode::slideOff)),
                nullptr
            );
            this->runAction(sequence);
        }
    });

    nm.on<DuelResults>([this](DuelResults event) {
        myNode->setGuessed(false);
        oppNode->setGuessed(false);

        LeaderboardEntry myEntry;
        LeaderboardEntry oppEntry;
        for (auto entry : GuessManager::get().currentDuel.players) {
            if (entry.second.account_id == GJAccountManager::get()->m_accountID) {
                myEntry = entry.second;
            } else {
                oppEntry = entry.second;
            }
        }

        myNode->setHealth(event.totalScores[std::to_string(myEntry.account_id)]);
        oppNode->setHealth(event.totalScores[std::to_string(oppEntry.account_id)]);
    });

    return true;
}

void DuelsPersistentNode::updateDuel(Duel duel) {
    log::debug("removing nodes");
    if (myNode) myNode->removeFromParent();
    log::debug("removed me node");
    if (oppNode) oppNode->removeFromParent();
    log::debug("removed they node");

    LeaderboardEntry myEntry;
    LeaderboardEntry oppEntry;
    for (auto entry : duel.players) {
        if (entry.second.account_id == GJAccountManager::get()->m_accountID) {
            myEntry = entry.second;
            log::debug("found me");
        } else {
            oppEntry = entry.second;
            log::debug("found them");
        }
    }

    myNode = PlayerScoreNode::create(myEntry, duel.scores[std::to_string(myEntry.account_id)]);
    myNode->setGuessed(false);
    this->addChildAtPosition(myNode, Anchor::BottomLeft, ccp(-80.f, -25.f));

    log::debug("me created");
    
    oppNode = PlayerScoreNode::create(oppEntry, duel.scores[std::to_string(oppEntry.account_id)]);
    oppNode->setGuessed(false);
    this->addChildAtPosition(oppNode, Anchor::BottomLeft, ccp(-80.f, -65.f));

    log::debug("opp created");
}

void DuelsPersistentNode::persist() {
    log::debug("persisting");
    auto sm = SceneManager::get();
    sm->keepAcrossScenes(this);
    this->setZOrder(9999);
    log::debug("persisted"); 
}

void DuelsPersistentNode::forget() {
    auto sm = SceneManager::get();
    sm->forget(this);
    this->removeFromParentAndCleanup(true);
}

void DuelsPersistentNode::slideOn() {
    auto director = CCDirector::sharedDirector();
    auto screenSize = director->getWinSize();

    auto action = CCEaseExponentialOut::create(
        CCMoveTo::create(0.5f, { screenSize.width, screenSize.height })
    );
    if (GuessManager::get().persistentNode) {
        this->stopAllActions();
        this->runAction(action);
    }
}

void DuelsPersistentNode::slideOff() {
    auto director = CCDirector::sharedDirector();
    auto screenSize = director->getWinSize();

    auto action = CCEaseExponentialIn::create(
        CCMoveTo::create(0.5f, { screenSize.width + MOVE_BY, screenSize.height })
    );
    if (GuessManager::get().persistentNode) {
        this->stopAllActions();
        this->runAction(action);
    }
}
