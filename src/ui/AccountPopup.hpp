#pragma once

#include <Geode/Geode.hpp>
#include <managers/GuessManager.hpp>
using namespace geode::prelude;

class AccountPopup : public geode::Popup {
protected:
    bool init(LeaderboardEntry user);
    void getGuesses();

    CCMenuItemSpriteExtra* backBtn;
    CCMenuItemSpriteExtra* nextBtn;

    int current_page = 0;
    LeaderboardEntry user;
    Border* guessList = nullptr;

    void onClose(CCObject*) override;
public:
    static AccountPopup* create(LeaderboardEntry user);
};
