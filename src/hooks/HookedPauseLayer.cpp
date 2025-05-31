#include "HookedPauseLayer.hpp"
#include <managers/GuessManager.hpp>
#include <ui/layers/LevelLayer.hpp>

void HookedPauseLayer::tryQuit(CCObject* sender) {
    auto& gm = GuessManager::get();
    if (!gm.currentLevel) {
        PauseLayer::tryQuit(sender);
        return;
    }

    gm.syncScores();
    
    auto playLayer = PlayLayer::get();

    auto layer = LevelLayer::create();
    auto scene = CCScene::create();
    scene->addChild(layer);
    CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(.5f, scene));

    playLayer->resetAudio();
    FMODAudioEngine::get()->unloadAllEffects();

    FMODAudioEngine::get()->playEffect("quitSound_01.ogg", 1.f, 0.f, 0.7f);

    GameManager::get()->fadeInMenuMusic();
}

void HookedPauseLayer::customSetup() {
    PauseLayer::customSetup();

    auto& gm = GuessManager::get();
    if (!gm.currentLevel) return;

    if (auto titleLabel = typeinfo_cast<CCLabelBMFont*>(this->getChildByIDRecursive("level-name"))) {
        titleLabel->setString(gm.currentLevel->m_levelName.c_str());
    }
}
