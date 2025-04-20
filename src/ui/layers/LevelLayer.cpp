#include "LevelLayer.hpp"
#include <managers/GuessManager.hpp>
#include <ui/GuessPopup.hpp>

LevelLayer* LevelLayer::create() {
    auto ret = new LevelLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

static int getLevelDifficulty(GJGameLevel* level) {
    if (level->m_autoLevel) return 0;
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
        default: return -1;
    }
}

static std::string getLevelLength(GJGameLevel* level) {
    

    switch (level->m_levelLength) {
        case 0: return "Tiny";
        case 1: return "Short";
        case 2: return "Medium";
        case 3: return "Long";
        case 4: return "XL";
        case 5: return "Plat.";
        default: return 0;
    }
}

bool LevelLayer::init() {
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

    auto bottomLeftArt = CCSprite::createWithSpriteFrameName("GJ_sideArt_001.png");
    bottomLeftArt->setPosition({ -1.f, -1.f });
    bottomLeftArt->setAnchorPoint({ 0, 0 });

    auto bottomRightArt = CCSprite::createWithSpriteFrameName("GJ_sideArt_001.png");
    bottomRightArt->setPosition({ size.width + 1.f, -1.f });
    bottomRightArt->setAnchorPoint({ 0, 0 });
    bottomRightArt->setRotation(-90.f);

    auto& gm = GuessManager::get();

    auto playBtn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_playBtn2_001.png", 1.f, [this](CCObject*){
        auto& gm = GuessManager::get();
        auto scene = PlayLayer::scene(gm.currentLevel, false, false);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
    });

    auto guessBtn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Guess!"), [this](CCObject*) {
        auto popup = GuessPopup::create();
        popup->show();
    });

    auto nameLabel = CCLabelBMFont::create(
        gm.options.mode == GameMode::Normal ? gm.realLevel->m_levelName.c_str() : "?????????",
        "bigFont.fnt"
    );


    auto authorLabel = CCLabelBMFont::create(
        gm.options.mode == GameMode::Normal ? fmt::format("By {}", gm.realLevel->m_creatorName).c_str() : "By ??????",
        "goldFont.fnt"
    );

    auto difficultySprite = GJDifficultySprite::create(
        gm.options.mode == GameMode::Normal ? getLevelDifficulty(gm.realLevel) : 0,
        static_cast<GJDifficultyName>(0)
    );

    auto starsLabel = CCLabelBMFont::create(
        gm.options.mode == GameMode::Normal ? fmt::format("{}", (gm.realLevel->m_stars).value()).c_str() : "??",
        "bigFont.fnt"
    );

    auto starsIcon = CCSprite::createWithSpriteFrameName(
        "star_small01_001.png"
    );

    auto downloadsLabel = CCLabelBMFont::create(
        gm.options.mode == GameMode::Normal ? fmt::format("{}", gm.realLevel->m_downloads).c_str() : "????",
        "bigFont.fnt"
    );

    auto downloadsIcon = CCSprite::createWithSpriteFrameName(
        "GJ_downloadsIcon_001.png"
    );

    auto likesLabel = CCLabelBMFont::create(
        gm.options.mode == GameMode::Normal ? fmt::format("{}", gm.realLevel->m_likes).c_str() : "????",
        "bigFont.fnt"
    );

    auto likesIcon = CCSprite::createWithSpriteFrameName(
        "GJ_likesIcon_001.png"
    );

    auto durationLabel = CCLabelBMFont::create(
        gm.options.mode == GameMode::Normal ? fmt::format("{}", getLevelLength(gm.realLevel)).c_str() : "????",
        "bigFont.fnt"
    );

    auto durationIcon = CCSprite::createWithSpriteFrameName(
        "GJ_timeIcon_001.png"
    );

    nameLabel->setPosition({ size.width * 0.5f, director->getScreenTop() - 17.f });
    authorLabel->setPosition({ size.width * 0.5f, nameLabel->getPositionY() - 22.f });
    difficultySprite->setPosition({ size.width * 0.5f - 100.f, 226.f});
    starsLabel->setPosition({ difficultySprite->getPositionX(), difficultySprite->getPositionY() - 30.f});
    starsIcon->setPosition({ starsLabel->getPositionX() + 8.f, starsLabel->getPositionY() });
    downloadsIcon->setPosition({ size.width * 0.5f + 80.f, 249.f});
    likesIcon->setPosition({ downloadsIcon->getPositionX(), downloadsIcon->getPositionY() - 27.f });
    durationIcon->setPosition({ downloadsIcon->getPositionX(), downloadsIcon->getPositionY() - 56.f });
    downloadsLabel->setPosition({ downloadsIcon->getPositionX() + 33.f, downloadsIcon->getPositionY() });
    likesLabel->setPosition({ likesIcon->getPositionX() + 33.f, likesIcon->getPositionY() });
    durationLabel->setPosition({ durationIcon->getPositionX() + 33.f, durationIcon->getPositionY() });


    nameLabel->limitLabelWidth(300.f, 0.8f, 0.0f);
    authorLabel->limitLabelWidth(300.f, 0.8f, 0.0f);

    starsLabel->setScale(0.4f);
    starsLabel->setAnchorPoint({ 1.f, 0.5f });
    downloadsLabel->setScale(0.5f);
    downloadsLabel->setAnchorPoint({ 0.f, 0.5f });
    likesLabel->setScale(0.5f);
    likesLabel->setAnchorPoint({ 0.f, 0.5f });
    durationLabel->setScale(0.5f);
    durationLabel->setAnchorPoint({ 0.f, 0.5f });
    downloadsIcon->setAnchorPoint({ 0.f, 0.5f });
    likesIcon->setAnchorPoint({ 0.f, 0.5f });
    durationIcon->setAnchorPoint({ 0.f, 0.5f });

    if (gm.realLevel->m_accountID == 0) {
        authorLabel->setColor({ 90, 255, 255 });
    }

    this->addChild(nameLabel);
    this->addChild(authorLabel);
    this ->addChild(difficultySprite);
    this->addChild(starsLabel);
    this->addChild(starsIcon);
    this->addChild(downloadsLabel);
    this->addChild(downloadsIcon);
    this->addChild(likesLabel);
    this->addChild(likesIcon);
    this->addChild(durationLabel);
    this->addChild(durationIcon);

    auto songWidget = CustomSongWidget::create(LevelTools::getSongObject(gm.currentLevel->m_songID), nullptr, false, false, true, gm.currentLevel->m_songID < 30, false, false, 0);
    songWidget->getSongInfoIfUnloaded();
    songWidget->updateSongInfo();
    songWidget->onGetSongInfo(nullptr);
    songWidget->setPosition({ size.width / 2, 50.f });

    this->addChild(songWidget);

    auto buttonMenu = CCMenu::create();
    buttonMenu->addChild(playBtn);
    buttonMenu->addChild(guessBtn);

    playBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(size.width * 0.5f, size.height * 0.5f + 51.f)));
    guessBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(size.width * 0.5f, size.height * 0.5f - 26.f)));

    this->addChild(background);
    this->addChild(buttonMenu);
    this->addChild(bottomLeftArt);
    this->addChild(bottomRightArt);

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

    this->setKeypadEnabled(true);

    return true;
}

void LevelLayer::keyBackClicked() {
    auto& gm = GuessManager::get();
    gm.endGame();
}
