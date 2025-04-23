#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/LoadingSpinner.hpp>

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

    static constexpr float TIMEOUT_SECONDS = 30.f; // How long it takes for the layer to remove itself
    LoadingSpinner* m_spinner;
public:
    static LoadingOverlayLayer* create();

    void addToScene();
    void removeMe();
};