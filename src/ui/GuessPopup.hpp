#pragma once

#include <Geode/Geode.hpp>

#include <ui/NumberInput.hpp>

using namespace geode::prelude;

class GuessPopup : public geode::Popup<>, TextInputDelegate {
protected:
    bool setup() override;

    virtual void textInputShouldOffset(CCTextInputNode* p0, float p1) override;
    virtual void textInputReturn(CCTextInputNode* p0) override;

    NumberInput* m_yearInput = nullptr;
    NumberInput* m_monthInput = nullptr;
    NumberInput* m_dayInput = nullptr;
public:
    static GuessPopup* create();
};