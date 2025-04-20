#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>

#include <managers/GuessManager.hpp>

#include "ui/StartPopup.hpp"
#include "ui/layers/LevelLayer.hpp"

using namespace geode::prelude;

class $modify(MyCL, CreatorLayer) {
    struct Fields {
    };
    
    bool init() {
        if (!CreatorLayer::init()) return false;
        
        auto btn = CCMenuItemExt::createSpriteExtra(CircleButtonSprite::create(CCLabelBMFont::create("GDG", "goldFont.fnt")), [this](CCObject*) {
            StartPopup::create()->show();
        });
        
        if (auto menu = this->getChildByIDRecursive("bottom-right-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        }
        
        return true;
    }
};

#ifdef DEBUG_ENABLED
#include <Geode/modify/MenuLayer.hpp>
class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;
    
        auto btn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_playBtn2_001.png", 1.f, [this](auto sender) {
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

        this->addChild(menu);

        return true;
    }
};
#endif