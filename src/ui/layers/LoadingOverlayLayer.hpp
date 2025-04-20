#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class LoadingOverlayLayer : public cocos2d::CCLayerColor {
protected:
    bool init() override;

    ~LoadingOverlayLayer() override {
        cocos2d::CCTouchDispatcher::get()->unregisterForcePrio(this);
    }

    void registerWithTouchDispatcher() override {
        cocos2d::CCTouchDispatcher::get()->addTargetedDelegate(this, -500, true);
    }

    void forceRemoveLayer(); // Last ditch incase it gets stuck for some reason

    static constexpr float TIMEOUT_SECONDS = 20.f;
public:
    static LoadingOverlayLayer* create();

    void addToScene();
};