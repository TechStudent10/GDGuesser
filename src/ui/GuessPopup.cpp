#include "GuessPopup.hpp"
#include <managers/GuessManager.hpp>
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
    
    m_yearInput = NumberInput::create("Year", 2025, InputType::Year);
    m_yearInput->setAnchorPoint({ 0.5f, 0.5f });

    m_monthInput = NumberInput::create("Month", 12, InputType::Month);
    m_monthInput->setAnchorPoint({ 0.5f, 0.5f });

    m_dayInput = NumberInput::create("Day", 31, InputType::Day);
    m_dayInput->setAnchorPoint({ 0.5f, 0.5f });

    m_mainLayer->addChildAtPosition(m_yearInput, Anchor::Center, ccp(-80.f, 0.f));
    m_mainLayer->addChildAtPosition(m_monthInput, Anchor::Center);
    m_mainLayer->addChildAtPosition(m_dayInput, Anchor::Center, ccp(80.f, 0.f));

    auto submitBtn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Guess!"), [this](CCObject*) {
        auto& gm = GuessManager::get();

        bool invalidDate = false;

        int year = m_yearInput->getValue();
        int month = m_monthInput->getValue();
        int day = m_dayInput->getValue();

        if (year > 2025 || year < 2013) {
            m_yearInput->m_titleLabel->setColor({ 255, 0, 0 });
            m_yearInput->m_titleLabel->runAction(CCTintTo::create(0.5f, 255, 255, 255));
            invalidDate = true;
        }
        
        if (month > 12 || month < 1) {
            m_monthInput->m_titleLabel->setColor({ 255, 0, 0 });
            m_monthInput->m_titleLabel->runAction(CCTintTo::create(0.5f, 255, 255, 255));
            invalidDate = true;
        }
        
        
        if (day > 31 || day < 1) {
            m_dayInput->m_titleLabel->setColor({ 255, 0, 0 });
            m_dayInput->m_titleLabel->runAction(CCTintTo::create(0.5f, 255, 255, 255));
            invalidDate = true;
        }

        if (!invalidDate) {
            gm.submitGuess({
                year,
                month,
                day
            }, [this](int score, std::string correctDate, LevelDate date) {
                this->onClose(nullptr);
                ResultsPopup::create(score, correctDate, date)->show();
            });
        }

    });

    auto submitMenu = CCMenu::create();
    submitMenu->addChild(submitBtn);

    m_mainLayer->addChildAtPosition(submitMenu, Anchor::Bottom, ccp(0.f, 20.f));

    return true;
}