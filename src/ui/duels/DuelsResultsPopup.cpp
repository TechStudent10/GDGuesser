#include "DuelsResultsPopup.hpp"

#include <managers/GuessManager.hpp>
#include <managers/net/manager.hpp>
#include <managers/net/client.hpp>
#include <managers/net/server.hpp>

class FinalPlayerStatNode : public CCNode {
public:
    static FinalPlayerStatNode* create(LeaderboardEntry player, int score, int totalScore, LevelDate guessed, bool reduced) {
        auto ret = new FinalPlayerStatNode;
        if (ret->init(player, score, totalScore, guessed, reduced)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
protected:
    bool init(LeaderboardEntry player, int score, int totalScore, LevelDate guessed, bool reduced) {
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

        this->addChildAtPosition(playerIcon, Anchor::Center, ccp(0.f, 30.f));

        auto playerName = CCLabelBMFont::create(player.username.c_str(), "goldFont.fnt");
        playerName->setScale(0.55f);
        this->addChildAtPosition(playerName, Anchor::Center, ccp(0.f, 5.f));

        auto roundScoreLabel = CCLabelBMFont::create(fmt::format("Round: {}", score).c_str(), "bigFont.fnt");
        roundScoreLabel->setScale(0.3f);
        this->addChildAtPosition(roundScoreLabel, Anchor::Center, ccp(0.f, -10.f));

        auto healthIcon = CCSprite::createWithSpriteFrameName("d_heart01_001.png");
        healthIcon->setScale(0.45f);
        this->addChildAtPosition(healthIcon, Anchor::Center, ccp(-30.f, -25.f));

        auto scoreLabel = CCLabelBMFont::create(std::to_string(totalScore).c_str(), "bigFont.fnt");
        scoreLabel->setScale(0.6f);
        this->addChildAtPosition(scoreLabel, Anchor::Center, ccp(0.f, -25.f));

        scoreLabel->setColor(
            {
                static_cast<GLubyte>(reduced ? 255 : 0),
                static_cast<GLubyte>(reduced ? 0 : 255),
                0
            }
        );

        auto guessedLabel = CCLabelBMFont::create(
            "Guessed:",
            "bigFont.fnt"
        );
        guessedLabel->setScale(0.4f);
        this->addChildAtPosition(guessedLabel, Anchor::Center, ccp(0.f, -45.f));
        guessedLabel->setID("result-guessed-label");

        auto guessedDateLabel = CCLabelBMFont::create(
            GuessManager::get().formatDate(guessed).c_str(),
            "bigFont.fnt"
        );
        guessedDateLabel->setScale(0.35f);
        guessedDateLabel->setColor({ 96, 171, 239 });
        this->addChildAtPosition(guessedDateLabel, Anchor::Center, ccp(0.f, -60.f));
        guessedDateLabel->setID("result-guessed-date-label");
        
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

    m_closeBtn->removeFromParent();

    auto resultBg = cocos2d::extension::CCScale9Sprite::create("square02b_001.png", { 0.0f, 0.0f, 80.0f, 80.0f });
    resultBg->setContentSize({340,148});
    resultBg->setColor(ccc3(0,0,0));
    resultBg->setOpacity(75);
    m_mainLayer->addChildAtPosition(resultBg, Anchor::Center);
    resultBg->setID("result-info-background");

    auto nameLabel = CCLabelBMFont::create(
        gm.realLevel->m_levelName.c_str(),
        "bigFont.fnt"
    );
    nameLabel->limitLabelWidth(160.f, 0.6f, 0.0f);
    m_mainLayer->addChildAtPosition(nameLabel, Anchor::Center, ccp(0.f, 60.f));
    nameLabel->setID("result-name-label");

    auto authorLabel = CCLabelBMFont::create(
        fmt::format("By {}", gm.realLevel->m_creatorName).c_str(),
        "goldFont.fnt"
    );
    authorLabel->limitLabelWidth(160.f, 0.5f, 0.0f);
    if (gm.realLevel->m_accountID == 0) {
        authorLabel->setColor({ 90, 255, 255 });
    }
    m_mainLayer->addChildAtPosition(authorLabel, Anchor::Center, ccp(0.f, 40.f));
    authorLabel->setID("result-author-label");

    auto difficultySprite = GJDifficultySprite::create(
        gm.getLevelDifficulty(gm.realLevel),
        static_cast<GJDifficultyName>(1)
    );
    difficultySprite->updateFeatureState(gm.getFeaturedState(gm.realLevel));
    m_mainLayer->addChildAtPosition(difficultySprite, Anchor::Center, ccp(0.f, 5.f));
    difficultySprite->setID("result-difficulty-sprite");

    auto starsLabel = CCLabelBMFont::create(
        fmt::format("{}", (gm.realLevel->m_stars).value()).c_str(),
        "bigFont.fnt"
    );
    auto starsIcon = CCSprite::createWithSpriteFrameName(
        gm.realLevel->m_levelLength == 5  ? "moon_small01_001.png" : "star_small01_001.png"
    );
    starsLabel->setScale(0.4f);
    starsLabel->setAnchorPoint({ 1.f, 0.5f });
    m_mainLayer->addChildAtPosition(starsLabel, Anchor::Center, ccp(0.f, gm.realLevel->m_demon > 0 ? -35.f : -25.f));
    m_mainLayer->addChildAtPosition(starsIcon, Anchor::Center, ccp(5.f, gm.realLevel->m_demon > 0 ? -35.f : -25.f));
    starsLabel->setID("result-stars-label");
    starsIcon->setID("result-stars-icon");

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
        results.totalScores[myIdStr],
        results.guessedDates[myIdStr],
        results.scores[myIdStr] < results.scores[oppIdStr]
    );
    m_mainLayer->addChildAtPosition(myNode, Anchor::Center, ccp(-95.f, 0.f));

    auto oppNode = FinalPlayerStatNode::create(
        oppEntry,
        results.scores[oppIdStr],
        results.totalScores[oppIdStr],
        results.guessedDates[oppIdStr],
        results.scores[oppIdStr] < results.scores[myIdStr]
    );
    m_mainLayer->addChildAtPosition(oppNode, Anchor::Center, ccp(95.f, 0.f));

    auto correctLabel = CCLabelBMFont::create(
        "Correct:",
        "bigFont.fnt"
    );
    correctLabel->setScale(0.4f);
    m_mainLayer->addChildAtPosition(correctLabel, Anchor::Center, ccp(0.f, -45.f));
    correctLabel->setID("result-correct-label");

    auto correctDateLabel = CCLabelBMFont::create(
        gm.formatDate(results.correctDate).c_str(),
        "bigFont.fnt"
    );
    correctDateLabel->setScale(0.35f);
    correctDateLabel->setColor({ 96, 171, 239 });
    m_mainLayer->addChildAtPosition(correctDateLabel, Anchor::Center, ccp(0.f, -60.f));
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
        gm.startGame(event.levelId, event.options);
    });

    return true;
}

void DuelsResultsPopup::onClose(CCObject* sender) {}
