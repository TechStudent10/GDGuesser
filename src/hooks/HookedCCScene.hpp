#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/CCScene.hpp>

using namespace geode::prelude;

class $modify(HookedCCScene, cocos2d::CCScene) {
    //$override
    int getHighestChildZ();
};