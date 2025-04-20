#include "HookedEndLevelLayer.hpp"
#include <managers/GuessManager.hpp>
#include <ui/layers/LevelLayer.hpp>

void HookedEndLevelLayer::onMenu(CCObject* sender) {
    if (!GuessManager::get().currentLevel) {
        EndLevelLayer::onMenu(sender);
        return;
    }
    
    auto layer = LevelLayer::create();
    auto scene = CCScene::create();
    scene->addChild(layer);
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
}

void HookedEndLevelLayer::onLevelLeaderboard(CCObject* sender) {
    if (!GuessManager::get().currentLevel) {
        EndLevelLayer::onLevelLeaderboard(sender);
    }

    return;
}
