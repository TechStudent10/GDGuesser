#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class LevelLayer : public CCLayer {
protected:
    bool init();
    void keyBackClicked();

    void playStep2();
    void playStep3();
    void playStep4();
    void loadLevelStep();

    bool m_isBusy = false;

    CCSprite* m_playSprite = nullptr;
    CCMenuItemSpriteExtra* m_playBtn = nullptr;
    CCMenuItemSpriteExtra* m_guessBtn = nullptr;

    CCProgressTimer* m_progressTimer = nullptr;

    CustomSongWidget* m_songWidget = nullptr;

    CCScene* m_playScene = nullptr;
public:
    static LevelLayer* create();
};
