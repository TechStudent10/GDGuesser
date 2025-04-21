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

    auto& gm = GuessManager::get();
    
    m_yearInput = NumberInput::create("Year", 2025, 2013, InputType::Year);
    m_yearInput->setAnchorPoint({ 0.5f, 0.5f });
    m_yearInput->m_input->setDelegate(this);
    
    m_monthInput = NumberInput::create("Month", 12, 1, InputType::Month);
    m_monthInput->setAnchorPoint({ 0.5f, 0.5f });
    m_monthInput->m_input->setDelegate(this);
    
    m_dayInput = NumberInput::create("Day", 31, 1, InputType::Day);
    m_dayInput->setAnchorPoint({ 0.5f, 0.5f });
    m_dayInput->m_input->setDelegate(this);

    auto dayPosition = ccp(-80.f, .0f);
    auto monthPosition = ccp(.0f, .0f);
    auto yearPosition = ccp(80.f, .0f);

    switch (gm.getDateFormat()) {
        case DateFormat::American: {
            monthPosition = ccp(-80.f, .0f);
            dayPosition = ccp(.0f, .0f);
            break;
        }

        case DateFormat::Backwards: {
            yearPosition = ccp(-80.f, .0f);
            dayPosition = ccp(80.f, .0f);
        }

        default: break;
    }
    
    m_mainLayer->addChildAtPosition(m_yearInput, Anchor::Center, yearPosition);
    m_mainLayer->addChildAtPosition(m_monthInput, Anchor::Center, monthPosition);
    m_mainLayer->addChildAtPosition(m_dayInput, Anchor::Center, dayPosition);

    auto submitSpr = ButtonSprite::create("Guess!", .9f);
    submitSpr->setScale(.8f);

    auto submitBtn = CCMenuItemExt::createSpriteExtra(submitSpr, [this](CCObject*) {
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
            }, [this](int score, LevelDate correctDate, LevelDate date) {
                this->onClose(nullptr);
                ResultsPopup::create(score, correctDate, date)->show();
            });
        }

    });

    auto submitMenu = CCMenu::create();
    submitMenu->setContentWidth(m_mainLayer->getContentWidth());

    submitMenu->addChild(submitBtn);

    m_mainLayer->addChildAtPosition(submitMenu, Anchor::Bottom, ccp(0.f, 23.f));

    return true;
}

void GuessPopup::textInputShouldOffset(CCTextInputNode* p0, float p1) { // This offsets the guess popup when you focus an input on iOS
    auto size = CCDirector::get()->getWinSize();
    m_mainLayer->stopAllActions();
    m_mainLayer->runAction(CCMoveTo::create(.2f, ccp(size.width * .5f, size.height * .5f + (p1 * .5f))));
}

void GuessPopup::textInputReturn(CCTextInputNode* p0) { // Moves it back
    auto size = CCDirector::get()->getWinSize();
    m_mainLayer->stopAllActions();
    m_mainLayer->runAction(CCMoveTo::create(.2f, size * .5f));
}