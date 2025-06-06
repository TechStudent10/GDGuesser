#include "LeaderboardLayer.hpp"
#include <managers/GuessManager.hpp>
#include <ui/AccountPopup.hpp>

#include <Geode/ui/LoadingSpinner.hpp>

class UserCell : public CCNode {
protected:
    bool init(LeaderboardEntry lbEntry, int index, float width) {
        if (!CCNode::init())
            return false;
        
        auto nameLabel = CCLabelBMFont::create(lbEntry.username.c_str(), "goldFont.fnt");
        nameLabel->limitLabelWidth(150.f, 1.f, .0f);
        
        auto nameBtn = CCMenuItemExt::createSpriteExtra(nameLabel, [lbEntry](CCObject*) {
            AccountPopup::create(lbEntry)->show();
        });
        
        nameBtn->setAnchorPoint(ccp(.0f, .5f));
        auto scoreLabel = CCLabelBMFont::create(GuessManager::get().formatNumberWithCommas(lbEntry.total_score).c_str(), "bigFont.fnt");
        scoreLabel->setScale(.6f);
        scoreLabel->setAlignment(cocos2d::kCCTextAlignmentRight);
        
        scoreLabel->setPosition({ width - 25.f, CELL_HEIGHT - 15.f});
        scoreLabel->setAnchorPoint({ 1.f, 0.5f });
        
        auto accuracyLabel = CCLabelBMFont::create(fmt::format("{:.1f}%", (float)lbEntry.total_score / (float)lbEntry.max_score * 100.f).c_str(), "bigFont.fnt");
        accuracyLabel->setScale(.6f);
        accuracyLabel->setAlignment(cocos2d::kCCTextAlignmentRight);
        
        accuracyLabel->setPosition({ width - 25.f, 15.f});
        accuracyLabel->setAnchorPoint({ 1.f, 0.5f });
        
        auto player = SimplePlayer::create(0);
        auto gm = GameManager::get();
        
        player->updatePlayerFrame(lbEntry.icon_id, IconType::Cube);
        player->setColor(gm->colorForIdx(lbEntry.color1));
        player->setSecondColor(gm->colorForIdx(lbEntry.color2));
        
        player->setPosition({ 25.f, CELL_HEIGHT - 20.f });
        player->setAnchorPoint({ 0.f, 0.5f });
        player->setScale(.8f);
        
        if (lbEntry.color3 == -1) {
            player->disableGlowOutline();
        } else {
            player->setGlowOutline(gm->colorForIdx(lbEntry.color3));
        }
        
        auto positionLabel = CCLabelBMFont::create(std::to_string(index).c_str(), "goldFont.fnt");
        positionLabel->setScale(.7f);
        positionLabel->setPosition({ 25.f, 13.f });
        
        auto buttonMenu = CCMenu::create();
        buttonMenu->addChild(nameBtn);
        buttonMenu->setContentSize(ccp(width, CELL_HEIGHT));
        buttonMenu->setPosition(0, 0);

        nameBtn->setPosition(ccp(player->getPositionX() + 25.f, CELL_HEIGHT * .5f));
        
        this->addChild(player);
        this->addChild(positionLabel);
        this->addChild(buttonMenu);
        this->addChild(scoreLabel);
        this->addChild(accuracyLabel);

        this->setAnchorPoint({ 0.f, 0.5f });
        this->setContentSize({ width, CELL_HEIGHT });

        return true;
    }

public:
    static constexpr float CELL_HEIGHT = 55.f;

