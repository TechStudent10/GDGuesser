#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <managers/GuessManager.hpp>
#include <ui/StartPopup.hpp>
#include <Geode/loader/Dispatch.hpp>

using namespace geode::prelude;
using LevelId = int64_t;
using LevelIdEvent = geode::DispatchFilter<GJGameLevel*, int64_t*>;

class $modify(MyCL, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;
        
        auto btnSpr = CCSprite::create("btn.png"_spr);
        btnSpr->setScale(0.1f);
        auto btn = CCMenuItemExt::createSpriteExtra(btnSpr, [this](CCObject*) {
            StartPopup::create()->show();
        });
        // btnSpr->setScale(0.075f);
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
#include <ui/layers/LevelLayer.hpp>

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
        btn->setID("debug-btn"_spr);
    
        auto menu = CCMenu::create();
        menu->setID("debug-menu"_spr);

        btn->setPosition(0.f, 100.f);
        menu->addChild(btn);
        menu->setZOrder(100);

        this->addChild(menu);

        return true;
    }
};
#endif

#ifdef GEODE_IS_WINDOWS
// stolen with permission from globed

// _Throw_Cpp_error reimpl

static constexpr const char* msgs[] = {
    // error messages
    "device or resource busy",
    "invalid argument",
    "no such process",
    "not enough memory",
    "operation not permitted",
    "resource deadlock would occur",
    "resource unavailable try again",
};

using errc = std::errc;

static constexpr errc codes[] = {
    // system_error codes
    errc::device_or_resource_busy,
    errc::invalid_argument,
    errc::no_such_process,
    errc::not_enough_memory,
    errc::operation_not_permitted,
    errc::resource_deadlock_would_occur,
    errc::resource_unavailable_try_again,
};

[[noreturn]] void __cdecl _throw_cpp_error_hook(int code) {
    throw std::system_error((int) codes[code], std::generic_category(), msgs[code]);
}

$on_mod(Loaded) {
    auto address = reinterpret_cast<void*>(&std::_Throw_Cpp_error);

    // movabs rax, <hook>
    std::vector<uint8_t> patchBytes = {
        0x48, 0xb8
    };

    for (auto byte : geode::toBytes(&_throw_cpp_error_hook)) {
        patchBytes.push_back(byte);
    }

    // jmp rax
    patchBytes.push_back(0xff);
    patchBytes.push_back(0xe0);

    if (auto res = Mod::get()->patch(address, patchBytes)) {} else {
        log::warn("_Throw_Cpp_error hook failed: {}", res.unwrapErr());
    }
};
#endif

$execute {
    new EventListener(+[](GJGameLevel* level, LevelId* levelId) {
        auto gm = GuessManager::get();
        if (gm.currentLevel) {
            *levelId = gm.realLevel->m_levelID;
        }
        return ListenerResult::Propagate;
    }, LevelIdEvent("dankmeme.globed2/setup-level-id"));
};