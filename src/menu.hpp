#pragma once

#include <Geode/Geode.hpp>

namespace srank {

    // Open the menu if it's closed, close it if it's open
    void toggleMenu();

    // Adds a small Srank icon button to `parent` that toggles the menu.
    // Returns the menu holding the button; the caller sets its position.
    cocos2d::CCMenu* makeOpenButton(cocos2d::CCNode* parent, float scale, unsigned char opacity);
}
