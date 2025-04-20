#pragma once

#include <Geode/Geode.hpp>

#include <ui/NumberInput.hpp>

using namespace geode::prelude;

class GuessPopup : public geode::Popup<> {
protected:
    bool setup() override;

    void pulseInputsRed();

    NumberInput* m_yearInput = nullptr;
    NumberInput* m_monthInput = nullptr;
    NumberInput* m_dayInput = nullptr;
public:
    static GuessPopup* create();
};