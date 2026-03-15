#pragma once

#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <managers/types.hpp>

class DuelsPopup : public geode::Popup {
public:
    static DuelsPopup* create();
protected:
    CCMenuItemSpriteExtra* readyBtn;
    
    CCNode* myNode;
    CCNode* oppNode;

    bool init();
    void updatePlayers(Duel duel);

    void onClose(CCObject*) override;
};
