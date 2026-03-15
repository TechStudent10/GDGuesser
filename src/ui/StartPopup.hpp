#pragma once

#include <Geode/Geode.hpp>
#include <managers/GuessManager.hpp>

using namespace geode::prelude;

class StartPopup : public geode::Popup {
protected:
    GameOptions options = {};
    bool init();
public:
    static StartPopup* create();
};
