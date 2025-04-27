#include "LevelLayer.hpp"

#include <managers/GuessManager.hpp>
#include <ui/GuessPopup.hpp>

LevelLayer* LevelLayer::create() {
    auto ret = new LevelLayer();

    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    
    delete ret;
    return nullptr;
}

CCScene* LevelLayer::scene() {
    auto scene = CCScene::create();
    auto layer = LevelLayer::create();
    scene->addChild(layer);
    return scene;
}

bool LevelLayer::init() {
    if (!CCLayer::init())
        return false;

    auto director = CCDirector::sharedDirector();
    auto size = director->getWinSize();

    auto background = createLayerBG();
    addSideArt(this, SideArt::Bottom);
    addSideArt(this, SideArt::TopRight);

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

            createLoadingSprite();

            m_playBtn->setEnabled(false);
            m_guessBtn->setEnabled(false);
            m_settingsBtn->setEnabled(false);
            m_infoBtn->setEnabled(false);
            m_favouriteBtn->setEnabled(false);
        }
    });

    m_guessBtn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Guess!"), [this](CCObject*) {
        if (!m_isBusy)
            GuessPopup::create()->show();
    });

    m_settingsBtn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_optionsBtn_001.png", .6f, [gm, this](auto sender) {
        if (!m_isBusy)
            GameLevelOptionsLayer::create(gm.currentLevel)->show();
    });

    m_infoBtn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_infoIcon_001.png", 1.f, [gm, this](auto sender) {
        createInfoAlert();
    });

    m_favouriteSprite = CCSprite::createWithSpriteFrameName(gm.realLevel->m_levelFavorited ? "gj_heartOn_001.png" : "gj_heartOff_001.png");

    m_favouriteBtn = CCMenuItemExt::createSpriteExtra(m_favouriteSprite, [gm, this](auto sender) {
        if (!this->m_isBusy) {
            gm.realLevel->m_levelFavorited = !gm.realLevel->m_levelFavorited;
            m_favouriteSprite->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                gm.realLevel->m_levelFavorited ? "gj_heartOn_001.png" : "gj_heartOff_001.png"
            ));
        }
    });

    auto nameLabel = CCLabelBMFont::create(
        gm.options.mode == GameMode::Normal ? gm.realLevel->m_levelName.c_str() : "?????????",
        "bigFont.fnt"
    );

    auto authorLabel = CCLabelBMFont::create(
        gm.options.mode == GameMode::Normal ? fmt::format("By {}", gm.realLevel->m_creatorName).c_str() : "By ??????",
        "goldFont.fnt"
    );

    // thanks once again Vinster!!
    auto difficultySprite = GJDifficultySprite::create(
        gm.options.mode == GameMode::Normal ? gm.getLevelDifficulty(gm.realLevel) : 0,
        GJDifficultyName::Long
    );

    auto starsLabel = CCLabelBMFont::create(
        gm.options.mode == GameMode::Normal ? fmt::format("{}", (gm.realLevel->m_stars).value()).c_str() : "??",
        "bigFont.fnt"
    );

    auto starsIcon = CCSprite::createWithSpriteFrameName(
        gm.realLevel->m_levelLength == 5  ? "moon_small01_001.png" : "star_small01_001.png"
    );

    starsLabel->setPosition({ size.width * 0.5f - 100.f, director->getScreenTop() * 0.6f});
    difficultySprite->setPosition({ starsLabel->getPositionX(), starsLabel->getPositionY() + (gm.realLevel->m_demon > 0 && gm.options.mode == GameMode::Normal ? 40.f : 30.f)});
    starsIcon->setPosition({ starsLabel->getPositionX() + 8.f, starsLabel->getPositionY() });

    starsLabel->setScale(0.4f);
    starsLabel->setAnchorPoint({ 1.f, 0.5f });

    this->addChild(difficultySprite);
    this->addChild(starsLabel);
    this->addChild(starsIcon);

    nameLabel->setPosition({ size.width * 0.5f, director->getScreenTop() - 17.f });
    authorLabel->setPosition({ size.width * 0.5f, nameLabel->getPositionY() - 22.f });

    nameLabel->limitLabelWidth(300.f, 0.8f, 0.0f);
    authorLabel->limitLabelWidth(300.f, 0.8f, 0.0f);

    if (gm.realLevel->m_accountID == 0) {
        authorLabel->setColor({ 90, 255, 255 });
    }

    this->addChild(nameLabel);
    this->addChild(authorLabel);

    // This fixes the song being unknown? its weird but it works so who cares
    auto songObject = gm.realLevel->m_songID == 0 ? LevelTools::getSongObject(gm.realLevel->m_audioTrack) : SongInfoObject::create(gm.realLevel->m_songID);
    auto songWidget = CustomSongWidget::create(songObject, nullptr, false, false, true, gm.realLevel->m_songID == 0, false, false, 0);
    
    songWidget->updateWithMultiAssets(gm.realLevel->m_songIDs, gm.realLevel->m_sfxIDs, 0);

    songWidget->updateSongInfo();
    songWidget->setPosition({ size.width / 2, 50.f });

    this->addChild(songWidget);

    auto buttonMenu = CCMenu::create();
    buttonMenu->addChild(m_playBtn);
    buttonMenu->addChild(m_guessBtn);
    buttonMenu->addChild(m_settingsBtn);
    buttonMenu->addChild(m_infoBtn);
    buttonMenu->addChild(m_favouriteBtn);

    m_playBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(size.width * 0.5f, size.height * 0.5f + 51.f)));
    m_guessBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(size.width * 0.5f, size.height * 0.5f - 26.f)));
    m_settingsBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(30.f, 70.f)));
    m_infoBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(30.f, 30.f)));
    m_favouriteBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(68.f, 30.f)));

    this->addChild(background, -5);
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

void LevelLayer::createInfoAlert() {
    if (!m_isBusy) {
        auto& gm = GuessManager::get();

        std::string description = fmt::format(
            "<cy>{0}</c>\n<cg>Total Attempts</c>: {1}\n<cl>Total Jumps</c>: {2}\n<cp>Normal </c>: {3}%\n<co>Practice</c>: {4}%",
            gm.options.mode == GameMode::Normal ? gm.realLevel->m_levelName : "???????",
            gm.realLevel->m_attempts.value(),
            gm.realLevel->m_jumps.value(),
            gm.realLevel->m_normalPercent.value(),
            gm.realLevel->m_practicePercent
        );

        FLAlertLayer::create(nullptr, "Level Stats", description, "OK", nullptr, 300.f, false, 0.f, 1.f)->show();
    }
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
    gm.endGame(true);
}

void LevelLayer::createLoadingSprite() {
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
}
