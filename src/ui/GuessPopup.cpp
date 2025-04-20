#include "GuessPopup.hpp"
#include <managers/GuessManager.hpp>
#include <ui/NumberInput.hpp>
#include <ui/ResultsPopup.hpp>

GuessPopup* GuessPopup::create() {
    auto ret = new GuessPopup;
    if (ret->initAnchored(310.f, 190.f)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GuessPopup::setup() {
    this->setTitle("Input your guess!");
    
    auto yearInput = NumberInput::create("Year", 2025);
    yearInput->setAnchorPoint({ 0.5f, 0.5f });

    auto monthInput = NumberInput::create("Month", 12);
    monthInput->setAnchorPoint({ 0.5f, 0.5f });

    auto dayInput = NumberInput::create("Day", 31);
    dayInput->setAnchorPoint({ 0.5f, 0.5f });

    m_mainLayer->addChildAtPosition(yearInput, Anchor::Center, ccp(-80.f, 0.f));
    m_mainLayer->addChildAtPosition(monthInput, Anchor::Center);
    m_mainLayer->addChildAtPosition(dayInput, Anchor::Center, ccp(80.f, 0.f));

    auto submitBtn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Guess!"), [this, yearInput, monthInput, dayInput](CCObject*) {
        auto& gm = GuessManager::get();
        gm.submitGuess({
            yearInput->getValue(),
            monthInput->getValue(),
            dayInput->getValue()
        }, [this](int score, std::string correctDate, LevelDate date) {
            this->onClose(nullptr);
            ResultsPopup::create(score, correctDate, date)->show();
        });
    });

    auto submitMenu = CCMenu::create();
    submitMenu->addChild(submitBtn);

    m_mainLayer->addChildAtPosition(submitMenu, Anchor::Bottom, ccp(0.f, 20.f));

    return true;
}
