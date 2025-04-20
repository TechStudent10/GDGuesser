#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class NumberInput : public CCNode {
protected:
    bool init(std::string title, int max);

    TextInput* m_input;

    CCMenu* m_topMenu;
    CCMenuItemSpriteExtra* m_topButton;

    CCMenu* m_bottomMenu;
    CCMenuItemSpriteExtra* m_bottomButton;

    int m_max;
public:
    static NumberInput* create(std::string title, int max);

    int getValue();
};
