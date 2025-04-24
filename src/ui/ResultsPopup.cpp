#include "ResultsPopup.hpp"
#include <managers/GuessManager.hpp>

ResultsPopup* ResultsPopup::create(int score, LevelDate correctDate, LevelDate date) {
    auto ret = new ResultsPopup;
    if (ret->initAnchored(360.f, 235.f, score, correctDate, date)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ResultsPopup::setup(int score, LevelDate correctDate, LevelDate date) {
    this->setTitle("Results");

    auto& gm = GuessManager::get();
    auto director = CCDirector::sharedDirector();
    auto size = director->getWinSize();

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

    auto resultBg = cocos2d::extension::CCScale9Sprite::create("square02b_001.png", { 0.0f, 0.0f, 80.0f, 80.0f });
    resultBg->setContentSize({340,148});
    resultBg->setColor({123,60,31});
    m_mainLayer->addChildAtPosition(resultBg, Anchor::Center, ccp(0.f, 0.f));
    resultBg->setID("result-info-background");

    auto separator = CCSprite::createWithSpriteFrameName("floorLine_001.png");
    separator->setScaleX(0.3f);
    separator->setScaleY(1);
    separator->setOpacity(100);
    separator->setRotation(90);
    separator->setID("result-separator");
    m_mainLayer->addChildAtPosition(separator, Anchor::Center, ccp(0.f, 0.f));

    auto nameLabel = CCLabelBMFont::create(
        gm.realLevel->m_levelName.c_str(),
        "bigFont.fnt"
    );
    nameLabel->limitLabelWidth(160.f, 0.6f, 0.0f);
    m_mainLayer->addChildAtPosition(nameLabel, Anchor::Center, ccp(-85.f, 60.f));
    nameLabel->setID("result-name-label");

    auto authorLabel = CCLabelBMFont::create(
        fmt::format("By {}", gm.realLevel->m_creatorName).c_str(),
        "goldFont.fnt"
    );
    authorLabel->limitLabelWidth(160.f, 0.5f, 0.0f);
    if (gm.realLevel->m_accountID == 0) {
        authorLabel->setColor({ 90, 255, 255 });
    }
    m_mainLayer->addChildAtPosition(authorLabel, Anchor::Center, ccp(-85.f, 40.f));
    authorLabel->setID("result-author-label");

    auto difficultySprite = GJDifficultySprite::create(
        gm.getLevelDifficulty(gm.realLevel),
        static_cast<GJDifficultyName>(1)
    );
    difficultySprite->updateFeatureState(gm.getFeaturedState(gm.realLevel));
    m_mainLayer->addChildAtPosition(difficultySprite, Anchor::Center, ccp(-85.f, 0.f));
    difficultySprite->setID("result-difficulty-sprite");

    auto starsLabel = CCLabelBMFont::create(
        fmt::format("{}", (gm.realLevel->m_stars).value()).c_str(),
        "bigFont.fnt"
    );
    auto starsIcon = CCSprite::createWithSpriteFrameName(
        gm.realLevel->m_levelLength == 5  ? "moon_small01_001.png" : "star_small01_001.png"
    );
    starsLabel->setScale(0.4f);
    starsLabel->setAnchorPoint({ 1.f, 0.5f });
    m_mainLayer->addChildAtPosition(starsLabel, Anchor::Center, ccp(-85.f, gm.realLevel->m_demon > 0 ? -40.f : -30.f));
    m_mainLayer->addChildAtPosition(starsIcon, Anchor::Center, ccp(-78.f, gm.realLevel->m_demon > 0 ? -40.f : -30.f));
    starsLabel->setID("result-stars-label");
    starsIcon->setID("result-stars-icon");

    auto yourScorelabel = CCLabelBMFont::create(
        "Your Score:",
        "bigFont.fnt"
    );
    yourScorelabel->setScale(0.5f);
    m_mainLayer->addChildAtPosition(yourScorelabel, Anchor::Center, ccp(85.f, 60.f));
    yourScorelabel->setID("result-your-score-label");

    auto scoreLabel = CCLabelBMFont::create(
        fmt::format("{}", score).c_str(),
        "bigFont.fnt"
    );
    float t = std::clamp(score / (gm.options.mode == GameMode::Normal ? 500.f : 600.f), 0.f, 1.f);
    GLubyte red   = static_cast<GLubyte>((1.f - t) * 255);
    GLubyte green = static_cast<GLubyte>(t * 255);
    scoreLabel->setScale(0.6f);
    scoreLabel->setColor({ red, green, 0 });
    m_mainLayer->addChildAtPosition(scoreLabel, Anchor::Center, ccp(85.f, 40.f));
    scoreLabel->setID("result-score-label");

    auto correctLabel = CCLabelBMFont::create(
        "Correct:",
        "bigFont.fnt"
    );
    correctLabel->setScale(0.4f);
    m_mainLayer->addChildAtPosition(correctLabel, Anchor::Center, ccp(45.f, 10.f));
    correctLabel->setID("result-correct-label");

    auto guessedLabel = CCLabelBMFont::create(
        "Guessed:",
        "bigFont.fnt"
    );
    guessedLabel->setScale(0.4f);
    m_mainLayer->addChildAtPosition(guessedLabel, Anchor::Center, ccp(125.f, 10.f));
    guessedLabel->setID("result-guessed-label");

    auto correctDateLabel = CCLabelBMFont::create(
        correctDateFormatted.c_str(),
        "bigFont.fnt"
    );
    correctDateLabel->setScale(0.35f);
    correctDateLabel->setColor({ 96, 171, 239 });
    m_mainLayer->addChildAtPosition(correctDateLabel, Anchor::Center, ccp(45.f, -5.f));
    correctDateLabel->setID("result-correct-date-label");

    auto guessedDateLabel = CCLabelBMFont::create(
        submittedDate.c_str(),
        "bigFont.fnt"
    );
    guessedDateLabel->setScale(0.35f);
    guessedDateLabel->setColor({ 96, 171, 239 });
    m_mainLayer->addChildAtPosition(guessedDateLabel, Anchor::Center, ccp(125.f, -5.f));
    guessedDateLabel->setID("result-guessed-date-label");

    auto totalScoreLabel = CCLabelBMFont::create(
        "Total Score:",
        "bigFont.fnt"
    );
    m_mainLayer->addChildAtPosition(totalScoreLabel, Anchor::Center, ccp(85.f, -30.f));
    totalScoreLabel->limitLabelWidth(160.f, 0.5f, 0.0f);
    totalScoreLabel->setID("result-total-score-label");

    auto totalLabel = CCLabelBMFont::create(
        fmt::format("{}", gm.totalScore).c_str(),
        "goldFont.fnt"
    );
    totalLabel->setScale(0.7f);
    m_mainLayer->addChildAtPosition(totalLabel, Anchor::Center, ccp(85.f, -50.f));
    totalLabel->setID("result-total-label");

    auto levelID = gm.realLevel->m_levelID.value();
    auto IDLabel = CCLabelBMFont::create(
        fmt::format("ID: {}", gm.realLevel->m_levelID.value()).c_str(),
        "goldFont.fnt"
    );
    auto IDBtn = CCMenuItemExt::createSpriteExtra(IDLabel, [levelID](CCMenuItemSpriteExtra* sender) {
        auto str = fmt::format("{}", levelID);
        clipboard::write(str);
        Notification::create("Copied to clipboard", NotificationIcon::None)->show();
    });
    auto IDMenu = CCMenu::create();
    IDMenu->addChild(IDBtn);
    IDMenu->setScale(0.5f);
    IDMenu->setAnchorPoint({ 0.f, 0.f });
    m_mainLayer->addChildAtPosition(IDMenu, Anchor::Center, ccp(-85.f, -60.f));
    IDMenu->setID("result-id-menu");

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
        gm.endGame(false);
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
