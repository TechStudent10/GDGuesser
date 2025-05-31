#pragma once

#include <Geode/Geode.hpp>
#include <managers/GuessManager.hpp>
using namespace geode::prelude;

class AccountPopup : public geode::Popup<LeaderboardEntry> {
protected:
    bool setup(LeaderboardEntry user);
public:
    static AccountPopup* create(LeaderboardEntry user);
};
