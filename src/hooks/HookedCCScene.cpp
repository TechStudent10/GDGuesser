#include "HookedCCScene.hpp"
#include "ui/duels/DuelsPersistentNode.hpp"

//yoinked from music integrations, thanks me!
int HookedCCScene::getHighestChildZ() {
    if(auto dpn = CCScene::get()->getChildByType<DuelsPersistentNode>(0)) {
        dpn->setZOrder(0);
        int result = CCScene::getHighestChildZ();
        result > 130 ? dpn->setZOrder(result + 1) : dpn->setZOrder(130);
        return result;
    } else {
        return CCScene::getHighestChildZ();
    }
}
