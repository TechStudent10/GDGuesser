#include "HookedPauseLayer.hpp"
#include <managers/GuessManager.hpp>
#include <ui/layers/LevelLayer.hpp>

void HookedPauseLayer::tryQuit(CCObject* sender) {
    auto& gm = GuessManager::get();
    if (!gm.currentLevel) {
        PauseLayer::tryQuit(sender);
        return;
    }

    // thanks cvolton for the help!
    int sessionAttempts = PlayLayer::get()->m_attempts;
    int oldAttempts = gm.realLevel->m_attempts.value();
    gm.realLevel->handleStatsConflict(gm.currentLevel);
    gm.realLevel->m_attempts = sessionAttempts + oldAttempts;
    gm.realLevel->m_orbCompletion = gm.currentLevel->m_orbCompletion;
    GameManager::get()->reportPercentageForLevel(gm.realLevel->m_levelID, gm.realLevel->m_normalPercent, gm.realLevel->isPlatformer());

    auto layer = LevelLayer::create();
    auto scene = CCScene::create();
    scene->addChild(layer);
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
}

void HookedPauseLayer::customSetup() {
    PauseLayer::customSetup();

    auto& gm = GuessManager::get();
    if (!gm.currentLevel) return;

    if (auto titleLabel = typeinfo_cast<CCLabelBMFont*>(this->getChildByIDRecursive("level-name"))) {
        titleLabel->setString(gm.currentLevel->m_levelName.c_str());
    }
}
