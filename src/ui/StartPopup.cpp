#include "StartPopup.hpp"
#include <ui/layers/LeaderboardLayer.hpp>
#include <ui/layers/LoadingOverlayLayer.hpp>

class DifficultySelectionPopup : public geode::Popup<GameOptions> {
protected:
    bool setup(GameOptions options) {
        this->setTitle("Select Difficulty");

        auto startGame = [this, options](GameMode mode) {
            // this is so dumb
            auto startGameFr = [this, options, mode]() {
                auto& gm = GuessManager::get();
                gm.startNewGame({
                    .mode = mode,
                    .versions = options.versions
                });
                this->onClose(nullptr);
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
        };

        auto normalBtn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Normal"), [startGame](CCObject*) {
            startGame(GameMode::Normal);
        });

        auto hardBtn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Hardcore"), [startGame](CCObject*) {
            startGame(GameMode::Hardcore);
        });

        auto menu = CCMenu::create();
        menu->setLayout(
            ColumnLayout::create()
                ->setAxisReverse(true)
        );

        menu->addChild(normalBtn);
        menu->addChild(hardBtn);

        menu->updateLayout();

        m_mainLayer->addChildAtPosition(menu, Anchor::Center);

        auto infoBtn = InfoAlertButton::create("Info", "<cg>Normal mode</c> is the regular version of the game. You can see the level name, author, and difficulty. Scores submitted in this mode are out of 500.\n<cr>Hardcore mode</c> gives you nothing but the song name and level to go by. Scores submitted in this mode are out of 600.", .8f);
        auto topRightMenu = CCMenu::create();
        topRightMenu->addChild(infoBtn);

        m_mainLayer->addChildAtPosition(topRightMenu, Anchor::TopRight, ccp(-20.f, -20.f));

        return true;
    }
public:
    static DifficultySelectionPopup* create(GameOptions options) {
        auto ret = new DifficultySelectionPopup;
        if (ret->initAnchored(280.f, 230.f, options)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

StartPopup* StartPopup::create() {
    auto ret = new StartPopup;
    if (ret->initAnchored(300.f, 200.f)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool StartPopup::setup() {
    this->setTitle("GDGuesser");

    auto btn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Start!"), [this](CCObject*) {
        DifficultySelectionPopup::create(options)->show();
    });
    auto startMenu = CCMenu::create();
    startMenu->addChild(btn);
    m_mainLayer->addChildAtPosition(startMenu, Anchor::Center, ccp(0.f, 30.f));

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
    auto discordBtn = CCMenuItemExt::createSpriteExtraWithFrameName("gj_discordIcon_001.png", 1.f, [](CCObject*) {
        CCApplication::get()->openURL(
            Mod::get()->getMetadata().getLinks().getCommunityURL()->c_str()
        );
    });
    auto lbMenu = CCMenu::create();
    lbMenu->addChild(lbBtn);
    lbMenu->addChild(discordBtn);
    lbMenu->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
    );
    m_mainLayer->addChildAtPosition(lbMenu, Anchor::TopRight, ccp(-30.f, -45.f));

    auto versionsMenu = CCMenu::create();

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

    auto versionsText = CCLabelBMFont::create("Versions", "goldFont.fnt");
    versionsText->setScale(.7f);

    m_mainLayer->addChildAtPosition(versionsText, Anchor::Bottom, ccp(0.f, 45.f + versionsContainer->getContentHeight() - 18.f));

    return true;
}
