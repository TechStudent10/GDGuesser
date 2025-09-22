#pragma once

#include <Geode/Geode.hpp>
#include <managers/net/manager.hpp>
#include <managers/net/client.hpp>
#include <managers/net/server.hpp>

using namespace geode::prelude;

// peak laziness
class PlayerScoreNode;

class DuelsPersistentNode : public CCNode {
public:
    static DuelsPersistentNode* create() {
        auto ret = new DuelsPersistentNode;
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    void persist();
    void forget();

    void slideOn();
    void slideOff();
protected:
    bool init() override;

    void updateDuel(Duel duel);

    PlayerScoreNode* myNode = nullptr;
    PlayerScoreNode* oppNode = nullptr;

    static constexpr float MOVE_BY = 130.f;
};
