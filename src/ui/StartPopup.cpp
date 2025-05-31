#include "StartPopup.hpp"
#include <ui/layers/LeaderboardLayer.hpp>
#include <ui/layers/LoadingOverlayLayer.hpp>
#include <ui/AccountPopup.hpp>

StartPopup* StartPopup::create() {
    auto ret = new StartPopup;
    if (ret->initAnchored(300.f, 250.f)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool StartPopup::setup() {
    this->setTitle("GDGuesser");

    auto playSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
    playSprite->setScale(0.75f);
    auto btn = CCMenuItemExt::createSpriteExtra(playSprite, [this](CCObject*) {
        auto startGameFr = [this]() {
            auto& gm = GuessManager::get();
            gm.startNewGame({
                .mode = options.mode,
                .versions = options.versions
            });
        };
        if (options.versions.size() != 13) {
            geode::createQuickPopup(
                "Warning",
                "Your scores will only be submitted if all versions are selected.\nYou can still play and get and get a score, though.",
                "Cancel", "Continue",
                [this, startGameFr](auto, bool btn2) {
                    if (!btn2) {
                        this->onClose(nullptr);
                        return;
                    }

                    startGameFr();
                }
            );
        } else {
            startGameFr();
        }
    });
    auto topRightMenu = CCMenu::create();
    auto discordBtn = CCMenuItemExt::createSpriteExtraWithFrameName("gj_discordIcon_001.png", 1.f, [](CCObject*) {
        CCApplication::get()->openURL(
            Mod::get()->getMetadataRef().getLinks().getCommunityURL()->c_str()
        );
    });
    auto infoBtn = InfoAlertButton::create("Info", "<cg>Normal mode</c> is the regular version of the game. You can see the level name, author, and difficulty. Scores submitted in this mode are out of 500.\n<cr>Hardcore mode</c> gives you nothing but the song name and level to go by. Scores submitted in this mode are out of 600.", .75f);
    topRightMenu->addChild(infoBtn);
    topRightMenu->addChild(discordBtn);
    topRightMenu->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
    );
    m_mainLayer->addChildAtPosition(topRightMenu, Anchor::TopRight, ccp(-30.f, -40.f));
    auto startMenu = CCMenu::create();
    startMenu->addChild(btn);
    m_mainLayer->addChildAtPosition(startMenu, Anchor::Center, ccp(0.f, 5.f));

    auto lbBtn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_levelLeaderboardBtn_001.png", .85f, [](CCObject*) {
        auto openLb = []() {
            auto layer = GDGLeaderboardLayer::create();
            auto scene = CCScene::create();
            scene->addChild(layer);
            CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
        };

        auto key = "seen-lb-reset-b4-stable-notice";
        if (!Mod::get()->getSavedValue<bool>(key, false)) {
            geode::createQuickPopup(
                "Reset Warning",
                "GDGuesser is currently in beta, and as a result is undergoing major changes in this time. Due to this, <cy>there will be a leaderboard reset right before GDGuesser exits beta</c>. After this reset, <cl>there will be no more resets to the leaderboard</c>, pending something requires it.",
                "OK", nullptr,
                [openLb](auto, bool) {
                    openLb();
                }
            );
            Mod::get()->setSavedValue(key, true);
            return;
        }
        openLb();
    });
    auto lbMenu = CCMenu::create();
    lbMenu->addChild(lbBtn);
    m_mainLayer->addChildAtPosition(lbMenu, Anchor::Center, ccp(60.f, 5.f));

    auto profileBtn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_profileButton_001.png", .75f, [](CCObject*) {
        auto& gm = GuessManager::get();
        gm.getAccount(GJAccountManager::get()->m_accountID, [](LeaderboardEntry user) {
            if (user.account_id != 0) {
                AccountPopup::create(user)->show();
            } else {
                CCArray* array = CCArray::create();
                array->addObject(DialogObject::create("Scratch", "<cy>Play a game</c> of GDGuesser to create a profile!", 10, 1, false, {255, 255, 255}));
                auto dl = DialogLayer::createDialogLayer(nullptr, array, 4);
                dl->setZOrder(106);
                dl->animateInRandomSide();
                CCScene::get()->addChild(dl);
            }
        });
        
    });
    auto profileMenu = CCMenu::create();
    profileMenu->addChild(profileBtn);
    m_mainLayer->addChildAtPosition(profileMenu, Anchor::Center, ccp(-60.f, 5.f));

    auto versionsMenu = CCMenu::create();
    auto difficultyMenu = CCMenu::create();

    std::vector<CCMenuItemToggler*> difficultyButtons;

    #define DIFFICULTY_BTN(selectedMode, sprite, labelText) \
        { \
            auto iconOn = CCScale9Sprite::createWithSpriteFrameName(sprite); \
            auto iconOff = CCScale9Sprite::createWithSpriteFrameName(sprite); \
            iconOff->setOpacity(255 * 0.5); \
            \
            auto label = CCLabelBMFont::create(labelText, "bigFont.fnt"); \
            label->setScale(0.3f); \
            \
            auto labelCopy = CCLabelBMFont::create(labelText, "bigFont.fnt"); \
            labelCopy->setScale(0.3f); \
            \
            auto containerOn = CCNode::create(); \
            containerOn->setContentSize({30.f, 45.f}); \
            iconOn->setPosition({containerOn->getContentSize().width / 2, containerOn->getContentSize().height / 2}); \
            containerOn->addChild(iconOn); \
            containerOn->addChildAtPosition(label, Anchor::Bottom, ccp(0.f, 0.f)); \
            \
            auto containerOff = CCNode::create(); \
            containerOff->setContentSize({30.f, 45.f}); \
            iconOff->setPosition({containerOff->getContentSize().width / 2, containerOff->getContentSize().height / 2}); \
            containerOff->addChild(iconOff); \
            containerOff->addChildAtPosition(labelCopy, Anchor::Bottom, ccp(0.f, 0.f)); \
            \
            auto difficultyBtn = CCMenuItemExt::createToggler(containerOn, containerOff, [this, difficultyMenu](CCMenuItemToggler* toggler) { \
                if (options.mode == selectedMode) { \
                    toggler->setClickable(false); \
                    return; \
                } \
                \
                for (auto child : CCArrayExt<CCMenuItemToggler*>(difficultyMenu->getChildren())) { \
                    if (child != toggler) { \
                        child->toggle(false); \
                        child->setClickable(true); \
                    } \
                } \
                options.mode = selectedMode; \
            }); \
            difficultyMenu->addChild(difficultyBtn); \
            difficultyButtons.push_back(difficultyBtn); \
        }

    DIFFICULTY_BTN(GameMode::Normal, "diffIcon_01_btn_001.png", "Normal")
    DIFFICULTY_BTN(GameMode::Hardcore, "diffIcon_05_btn_001.png", "Hardcore")
    //DIFFICULTY_BTN(GameMode::Extreme, "diffIcon_10_btn_001.png", "Extreme")

    #undef DIFFICULTY_BTN

    if (!difficultyButtons.empty()) {
        difficultyButtons[0]->toggle(true);
        difficultyButtons[0]->setClickable(false);
        options.mode = GameMode::Normal;
    }

    #define VERSION_BTN(version_name) \
        { \
            auto versionTextOff = CCLabelBMFont::create(version_name, "bigFont.fnt"); \
            versionTextOff->setOpacity(255 * .5); \
            auto versionBtn = CCMenuItemExt::createToggler(CCLabelBMFont::create(version_name, "bigFont.fnt"), versionTextOff, [this](CCMenuItemToggler* toggler) { \
                if (!toggler->isOn()) { \
                    if (std::find(options.versions.begin(), options.versions.end(), version_name) == options.versions.end()) { \
                        options.versions.push_back(version_name); \
                    } \
                } else { \
                    auto searchResult = std::find(options.versions.begin(), options.versions.end(), version_name); \
                    if (searchResult != options.versions.end()) { \
                        options.versions.erase(searchResult); \
                    } \
                } \
            }); \
            versionBtn->toggle(true); \
            versionsMenu->addChild(versionBtn); \
            options.versions.push_back(version_name); \
        }

    VERSION_BTN("1.0")
    VERSION_BTN("1.1")
    VERSION_BTN("1.2")
    VERSION_BTN("1.3")
    VERSION_BTN("1.4")
    VERSION_BTN("1.5")
    VERSION_BTN("1.6")
    VERSION_BTN("1.7")
    VERSION_BTN("1.8")
    VERSION_BTN("1.9")
    VERSION_BTN("2.0")
    VERSION_BTN("2.1")
    VERSION_BTN("2.2")

    #undef VERSION_BTN

    versionsMenu->setLayout(
        RowLayout::create()
            ->setGrowCrossAxis(true)
            ->setGap(8.f)
    );
    versionsMenu->setScale(0.5f);
    versionsMenu->setContentWidth(380.f);
    versionsMenu->updateLayout();

    difficultyMenu->setLayout(
        RowLayout::create()
            ->setGrowCrossAxis(true)
            ->setGap(24.f)
    );
    difficultyMenu->setContentWidth(380.f);
    difficultyMenu->updateLayout();

    auto versionsContainer = CCScale9Sprite::create("square02b_001.png");
    versionsContainer->setContentSize({
        275.f,
        versionsMenu->getContentHeight() - 10.f
    });
    versionsContainer->setColor(ccc3(0,0,0));
    versionsContainer->setOpacity(75);

    versionsMenu->setPosition(
        {
            275.f / 2.f,
            versionsContainer->getContentHeight() / 2.f
        }
    );
    versionsContainer->addChild(versionsMenu);

    m_mainLayer->addChildAtPosition(versionsContainer, Anchor::Bottom, ccp(0.f, 45.f));
    m_mainLayer->addChildAtPosition(difficultyMenu, Anchor::Top, ccp(0.f, -55.f));

    auto versionsText = CCLabelBMFont::create("Versions", "goldFont.fnt");
    versionsText->setScale(.7f);

    m_mainLayer->addChildAtPosition(versionsText, Anchor::Bottom, ccp(0.f, 45.f + versionsContainer->getContentHeight() - 18.f));

    return true;
}
