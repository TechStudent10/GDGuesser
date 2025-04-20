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

    m_playSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");

    m_playBtn = CCMenuItemExt::createSpriteExtra(m_playSprite, [this](CCObject*){
        if (!m_isBusy) {

            this->setKeyboardEnabled(false);
            this->m_isBusy = true;
            FMODAudioEngine::get()->playEffect("playSound_01.ogg", 1.f, 0.f, 0.3f);
            GameManager::get()->m_loadingLevel = true;

            auto sequence = CCSequence::create(CCDelayTime::create(0.f), CCCallFunc::create(this, callfunc_selector(LevelLayer::playStep2)), 0);
            this->runAction(sequence);

            auto outerCircle = CCSprite::createWithSpriteFrameName("d_circle_01_001.png");
            outerCircle->setColor({ 0, 46, 115 });
            outerCircle->setScale(1.88);
            outerCircle->setZOrder(-5);
            outerCircle->setPosition(m_playSprite->getContentSize() * 0.5f);
            m_playSprite->addChild(outerCircle);

            auto innerCircleLight = CCSprite::createWithSpriteFrameName("d_circle_01_001.png");
            innerCircleLight->setColor({ 0, 87, 218 });
            innerCircleLight->setScale(1.75);
            innerCircleLight->setZOrder(-4);
            innerCircleLight->setPosition(m_playSprite->getContentSize() * 0.5f);
            m_playSprite->addChild(innerCircleLight);

            auto innerCircleDark = CCSprite::createWithSpriteFrameName("d_circle_01_001.png");
            innerCircleDark->setColor({ 0, 46, 115 });
            innerCircleDark->setScale(1.63);
            innerCircleDark->setZOrder(-2);
            innerCircleDark->setPosition(m_playSprite->getContentSize() * 0.5f);
            m_playSprite->addChild(innerCircleDark);
            
            auto progressTimerSpr = CCSprite::createWithSpriteFrameName("d_circle_01_001.png");
            progressTimerSpr->setColor({ 0, 0xff, 0 });
            m_progressTimer = CCProgressTimer::create(progressTimerSpr);
            m_progressTimer->setType(CCProgressTimerType::kCCProgressTimerTypeRadial);
            m_progressTimer->setScale(1.75f);
            m_progressTimer->setZOrder(-3);
            m_progressTimer->setPosition(m_playSprite->getContentSize() * 0.5f);
            m_progressTimer->setPercentage(0.f);
            m_playSprite->addChild(m_progressTimer);

            m_playBtn->setEnabled(false);
            m_guessBtn->setEnabled(false);
        }
    });

    m_guessBtn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Guess!"), [this](CCObject*) {
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
    buttonMenu->addChild(m_playBtn);
    buttonMenu->addChild(m_guessBtn);

    m_playBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(size.width * 0.5f, size.height * 0.5f + 51.f)));
    m_guessBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(size.width * 0.5f, size.height * 0.5f - 26.f)));

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

void LevelLayer::playStep2() {
    auto& gm = GuessManager::get();

    std::string path = gm.realLevel->getAudioFileName();
    FMODAudioEngine::get()->loadMusic(path, 1.f, 0.f, 1.f, true, 0, 0, false);

    auto sequence = CCSequence::create(CCDelayTime::create(0.f), CCCallFunc::create(this, callfunc_selector(LevelLayer::playStep3)), 0);
    this->runAction(sequence);
}

void LevelLayer::playStep3() {
    auto& gm = GuessManager::get();

    GameManager::get()->m_sceneEnum = 3;
    this->m_playScene = PlayLayer::scene(gm.currentLevel, false, true);
    m_playScene->retain();

    auto sequence = CCSequence::create(CCDelayTime::create(0.f), CCCallFunc::create(this, callfunc_selector(LevelLayer::loadLevelStep)), 0);
    this->runAction(sequence);
}

void LevelLayer::loadLevelStep() {
    auto playLayer = m_playScene->getChildByType<PlayLayer*>(0);
    playLayer->processCreateObjectsFromSetup();
    m_progressTimer->setPercentage(playLayer->m_loadingProgress * 100.f);

    auto callfunc = callfunc_selector(LevelLayer::loadLevelStep);
    if (1.0 <= playLayer->m_loadingProgress)
        callfunc = callfunc_selector(LevelLayer::playStep4);

    auto sequence = CCSequence::create(CCDelayTime::create(0.f), CCCallFunc::create(this, callfunc), 0);
    this->runAction(sequence);       
}

void LevelLayer::playStep4() {
    GameManager::get()->m_loadingLevel = false;
    CCDirector::get()->replaceScene(CCTransitionFade::create(0.5f, this->m_playScene));
    this->m_playScene->release();
    this->m_playScene = nullptr;
}

void LevelLayer::keyBackClicked() {
    auto& gm = GuessManager::get();
    gm.endGame();
}
