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

    nameLabel->setPosition({ size.width * 0.5f, director->getScreenTop() - 17.f });
    authorLabel->setPosition({ size.width * 0.5f, nameLabel->getPositionY() - 22.f });

    nameLabel->limitLabelWidth(300.f, 0.8f, 0.0f);
    authorLabel->limitLabelWidth(300.f, 0.8f, 0.0f);

    if (gm.realLevel->m_accountID == 0) {
        authorLabel->setColor({ 90, 255, 255 });
    }

    this->addChild(nameLabel);
    this->addChild(authorLabel);

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
