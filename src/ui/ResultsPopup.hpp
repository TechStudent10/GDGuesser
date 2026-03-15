#pragma once

#include <Geode/Geode.hpp>
#include <managers/GuessManager.hpp>

using namespace geode::prelude;

class ResultsPopup : public geode::Popup {
protected:
    bool init(int score, LevelDate correctDate, LevelDate date);

    void onClose(cocos2d::CCObject *) override;
public:
    static ResultsPopup* create(int score, LevelDate correctDate, LevelDate date);
};
