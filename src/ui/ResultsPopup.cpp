#include "ResultsPopup.hpp"
#include <managers/GuessManager.hpp>

ResultsPopup* ResultsPopup::create(int score, LevelDate correctDate, LevelDate date) {
    auto ret = new ResultsPopup;
    if (ret->initAnchored(360.f, 145.f, score, correctDate, date)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ResultsPopup::setup(int score, LevelDate correctDate, LevelDate date) {
    this->setTitle("Results");

    auto& gm = GuessManager::get();

    m_closeBtn->removeFromParent();

    std::string submittedDate = "";
    std::string correctDateFormatted = "";

    switch (gm.getDateFormat()) {
        case DateFormat::American: {
            submittedDate = fmt::format("{:02d}/{:02d}/{:04d}", date.month, date.day, date.year);
            correctDateFormatted = fmt::format("{:02d}/{:02d}/{:04d}", correctDate.month, correctDate.day, correctDate.year);
            break;
        }
        
        case DateFormat::Backwards: {        
            submittedDate = fmt::format("{:04d}/{:02d}/{:02d}", date.year, date.month, date.day);
            correctDateFormatted = fmt::format("{:04d}/{:02d}/{:02d}", correctDate.year, correctDate.month, correctDate.day);
            break;
        }
        
        default: {
            submittedDate = fmt::format("{:02d}/{:02d}/{:04d}", date.day, date.month, date.year);
            correctDateFormatted = fmt::format("{:02d}/{:02d}/{:04d}", correctDate.day, correctDate.month, correctDate.year);
        }
    }

    auto textArea = TextArea::create(fmt::format("You got a score of <cl>{}</c>!\nThe correct answer was <cl>{}</c>.\nYou guessed <cl>{}</c>.\nYour total score is <cl>{}</c>.", score, correctDateFormatted, submittedDate, GuessManager::get().totalScore), "bigFont.fnt", 1.f, 800.f, { 0.5f, 1.f }, 36, false);
    textArea->setScale(0.43f);
    m_mainLayer->addChildAtPosition(textArea, Anchor::Center, ccp(0.f, 14.f));

    auto nextRoundSpr = ButtonSprite::create("Continue", "goldFont.fnt", "GJ_button_01.png", 0.9);
    nextRoundSpr->setScale(0.8f);
    auto nextRoundBtn = CCMenuItemExt::createSpriteExtra(nextRoundSpr, [](CCObject*) {
        auto& gm = GuessManager::get();
        gm.startNewGame(gm.options);
    });

    auto endGameSpr = ButtonSprite::create("End Game", "goldFont.fnt", "GJ_button_06.png", 0.9);
    endGameSpr->setScale(0.8f);
    auto endGameBtn = CCMenuItemExt::createSpriteExtra(endGameSpr, [](CCObject*) {
        auto& gm = GuessManager::get();
        gm.endGame();
    });
    
    auto nextRoundMenu = CCMenu::create();
    nextRoundMenu->setContentWidth(m_mainLayer->getContentWidth());
    nextRoundMenu->addChild(endGameBtn);
    nextRoundMenu->addChild(nextRoundBtn);

    nextRoundMenu->setLayout(
        RowLayout::create()
    );

    m_mainLayer->addChildAtPosition(nextRoundMenu, Anchor::Bottom, ccp(0.f, 25.f));

    return true;
}

void ResultsPopup::onClose(CCObject*) {}
