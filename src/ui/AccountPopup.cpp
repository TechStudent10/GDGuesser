#include "AccountPopup.hpp"
#include <managers/GuessManager.hpp>

class HistoryCell : public CCNode {
protected:
    bool init(GuessEntry guessEntry, int index, float width) {
        if (!CCNode::init())
            return false;

        this->setContentSize({ width, CELL_HEIGHT });
        auto sprite = guessEntry.mode == GameMode::Normal ? "diffIcon_01_btn_001.png" : "diffIcon_05_btn_001.png";
        auto diffLabelText = guessEntry.mode == GameMode::Normal ? "Normal" : "Hardcore";

        auto icon = CCScale9Sprite::createWithSpriteFrameName(sprite);
        icon->setScale(0.8f);
        this->addChildAtPosition(icon, Anchor::Left, ccp(25.f, 5));

        auto diffLabel = CCLabelBMFont::create(diffLabelText, "bigFont.fnt");
        diffLabel->setScale(0.25f);
        this->addChildAtPosition(diffLabel, Anchor::Left, ccp(25.f, -15));

        auto nameLabel = CCLabelBMFont::create(guessEntry.level_name.c_str(), "bigFont.fnt");
        nameLabel->limitLabelWidth(100.f, .4f, .0f);
        nameLabel->setAnchorPoint({0.0f, 0.5f});
        this->addChildAtPosition(nameLabel, Anchor::Top, ccp(-70.f, -10.f));

        auto infoGrid = CCNode::create();
        infoGrid->setContentSize({width * 0.35f, 12.5f});

        int maxScore;
        switch(guessEntry.mode) {
            case GameMode::Normal: maxScore = 500.f; break;
            case GameMode::Hardcore: maxScore = 600.f; break;
            default: 500;
        }

        auto gm = GuessManager::get();
        std::string submittedDate = gm.formatDate(guessEntry.guessed_date);
        std::string correctDateFormatted = gm.formatDate(guessEntry.correct_date);

        float t = std::clamp(static_cast<float>(guessEntry.score) / maxScore, 0.f, 1.f);
        GLubyte red   = static_cast<GLubyte>((1.f - t) * 255);
        GLubyte green = static_cast<GLubyte>(t * 255);
        cocos2d::ccColor3B color = { red, green, 0 };
        #define STAT(sprite, spriteScale, value, anchor) \
        { \
            auto item = CCNode::create(); \
            \
            auto icon = CCSprite::createWithSpriteFrameName(sprite); \
            icon->setScale(spriteScale); \
            \
            auto label = CCLabelBMFont::create(value, "bigFont.fnt"); \
            label->setScale(0.3f); \
            label->setAnchorPoint({0.f, 0.4f}); \
            if(anchor == Anchor::TopRight || anchor == Anchor::BottomRight) label->setColor(color); \
            \
            item->setContentSize({icon->getScaledContentSize().width + label->getScaledContentSize().width + 4.f, icon->getScaledContentSize().height}); \
            item->addChildAtPosition(icon, Anchor::Left, ccp(icon->getScaledContentSize().width / 2, 0.f)); \
            item->addChildAtPosition(label, Anchor::Left, ccp(icon->getScaledContentSize().width + 4.f, 0.f)); \
            \
            item->setAnchorPoint({0.f, 0.5f}); \
            infoGrid->addChildAtPosition(item, anchor, ccp(0.f, 0.f)); \
        }

        STAT("GJ_completesIcon_001.png", 0.4f, correctDateFormatted.c_str(), Anchor::TopLeft);
        STAT("GJ_starsIcon_001.png", 0.5f, std::to_string(guessEntry.score).c_str(), Anchor::TopRight);
        STAT("GJ_filterIcon_001.png", 0.4f, submittedDate.c_str(), Anchor::BottomLeft);
        STAT("GJ_sTrendingIcon_001.png", 0.6f, fmt::format("{:.1f}%", (float)guessEntry.score / (float)maxScore * 100.f).c_str(), Anchor::BottomRight);

        #undef STAT

        infoGrid->setAnchorPoint({0.f, 0.5f});
        infoGrid->setPosition({nameLabel->getPositionX(), 15.f});
        this->addChild(infoGrid);

        auto authorLabel = CCLabelBMFont::create(guessEntry.level_creator.c_str(), "goldFont.fnt");
        authorLabel->limitLabelWidth(80.f, .35f, .0f);
        authorLabel->setAnchorPoint({0.0f, 0.5f});
        authorLabel->setPosition(nameLabel->getPositionX() + nameLabel->getScaledContentWidth() + 5.f, nameLabel->getPositionY());
        this->addChild(authorLabel);

        auto level = GameLevelManager::get()->getSavedLevel(guessEntry.level_id);

        if (level) {
            auto menu = CCMenu::create();
            menu->setPosition(CCPointZero);
            menu->setContentSize(this->getContentSize());
            menu->setAnchorPoint({ 0.f, 0.f });
            menu->ignoreAnchorPointForPosition(false);

            auto viewBtn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("View"), [level](CCObject*) {
                auto layer = LevelInfoLayer::create(level, false);
                auto scene = CCScene::create();
                scene->addChild(layer);
                CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
            });
            viewBtn->setScale(0.6f);
            viewBtn->m_baseScale = 0.6f;
            viewBtn->setPosition({ this->getContentSize().width - 30.f, infoGrid->getPositionY() });

            menu->addChild(viewBtn);
            this->addChild(menu);
        }
        
