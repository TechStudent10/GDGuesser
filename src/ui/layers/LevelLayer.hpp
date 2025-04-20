#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class LevelLayer : public CCLayer {
protected:
    bool init();
    void keyBackClicked();
public:
    static LevelLayer* create();
};
