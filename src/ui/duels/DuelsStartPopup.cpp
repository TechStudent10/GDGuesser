#include "DuelsStartPopup.hpp"
#include "DuelsPopup.hpp"

#include <managers/GuessManager.hpp>

DuelsStartPopup* DuelsStartPopup::create() {
    auto ret = new DuelsStartPopup;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool DuelsStartPopup::init() {
    if (!Popup::init(265.f, 180.f)) return false;

    this->setTitle("Duels!");

    // two buttons, create and join

    auto textInput = TextInput::create(165.f, "Join Code");
    auto joinBtn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Join"), [textInput](CCObject*) {
        auto& gm = GuessManager::get();
        gm.joinDuel(textInput->getString(), []() {
            DuelsPopup::create()->show();
        });
    });

    auto joinParent = CCMenu::create();
    joinParent->addChild(textInput);
    joinParent->addChild(joinBtn);
    joinParent->setLayout(
        RowLayout::create()
    );

    auto createBtn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Create"), [](CCObject*) {
        auto& gm = GuessManager::get();
        gm.createDuel([]() {
            DuelsPopup::create()->show();
        });
    });
    auto createMenu = CCMenu::create();
    createMenu->addChild(createBtn);

    auto separator = CCSprite::createWithSpriteFrameName("floorLine_001.png");
    separator->setScaleX(0.60f);
    separator->setScaleY(1.f);
    separator->setOpacity(100);

    m_mainLayer->addChildAtPosition(joinParent, Anchor::Center, ccp(0.f, +20.f));
    m_mainLayer->addChildAtPosition(createMenu, Anchor::Center, ccp(0.f, -50.f));
    m_mainLayer->addChildAtPosition(separator, Anchor::Center, ccp(0.f, -15.f));

    return true;
}

