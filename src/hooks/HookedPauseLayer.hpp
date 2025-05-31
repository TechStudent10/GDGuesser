#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

class $modify(HookedPauseLayer, PauseLayer) {
    $override
    void customSetup();

    $override
    void tryQuit(CCObject* sender);
};
