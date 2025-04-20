#pragma once

#include <Geode/Geode.hpp>
#include "../managers/GuessManager.hpp"

using namespace geode::prelude;

class ResultsPopup : public geode::Popup<int, std::string, LevelDate> {
protected:
    bool setup(int score, std::string correctDate, LevelDate date) override;

    void onClose(cocos2d::CCObject *) override;
public:
    static ResultsPopup* create(int score, std::string correctDate, LevelDate date);
};
