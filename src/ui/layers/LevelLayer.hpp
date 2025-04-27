#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class LevelLayer : public CCLayer {
protected:
    bool init();
    void keyBackClicked();

    void createLoadingSprite();

    void playStep2();
    void playStep3();
    void playStep4();
    void loadLevelStep();

    void createInfoAlert();

    bool m_isBusy = false;

    CCSprite* m_playSprite = nullptr;
    CCSprite* m_favouriteSprite = nullptr;

    CCMenuItemSpriteExtra* m_playBtn = nullptr;
    CCMenuItemSpriteExtra* m_guessBtn = nullptr;
    CCMenuItemSpriteExtra* m_settingsBtn = nullptr;
    CCMenuItemSpriteExtra* m_infoBtn = nullptr;
    CCMenuItemSpriteExtra* m_favouriteBtn = nullptr;

    CCProgressTimer* m_progressTimer = nullptr;

    CustomSongWidget* m_songWidget = nullptr;

    CCScene* m_playScene = nullptr;
public:
    static LevelLayer* create();
    static CCScene* scene();
};
