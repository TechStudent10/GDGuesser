#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <managers/GuessManager.hpp>
#include <ui/StartPopup.hpp>

using namespace geode::prelude;

class $modify(MyCL, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;
        
        auto btnSpr = CCSprite::create("btn.png"_spr);
        auto btn = CCMenuItemExt::createSpriteExtra(CircleButtonSprite::create(btnSpr), [this](CCObject*) {
            if (!Mod::get()->getSavedValue<bool>("seen-lb-reset-notice", false)) {
                geode::createQuickPopup(
                    "Notice",
                    "Due to issues with <cr>cheating</c> and <cy>changes in leaderboard sorting</c>, the leaderboard <cl>has been reset.</c>\nFor information on the new leaderboard system, <cg>click the info button on the leaderboard</c>.",
                    "OK", nullptr,
                    [](auto, bool) {
                        StartPopup::create()->show();
                    }
                );
                Mod::get()->setSavedValue("seen-lb-reset-notice", true);
                return;
            }
            StartPopup::create()->show();
        });
        btnSpr->setScale(0.075f);
        btn->setID("start-btn"_spr);

        if (auto menu = this->getChildByIDRecursive("bottom-right-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        }
        
        return true;
    }
};

#ifdef DEBUG_BUILD
#include <Geode/modify/MenuLayer.hpp>
#include "ui/layers/LevelLayer.hpp"
class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;
    
        auto btn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_playBtn2_001.png", 0.6f, [this](auto sender) {
            auto& gm = GuessManager::get();
            auto level = GameLevelManager::get()->getMainLevel(1, false);
            gm.realLevel = level;

            gm.currentLevel = GJGameLevel::create();
            gm.currentLevel->copyLevelInfo(level);
            gm.currentLevel->m_levelName = "???????";
            gm.currentLevel->m_creatorName = "GDGuesser";

            auto scene = CCScene::create();
            scene->addChild(LevelLayer::create());
            CCDirector::get()->replaceScene(CCTransitionFade::create(0.5f, scene));
        });
    
        auto menu = CCMenu::create();

        btn->setPosition(0.f, 100.f);
        menu->addChild(btn);
        menu->setZOrder(100);

        this->addChild(menu);

        return true;
    }
};
#endif
