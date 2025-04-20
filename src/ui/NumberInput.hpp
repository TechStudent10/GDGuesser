#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

enum class InputType {
    Year = 0,
    Month = 1,
    Day = 2,
    Other = 3
};

class NumberInput : public CCNode {
protected:
    bool init(std::string title, int max, InputType type);

    CCMenu* m_topMenu;
    CCMenuItemSpriteExtra* m_topButton;
    
    CCMenu* m_bottomMenu;
    CCMenuItemSpriteExtra* m_bottomButton;
    int m_max;
    public:
    static NumberInput* create(std::string title, int max, InputType type);
    
    int getValue();
    
    TextInput* m_input;
    CCLabelBMFont* m_titleLabel;
};