    static UserCell* create(LeaderboardEntry lbEntry, int index, float width) {
        auto ret = new UserCell;
        if (ret->init(lbEntry, index, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

class SearchPopup : public geode::Popup<> {
protected:
    bool setup() {
        this->setTitle("Player Search");
        
        auto searchInput = TextInput::create(150.f, "Name");
    
        auto text = CCLabelBMFont::create("Player Name", "bigFont.fnt");
        text->setScale(0.5f);

        auto btnSpr = ButtonSprite::create("Search");
        btnSpr->setScale(0.75f);
        auto btn = CCMenuItemExt::createSpriteExtra(
            btnSpr, [searchInput](CCObject*) {
                log::info("{}", searchInput->getString());
                auto& gm = GuessManager::get();
                gm.getAccount([](LeaderboardEntry entry) {
                    AccountPopup::create(entry)->show();
                }, 0, searchInput->getString());
            }
        );

        auto menu = CCMenu::create();
        menu->addChild(btn);

        m_mainLayer->addChildAtPosition(menu, Anchor::Bottom, ccp(0.f, 20.f));
        m_mainLayer->addChildAtPosition(searchInput, Anchor::Center, ccp(0.f, -5.f));
        m_mainLayer->addChildAtPosition(text, Anchor::Center, ccp(0.f, 20.f));

        return true;
    }

public:
    static SearchPopup* create() {
        auto ret = new SearchPopup;
        if (ret->initAnchored(210.f, 120.f)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

GDGLeaderboardLayer* GDGLeaderboardLayer::create() {
    auto ret = new GDGLeaderboardLayer();

    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    
    delete ret;
    return nullptr;
}

bool GDGLeaderboardLayer::init() {
    if (!CCLayer::init())
        return false;

    auto director = CCDirector::sharedDirector();
    auto size = director->getWinSize();

    auto background = createLayerBG();
    addSideArt(this, SideArt::Bottom);
    addSideArt(this, SideArt::TopRight);

    this->addChild(background, -5);

    auto closeBtnSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    auto closeBtn = CCMenuItemExt::createSpriteExtra(
        closeBtnSprite, [this](CCObject* target) {
            keyBackClicked();
        }
    );

    auto closeMenu = CCMenu::create();
    closeMenu->addChild(closeBtn);
    closeMenu->setPosition({ 24.f, director->getScreenTop() - 23.f });
    this->addChild(closeMenu);

    auto& gm = GuessManager::get();
    float listWidth = 400.f;
    gm.getLeaderboard([this, listWidth, size](std::vector<LeaderboardEntry> lb) {
        auto listItems = CCArray::create();
        for (int i = 0; i < lb.size(); i++) {
            auto item = lb[i];
            auto cell = UserCell::create(item, i + 1, listWidth);
            listItems->addObject(cell);
        }

        auto listNode = ListView::create(listItems, UserCell::CELL_HEIGHT, listWidth);
        auto listLayer = GJListLayer::create(listNode, "", {191, 114, 62, 255}, listWidth, listNode->getContentHeight(), 1);
        listLayer->ignoreAnchorPointForPosition(false);
        listLayer->setAnchorPoint({ 0.5f, 0.5f });
        listLayer->setPosition({ size.width / 2, size.height / 2 - 20.f });

        this->addChild(listLayer);
    });

    auto lbSpr = CCSprite::create("leaderboards.png"_spr);
    lbSpr->setPosition({ size.width / 2.f, size.height / 2 + 130.f });

    auto infoBtn = InfoAlertButton::create("Info", "The way the leaderboard is sorted is via <cy>accuracy</c>. Your accuracy is your total score divided by your overall <cl>maximum score</c> (the score you would have if you got max points on all questions).", 1.f);

    auto searchBtn = CCMenuItemExt::createSpriteExtraWithFrameName("gj_findBtn_001.png", 1.f, [](CCObject*) {
        SearchPopup::create()->show();
    });

    auto topRightMenu = CCMenu::create();
    topRightMenu->addChild(infoBtn);
    topRightMenu->addChild(searchBtn);
    topRightMenu->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
    );
    topRightMenu->setPosition({ size.width - 20.f, size.height - 40.f });

    this->addChild(topRightMenu);

    this->addChild(lbSpr);

    this->setKeypadEnabled(true);

    return true;
}

void GDGLeaderboardLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}
