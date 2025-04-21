#include "LeaderboardLayer.hpp"
#include <managers/GuessManager.hpp>

#include <Geode/ui/LoadingSpinner.hpp>

class UserCell : public CCNode {
protected:
    bool init(LeaderboardEntry lbEntry, int index, float width) {
        auto nameLabel = CCLabelBMFont::create(lbEntry.username.c_str(), "goldFont.fnt");
        nameLabel->limitLabelWidth(150.f, 1.f, .0f);
        
        auto nameBtn = CCMenuItemExt::createSpriteExtra(nameLabel, [lbEntry](CCObject*) {
            bool myself = lbEntry.account_id == GJAccountManager::get()->m_accountID;
            ProfilePage::create(lbEntry.account_id, myself)->show();
        });
        
        nameBtn->setAnchorPoint(ccp(.0f, .5f));
        auto scoreLabel = CCLabelBMFont::create(fmt::format("{}", lbEntry.total_score).c_str(), "bigFont.fnt");
        scoreLabel->setScale(.6f);
        scoreLabel->setAlignment(cocos2d::kCCTextAlignmentRight);
        
        scoreLabel->setPosition({ width - 25.f, CELL_HEIGHT - 15.f});
        scoreLabel->setAnchorPoint({ 1.f, 0.5f });
        
        
        auto accuracyLabel = CCLabelBMFont::create(fmt::format("{:.1f}%", lbEntry.accuracy).c_str(), "bigFont.fnt");
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

    auto spinner = LoadingSpinner::create(100.f);
    spinner->setPosition(size / 2);
    this->addChild(spinner);

    auto& gm = GuessManager::get();
    float listWidth = 400.f;
    gm.getLeaderboard([this, listWidth, size, spinner](std::vector<LeaderboardEntry> lb) {
        auto listItems = CCArray::create();
        for (int i = 0; i < lb.size(); i++) {
            auto item = lb[i];
            auto cell = UserCell::create(item, i + 1, listWidth);
            listItems->addObject(cell);
        }

        // auto clv = CustomListView::create(listItems, BoomListType::User, 200.f, listWidth);
        auto listNode = ListView::create(listItems, UserCell::CELL_HEIGHT, listWidth);
        auto listLayer = GJListLayer::create(listNode, "", {191, 114, 62, 255}, listWidth, listNode->getContentHeight(), 1);
        listLayer->ignoreAnchorPointForPosition(false);
        listLayer->setAnchorPoint({ 0.5f, 0.5f });
        listLayer->setPosition({ size.width / 2, size.height / 2 - 20.f });

        spinner->removeFromParent();

        // auto listBorder = ListBorders::create();
        // listBorder->addChild(listNode);
        // listBorder->setSpriteFrames("GJ_table_bottom_001.png", "GJ_table_side_001.png");

        this->addChild(listLayer);
    });

    auto lbSpr = CCSprite::create("leaderboards.png"_spr);
    lbSpr->setPosition({ size.width / 2.f, size.height / 2 + 130.f });

    this->addChild(lbSpr);

    this->setKeypadEnabled(true);

    return true;
}

void GDGLeaderboardLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}
