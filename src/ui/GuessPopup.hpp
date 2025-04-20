#pragma once

#include <Geode/Geode.hpp>

#include <ui/layers/LoadingOverlayLayer.hpp>

using namespace geode::prelude;

class GuessPopup : public geode::Popup<> {
protected:
    bool setup() override;
public:
    static GuessPopup* create();
};