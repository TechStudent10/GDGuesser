#include "StartPopup.hpp"
#include <managers/GuessManager.hpp>
#include <ui/layers/LeaderboardLayer.hpp>

class DifficultySelectionPopup : public geode::Popup<> {
protected:
    bool setup() {
        this->setTitle("Select your difficulty!");

        auto startGame = [this](GameMode mode) {
            auto& gm = GuessManager::get();
            gm.startNewGame({ .mode = mode });
            this->onClose(nullptr);
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

        auto infoBtn = InfoAlertButton::create("Info", "<cg>Normal mode</c> is the regular version of the game. You can see the level name, author, and difficulty. Scores submitted in this mode are out of 500.\n<cr>Hardcore mode</c> gives you nothing but the song name and level to go by. Scores submitted in this mode are out of 600.", 1.f);
        auto topRightMenu = CCMenu::create();
        topRightMenu->addChild(infoBtn);

        m_mainLayer->addChildAtPosition(topRightMenu, Anchor::TopRight);

        return true;
    }
public:
    static DifficultySelectionPopup* create() {
        auto ret = new DifficultySelectionPopup;
        if (ret->initAnchored(280.f, 230.f)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

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

    auto btn = CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Start!"), [](CCObject*) {
        DifficultySelectionPopup::create()->show();
    });
    auto startMenu = CCMenu::create();
    startMenu->addChild(btn);
    m_mainLayer->addChildAtPosition(startMenu, Anchor::Center);

    // CCMenuItemToggler* easyToggler;
    // CCMenuItemToggler* hardToggler;

    // auto easyOnSprite = CCSprite::createWithSpriteFrameName("diffIcon_01_btn_001.png");
    // auto easyOffSprite = CCSprite::createWithSpriteFrameName("diffIcon_01_btn_001.png");
    // easyOffSprite->setOpacity(0.5 * 255);

    // easyToggler = CCMenuItemExt::createToggler(easyOnSprite, easyOffSprite, [this, hardToggler, easyToggler](CCObject*) {
    //     if (!easyToggler->isOn()) return;

    //     hardToggler->toggle(false);
    // });

    // easyToggler->toggle(true);

    // auto hardOnSprite = CCSprite::createWithSpriteFrameName("diffIcon_06_btn_001.png");
    // auto hardOffSprite = CCSprite::createWithSpriteFrameName("diffIcon_06_btn_001.png");
    // hardOffSprite->setOpacity(0.5 * 255);

    // hardToggler = CCMenuItemExt::createToggler(hardOnSprite, hardOffSprite, [this, hardToggler, easyToggler](CCObject*) {
    //     if (!hardToggler->isOn()) return;

    //     easyToggler->toggle(false);
    // });

    // easyToggler->toggle(false);

    // auto diffMenu = CCMenu::create();
    // diffMenu->setLayout(
    //     RowLayout::create()
    // );
    // diffMenu->addChild(easyToggler);
    // diffMenu->addChild(hardToggler);

    // diffMenu->updateLayout();

    // m_mainLayer->addChildAtPosition(diffMenu, Anchor::Center, ccp(0.f, -25.f));

    auto lbBtn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_levelLeaderboardBtn_001.png", 1.f, [](CCObject*) {
        auto layer = GDGLeaderboardLayer::create();
        auto scene = CCScene::create();
        scene->addChild(layer);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.5f, scene));
    });
    auto lbMenu = CCMenu::create();
    lbMenu->addChild(lbBtn);
    m_mainLayer->addChildAtPosition(lbMenu, Anchor::TopRight, ccp(-30.f, -30.f));

    return true;
}
