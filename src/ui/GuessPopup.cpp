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
    
    m_yearInput = NumberInput::create("Year", 2025, 2013, InputType::Year);
    m_yearInput->setAnchorPoint({ 0.5f, 0.5f });
    m_yearInput->m_input->setDelegate(this);
    
    m_monthInput = NumberInput::create("Month", 12, 1, InputType::Month);
    m_monthInput->setAnchorPoint({ 0.5f, 0.5f });
    m_monthInput->m_input->setDelegate(this);
    
    m_dayInput = NumberInput::create("Day", 31, 1, InputType::Day);
    m_dayInput->setAnchorPoint({ 0.5f, 0.5f });
    m_dayInput->m_input->setDelegate(this);
    
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
    submitMenu->setContentWidth(m_mainLayer->getContentWidth());

    submitMenu->addChild(submitBtn);

    m_mainLayer->addChildAtPosition(submitMenu, Anchor::Bottom, ccp(0.f, 20.f));

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