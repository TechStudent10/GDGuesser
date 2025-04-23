#include "LoadingOverlayLayer.hpp"

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

    m_spinner = LoadingSpinner::create(100.f);
    
    // This makes sure that the game can never softlock because of bad coding skills
    this->runAction(CCSequence::create(CCDelayTime::create(TIMEOUT_SECONDS), CCCallFunc::create(this, callfunc_selector(LoadingOverlayLayer::removeMe)), 0));
    
    addChildAtPosition(m_spinner, Anchor::Center, ccp(0, 0), false);
    
    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);
    this->setZOrder(999);
    
    this->setOpacity(0);
    this->runAction(CCFadeTo::create(.25f, 150));

    return true;
}

void LoadingOverlayLayer::addToScene() {
    SceneManager::get()->keepAcrossScenes(this);
}

void LoadingOverlayLayer::removeMe() {
    SceneManager::get()->forget(this);
    this->removeFromParent();
}