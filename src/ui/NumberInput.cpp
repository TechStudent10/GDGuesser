#include "NumberInput.hpp"

NumberInput* NumberInput::create(std::string title, int max, InputType type) {
    auto ret = new NumberInput;
    if (ret->init(title, max, type)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool NumberInput::init(std::string title, int max, InputType type) {
    m_max = max;

    std::string placeholder = "0";
    int maxChars = -1;

    switch (type) {
        case InputType::Year: placeholder = "0000"; maxChars = 4; break;
        case InputType::Month:
        case InputType::Day:
            placeholder = "00"; maxChars = 2; break;
        default: break;
    }

    m_input = TextInput::create(50.f, placeholder);
    m_input->getInputNode()->m_numberInput = true;
    m_input->setFilter("1234567890");
    if (maxChars != -1) m_input->setMaxCharCount(maxChars + 1);

    auto topSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    topSpr->setRotation(90);
    m_topButton = CCMenuItemExt::createSpriteExtra(topSpr, [this](CCObject*) {
        auto newVal = this->getValue() + 1;
        m_input->setString(std::to_string(newVal > m_max ? 0 : newVal));
    });
    m_topButton->ignoreAnchorPointForPosition(true);

    auto bottomSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    bottomSpr->setRotation(-90);
    bottomSpr->setFlipY(true);
    m_bottomButton = CCMenuItemExt::createSpriteExtra(bottomSpr, [this](CCObject*) {
        auto newVal = this->getValue() - 1;
        m_input->setString(std::to_string(newVal < 0 ? m_max : newVal));
    });
    m_bottomButton->ignoreAnchorPointForPosition(true);

    m_topMenu = CCMenu::create();
    m_bottomMenu = CCMenu::create();

    m_topMenu->addChild(m_topButton);
    m_bottomMenu->addChild(m_bottomButton);

    m_topMenu->setContentSize(m_topButton->getContentSize());
    m_bottomMenu->setContentSize(m_bottomButton->getContentSize());

    m_titleLabel = CCLabelBMFont::create(title.c_str(), "bigFont.fnt");
    m_titleLabel->setScale(0.5f);

    this->addChild(m_titleLabel);
    this->addChild(m_topMenu);
    this->addChild(m_input);
    this->addChild(m_bottomMenu);
    this->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
    );

    this->setContentHeight(100.f);
    this->setContentWidth(1.f);

    this->updateLayout();

    return true;
}

int NumberInput::getValue() {
    return geode::utils::numFromString<int>(m_input->getString()).unwrapOr(0);
}
