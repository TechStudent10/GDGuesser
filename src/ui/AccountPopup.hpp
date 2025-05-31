#pragma once

#include <Geode/Geode.hpp>
#include <managers/GuessManager.hpp>
using namespace geode::prelude;

class AccountPopup : public geode::Popup<LeaderboardEntry> {
protected:
    bool setup(LeaderboardEntry user);
    void getGuesses();

    CCMenuItemSpriteExtra* backBtn;
    CCMenuItemSpriteExtra* nextBtn;

    int current_page = 0;
    LeaderboardEntry user;
    Border* guessList = nullptr;
public:
    static AccountPopup* create(LeaderboardEntry user);
};