        this->setAnchorPoint({ 0.f, 0.5f });

        return true;
    }

public:
    static constexpr float CELL_HEIGHT = 45.f;

    static HistoryCell* create(GuessEntry guessEntry, int index, float width) {
        auto ret = new HistoryCell;
        if (ret->init(guessEntry, index, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

AccountPopup* AccountPopup::create(LeaderboardEntry user) {
    auto ret = new AccountPopup;
    if (ret->initAnchored(300.f, 250.f, user)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool AccountPopup::setup(LeaderboardEntry user) {
    auto username = CCNode::create();
    auto player = SimplePlayer::create(0);
    auto gm = GameManager::get();
    auto gum = GuessManager::get();
    
    player->updatePlayerFrame(user.icon_id, IconType::Cube);
    player->setColor(gm->colorForIdx(user.color1));
    player->setSecondColor(gm->colorForIdx(user.color2));
    player->setScale(.7f);
    
    if (user.color3 == -1) {
        player->disableGlowOutline();
    } else {
        player->setGlowOutline(gm->colorForIdx(user.color3));
    }
    auto theB = CCLabelBMFont::create(user.username.c_str(), "bigFont.fnt");
    theB->limitLabelWidth(150.f, 0.75f, 0.0f);
    theB->setAnchorPoint({1.f, 0.5f});
    username->setContentSize({theB->getScaledContentWidth() + 20.f, theB->getScaledContentHeight()});
    username->addChildAtPosition(theB, Anchor::Right);
    username->addChildAtPosition(player, Anchor::Left);
    username->setAnchorPoint({0.5f, 0.5f});
    m_mainLayer->addChildAtPosition(username, Anchor::Top, ccp(0.f, -20.f));

    auto separator = CCSprite::createWithSpriteFrameName("floorLine_001.png");
    separator->setScaleX(0.60f);
    separator->setScaleY(1);
    separator->setOpacity(100);

    m_mainLayer->addChildAtPosition(separator, Anchor::Top, ccp(0.f, -35.f));

    auto infoRow = CCNode::create();
    infoRow->setLayout(RowLayout::create()
        ->setGap(20.f)
        ->setAxisAlignment(AxisAlignment::Center)
    );

    #define STAT(sprite, spriteScale, value) \
    { \
        auto item = CCNode::create(); \
        \
        auto icon = CCSprite::createWithSpriteFrameName(sprite); \
        icon->setScale(spriteScale); \
        \
        auto label = CCLabelBMFont::create(value, "bigFont.fnt"); \
        label->setScale(0.5f); \
        label->setAnchorPoint({0.f, 0.4f}); \
        \
        item->setContentSize({icon->getScaledContentSize().width + label->getScaledContentSize().width + 4.f, icon->getScaledContentSize().height}); \
        item->addChildAtPosition(icon, Anchor::Left, ccp(icon->getScaledContentSize().width / 2, 0.f)); \
        item->addChildAtPosition(label, Anchor::Left, ccp(icon->getScaledContentSize().width + 4.f, 0.f)); \
        \
        infoRow->addChild(item); \
    }

    if(user.leaderboard_position == 1) { STAT("rankIcon_1_001.png", 0.6f, std::to_string(user.leaderboard_position).c_str()); } else
    if(user.leaderboard_position <= 3) { STAT("rankIcon_top10_001.png", 0.6f, std::to_string(user.leaderboard_position).c_str()); } else
    if(user.leaderboard_position <= 10) { STAT("rankIcon_top50_001.png", 0.6f, std::to_string(user.leaderboard_position).c_str()); } else
    if(user.leaderboard_position <= 25) { STAT("rankIcon_top500_001.png", 0.75f, std::to_string(user.leaderboard_position).c_str()); } else
    if(user.leaderboard_position <= 50) { STAT("rankIcon_top200_001.png", 0.75f, std::to_string(user.leaderboard_position).c_str()); } else
    if(user.leaderboard_position <= 100) { STAT("rankIcon_top1000_001.png", 0.85f, std::to_string(user.leaderboard_position).c_str()); } else
    { STAT("rankIcon_all_001.png", 1.f, gum.formatNumberWithCommas(user.leaderboard_position).c_str()); }
    STAT("GJ_starsIcon_001.png", 0.6f, gum.formatNumberWithCommas(user.total_score).c_str());
    STAT("GJ_sTrendingIcon_001.png", 0.85f, fmt::format("{:.1f}%", (float)user.total_score / (float)user.max_score * 100.f).c_str());

    #undef STAT

    infoRow->setContentSize({275.f, 40.f});
    infoRow->updateLayout();
    infoRow->setAnchorPoint({0.5f, 0.5f});
    m_mainLayer->addChildAtPosition(infoRow, Anchor::Center, ccp(0.f, 75.f));

    auto difficultyBg = cocos2d::extension::CCScale9Sprite::create("square02b_001.png", { 0.0f, 0.0f, 80.0f, 80.0f });
    difficultyBg->setContentSize({240,60});
    difficultyBg->setColor(ccc3(0,0,0));
    difficultyBg->setOpacity(75);
    m_mainLayer->addChildAtPosition(difficultyBg, Anchor::Center, ccp(0.f, 30.f));
    difficultyBg->setID("difficulty-info-background");

    auto difficultyRow = CCNode::create();
    difficultyRow->setLayout(RowLayout::create()
        ->setGap(24.f)
        ->setAxisAlignment(AxisAlignment::Center)
    );

    #define DIFFICULTY(sprite, value) { \
        auto item = CCNode::create(); \
        auto icon = CCScale9Sprite::createWithSpriteFrameName(sprite); \
        item->setContentSize({30.f, 45.f}); \
        icon->setPosition({item->getContentSize().width / 2, item->getContentSize().height / 2}); \
        auto label = CCLabelBMFont::create(value, "bigFont.fnt"); \
        label->setScale(0.4f); \
        item->addChild(icon); \
        item->addChildAtPosition(label, Anchor::Bottom, ccp(0.f, 0.f)); \
        difficultyRow->addChild(item); \
    }

    DIFFICULTY("diffIcon_01_btn_001.png", gum.formatNumberWithCommas(user.total_normal_guesses).c_str())
    DIFFICULTY("diffIcon_05_btn_001.png", gum.formatNumberWithCommas(user.total_hardcore_guesses).c_str())

    #undef DIFFICULTY

    difficultyRow->setContentSize({240.f, 45.f});
    difficultyRow->updateLayout();
    difficultyRow->setAnchorPoint({0.5f, 0.5f});
    m_mainLayer->addChildAtPosition(difficultyRow, Anchor::Center, ccp(0.f, 35.f));

    auto historyItems = CCArray::create();
    GuessEntry item = {
        56143453,
        GameMode::Normal,
        350,
        {2020, 10, 10},
        {2021, 5, 15},
        "Skibidi",
        "Toilet"
    };
    GuessEntry item2 = {
        66318856,
        GameMode::Hardcore,
        500,
        {2020, 10, 10},
        {2021, 5, 15},
        "Dot Hog",
        "Toilet"
    };
    GuessEntry item3 = {
        10565740,
        GameMode::Normal,
        0,
        {2020, 10, 10},
        {2021, 5, 15},
        "Bloodbath",
        "Toilet"
    };
    auto cell = HistoryCell::create(item, 1, 240.f);
    auto cell2 = HistoryCell::create(item2, 1, 240.f);
    auto cell3 = HistoryCell::create(item3, 1, 240.f);
    historyItems->addObject(cell);
    historyItems->addObject(cell2);
    historyItems->addObject(cell3);

    auto historyBg = ListView::create(historyItems, 45.f, 240.f, 100.f);
    historyBg->setContentSize({240, 120});
    auto border = geode::Border::create(historyBg, {0,0,0,75}, {240.f, 100.f});
    //yoinked from creationRotation
    if(CCScale9Sprite* borderSprite = typeinfo_cast<CCScale9Sprite*>(border->getChildByID("geode.loader/border_sprite"))) {
        float scaleFactor = 1.7f;
        borderSprite->setContentSize(CCSize{borderSprite->getContentSize().width, borderSprite->getContentSize().height + 3} / scaleFactor);
        borderSprite->setScale(scaleFactor);
        borderSprite->setPositionY(-1);
    }
    border->ignoreAnchorPointForPosition(false);
    m_mainLayer->addChildAtPosition(border, Anchor::Center, ccp(0.f, -60.f));
    border->setID("history-list-background");

    return true;
}
