#include "LevelLayer.hpp"

#include <managers/GuessManager.hpp>
#include <ui/GuessPopup.hpp>

// thanks Vinster!
static int getLevelDifficulty(GJGameLevel* level) {
    if (level->m_autoLevel) return -1;
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
        default: return 0;
    }
}

LevelLayer* LevelLayer::create() {
    auto ret = new LevelLayer();

    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    
    delete ret;
    return nullptr;
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

    // thanks once again Vinster!!
    auto difficultySprite = GJDifficultySprite::create(
        gm.options.mode == GameMode::Normal ? getLevelDifficulty(gm.realLevel) : 0,
        static_cast<GJDifficultyName>(1)
    );

    auto starsLabel = CCLabelBMFont::create(
        gm.options.mode == GameMode::Normal ? fmt::format("{}", (gm.realLevel->m_stars).value()).c_str() : "??",
        "bigFont.fnt"
    );

    auto starsIcon = CCSprite::createWithSpriteFrameName(
        "star_small01_001.png"
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

    m_playBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(size.width * 0.5f, size.height * 0.5f + 51.f)));
    m_guessBtn->setPosition(buttonMenu->convertToNodeSpace(ccp(size.width * 0.5f, size.height * 0.5f - 26.f)));

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
