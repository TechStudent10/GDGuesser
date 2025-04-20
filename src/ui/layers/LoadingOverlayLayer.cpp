#include "LoadingOverlayLayer.hpp"

#include <Geode/ui/LoadingSpinner.hpp>

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

    this->setOpacity(0);
    this->runAction(CCFadeTo::create(0.14, 150));

    this->runAction(CCSequence::create(CCDelayTime::create(TIMEOUT_SECONDS), CCCallFunc::create(this, callfunc_selector(LoadingOverlayLayer::forceRemoveLayer)), 0));

    addChildAtPosition(LoadingSpinner::create(100.f), Anchor::Center, ccp(0, 0), false);

    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);
    this->setZOrder(999);

    return true;
}

void LoadingOverlayLayer::addToScene() {
    CCScene::get()->addChild(this);
}

void LoadingOverlayLayer::forceRemoveLayer() {
    this->removeFromParentAndCleanup(true);
}