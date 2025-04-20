#include "LeaderboardLayer.hpp"
#include <managers/GuessManager.hpp>

class UserCell : public CCNode {
protected:
    bool init(LeaderboardEntry lbEntry, float width) {
        auto nameLabel = CCLabelBMFont::create(lbEntry.username.c_str(), "goldFont.fnt");
        auto nameBtn = CCMenuItemExt::createSpriteExtra(nameLabel, [lbEntry](CCObject*) {
            bool myself = lbEntry.account_id == GJAccountManager::get()->m_accountID;
            ProfilePage::create(lbEntry.account_id, myself)->show();
        });
        nameBtn->ignoreAnchorPointForPosition(true);
        auto scoreLabel = CCLabelBMFont::create(std::to_string(lbEntry.total_score).c_str(), "bigFont.fnt");
        scoreLabel->setScale(0.8f);

        auto nameMenu = CCMenu::create();
        nameMenu->addChild(nameBtn);
        nameMenu->setContentHeight(nameBtn->getContentHeight());
        nameMenu->ignoreAnchorPointForPosition(false);

        nameMenu->setPosition({ 25.f, CELL_HEIGHT / 2.f });
        nameMenu->setAnchorPoint({ 0.f, 0.5f });

        scoreLabel->setPosition({ width - 25.f, CELL_HEIGHT / 2.f });
        scoreLabel->setAnchorPoint({ 1.f, 0.5f });

        this->addChild(nameMenu);
        this->addChild(scoreLabel);

        this->setContentSize({ width, CELL_HEIGHT });

        return true;
    }

public:
    static constexpr float CELL_HEIGHT = 40.f;

    static UserCell* create(LeaderboardEntry lbEntry, float width) {
        auto ret = new UserCell;
        if (ret->init(lbEntry, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

GDGLeaderboardLayer* GDGLeaderboardLayer::create() {
    auto ret = new GDGLeaderboardLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDGLeaderboardLayer::init() {
    auto director = CCDirector::sharedDirector();
    auto size = director->getWinSize();

    auto background = CCSprite::create("GJ_gradientBG.png");
    background->setScaleX(
        size.width / background->getContentWidth()
    );
    background->setScaleY(
        size.height / background->getContentHeight()
    );
    background->setAnchorPoint({ 0, 0 });
    background->setColor({ 0, 102, 255 });
    background->setZOrder(-10);

    this->addChild(background);

    auto closeBtnSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    auto closeBtn = CCMenuItemExt::createSpriteExtra(
        closeBtnSprite, [this](CCObject* target) {
            keyBackClicked();
        }
    );

    auto closeMenu = CCMenu::create();
    closeMenu->addChild(closeBtn);
    closeMenu->setPosition({ 30, size.height - 30 });
    this->addChild(closeMenu);

    auto& gm = GuessManager::get();
    float listWidth = 400.f;
    gm.getLeaderboard([this, listWidth, size](std::vector<LeaderboardEntry> lb) {
        auto listItems = CCArray::create();
        for (auto item : lb) {
            auto cell = UserCell::create(item, listWidth);
            cell->setAnchorPoint({ 0.f, 0.5f });
            cell->setContentSize({ listWidth, cell->CELL_HEIGHT });
            listItems->addObject(cell);
        }

        // auto clv = CustomListView::create(listItems, BoomListType::User, 200.f, listWidth);
        auto listNode = ListView::create(listItems, UserCell::CELL_HEIGHT, listWidth);
        auto listLayer = GJListLayer::create(listNode, "", {191, 114, 62, 255}, listWidth, listNode->getContentHeight(), 1);
        listLayer->ignoreAnchorPointForPosition(false);
        listLayer->setAnchorPoint({ 0.5f, 0.5f });
        listLayer->setPosition({ size.width / 2, size.height / 2 - 20.f });

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
