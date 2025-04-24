#include "LoadingOverlayLayer.hpp"

#include <managers/GuessManager.hpp>

LoadingOverlayLayer* LoadingOverlayLayer::create() {
    auto ret = new LoadingOverlayLayer();

    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool LoadingOverlayLayer::init() {
    if (!CCLayerColor::initWithColor({ 0, 0, 0, 150 }))
        return false;

    cocos2d::CCTouchDispatcher::get()->registerForcePrio(this, 2);

    auto& gm = GuessManager::get();

    m_spinner = LoadingSpinner::create(100.f);
    this->addChild(m_spinner);
    
    // This makes sure that the game can never softlock because of bad coding skills
    this->runAction(CCSequence::create(CCDelayTime::create(TIMEOUT_SECONDS), CCCallFunc::create(this, callfunc_selector(LoadingOverlayLayer::removeMe)), 0));
    
    m_statusLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_statusLabel->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
    this->addChild(m_statusLabel);

    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);
    this->setZOrder(999);
    
    this->setOpacity(0);
    this->runAction(CCFadeTo::create(.25f, 150));

    updateStatus(gm.taskStatus);

    return true;
}

void LoadingOverlayLayer::updateStatus(TaskStatus status) {
    auto& gm = GuessManager::get();
    
    auto size = CCDirector::get()->getWinSize();
    m_status = status;

    if (status != TaskStatus::None) {

    auto statusString = gm.statusToString(status);
        
    m_statusLabel->setAnchorPoint(ccp(.5f, .0f));
    m_statusLabel->setString(statusString.c_str());
    m_statusLabel->setPosition(size.width * .5f, size.height * .5f + 22.f);
    m_statusLabel->limitLabelWidth(175.f, .8f, .0f);
    m_statusLabel->setVisible(true);

        m_spinner->setPosition(size.width * .5f, size.height * .5f - 27.f);
    } else {
        m_statusLabel->setVisible(false);
        m_spinner->setPosition(size * .5f);
    }
}

void LoadingOverlayLayer::addToScene() {
    SceneManager::get()->keepAcrossScenes(this);
}

void LoadingOverlayLayer::removeMe() {
    SceneManager::get()->forget(this);
    this->removeFromParentAndCleanup(true);
    GuessManager::get().loadingOverlay = nullptr;
}