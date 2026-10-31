#include "menu.hpp"
#include "config.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

using namespace geode::prelude;

namespace {
    constexpr float W = 460.f;        // window width
    constexpr float H = 270.f;        // window height
    constexpr float TAB_W = 96.f;     // left tab column width
    constexpr float TITLE_H = 30.f;   // title bar height
    constexpr int ROWS = 7;           // rows per column
}

class SrankMenu : public CCLayer {
public:
    CCNode* m_root = nullptr;
    CCNode* m_content = nullptr;
    std::vector<CCLabelBMFont*> m_tabLabels;
    std::unordered_map<int, CCLabelBMFont*> m_valueLabels;
    int m_tab = 0;

    static SrankMenu* create() {
        auto ret = new SrankMenu();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;

        auto win = CCDirector::get()->getWinSize();

        m_root = CCNode::create();
        m_root->setPosition({win.width / 2.f - W / 2.f, win.height / 2.f - H / 2.f});
        this->addChild(m_root);

        // Invisible blocker: swallows clicks inside the window so they don't reach the game.
        // Lower touch priority than the real buttons so the buttons still win.
        auto blockMenu = CCMenu::create();
        blockMenu->setPosition({0.f, 0.f});
        blockMenu->setTouchPriority(-100);
        auto blocker = CCMenuItem::create();
        blocker->setContentSize({W, H});
        blocker->setPosition({W / 2.f, H / 2.f});
        blockMenu->addChild(blocker);
        m_root->addChild(blockMenu, 0);

        // Backgrounds
        auto bg = CCLayerColor::create(ccc4(12, 10, 26, 235), W, H);
        m_root->addChild(bg, -2);

        auto tabBg = CCLayerColor::create(ccc4(22, 17, 48, 255), TAB_W, H - TITLE_H);
        m_root->addChild(tabBg, -1);

        auto title = CCLayerColor::create(ccc4(76, 42, 150, 255), W, TITLE_H);
        title->setPosition({0.f, H - TITLE_H});
        m_root->addChild(title, -1);

        // Title bar
        if (auto icon = CCSprite::create("srank-icon.png"_spr)) {
            icon->setScale(0.2f);
            icon->setPosition({18.f, H - TITLE_H / 2.f});
            m_root->addChild(icon, 2);
        }
        auto titleLabel = CCLabelBMFont::create("SRANK MOD MENU", "bigFont.fnt");
        titleLabel->setScale(0.45f);
        titleLabel->setAnchorPoint({0.f, 0.5f});
        titleLabel->setPosition({36.f, H - TITLE_H / 2.f});
        m_root->addChild(titleLabel, 2);

        // Close button
        auto closeMenu = CCMenu::create();
        closeMenu->setPosition({0.f, 0.f});
        m_root->addChild(closeMenu, 3);
        auto xSprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        xSprite->setScale(0.5f);
        auto xButton = CCMenuItemSpriteExtra::create(xSprite, this, menu_selector(SrankMenu::onClose));
        xButton->setPosition({W - 18.f, H - TITLE_H / 2.f});
        closeMenu->addChild(xButton);

        // Tabs
        auto tabMenu = CCMenu::create();
        tabMenu->setPosition({0.f, 0.f});
        m_root->addChild(tabMenu, 3);

        auto const& tabNames = srank::tabs();
        for (size_t i = 0; i < tabNames.size(); ++i) {
            auto label = CCLabelBMFont::create(tabNames[i].c_str(), "bigFont.fnt");
            label->setScale(0.45f);
            auto item = CCMenuItemLabel::create(label, this, menu_selector(SrankMenu::onTab));
            item->setTag(static_cast<int>(i));
            item->setPosition({TAB_W / 2.f, H - TITLE_H - 28.f - static_cast<float>(i) * 34.f});
            tabMenu->addChild(item);
            m_tabLabels.push_back(label);
        }

        // Content area
        m_content = CCNode::create();
        m_content->setPosition({TAB_W, 0.f});
        m_root->addChild(m_content, 3);

        buildTab(0);
        return true;
    }

    void buildTab(int tab) {
        m_tab = tab;
        m_valueLabels.clear();
        m_content->removeAllChildrenWithCleanup(true);

        for (size_t i = 0; i < m_tabLabels.size(); ++i) {
            bool selected = static_cast<int>(i) == tab;
            m_tabLabels[i]->setColor(selected ? ccc3(255, 255, 255) : ccc3(150, 140, 190));
        }

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_content->addChild(menu, 1);

        auto const& feats = srank::features();
        auto const& tabNames = srank::tabs();
        float colW = (W - TAB_W) / 2.f;

        int slot = 0;
        for (size_t i = 0; i < feats.size(); ++i) {
            auto const& f = feats[i];
            if (tabNames[tab] != f.tab) continue;

            int col = slot / ROWS;
            int row = slot % ROWS;
            ++slot;

            float x0 = static_cast<float>(col) * colW;
            float y = H - TITLE_H - 26.f - static_cast<float>(row) * 31.f;

            if (f.kind == srank::Kind::Toggle) {
                auto toggler = CCMenuItemToggler::createWithStandardSprites(
                    this, menu_selector(SrankMenu::onToggle), 0.55f
                );
                toggler->setTag(static_cast<int>(i));
                toggler->toggle(srank::on(f.id));
                toggler->setPosition({x0 + 20.f, y});
                menu->addChild(toggler);

                auto label = CCLabelBMFont::create(f.label, "bigFont.fnt");
                label->setScale(0.34f);
                label->setAnchorPoint({0.f, 0.5f});
                label->setPosition({x0 + 38.f, y});
                m_content->addChild(label, 1);
            } else {
                // Slider rows: label on top, "-  value  +" below it
                auto label = CCLabelBMFont::create(f.label, "bigFont.fnt");
                label->setScale(0.34f);
                label->setAnchorPoint({0.f, 0.5f});
                label->setPosition({x0 + 12.f, y + 9.f});
                m_content->addChild(label, 1);

                auto minusSprite = ButtonSprite::create("-");
                minusSprite->setScale(0.55f);
                auto minus = CCMenuItemSpriteExtra::create(minusSprite, this, menu_selector(SrankMenu::onMinus));
                minus->setTag(static_cast<int>(i));
                minus->setPosition({x0 + 28.f, y - 8.f});
                menu->addChild(minus);

                auto value = CCLabelBMFont::create("", "bigFont.fnt");
                value->setScale(0.4f);
                value->setPosition({x0 + 84.f, y - 8.f});
                m_content->addChild(value, 1);
                m_valueLabels[static_cast<int>(i)] = value;

                auto plusSprite = ButtonSprite::create("+");
                plusSprite->setScale(0.55f);
                auto plus = CCMenuItemSpriteExtra::create(plusSprite, this, menu_selector(SrankMenu::onPlus));
                plus->setTag(static_cast<int>(i));
                plus->setPosition({x0 + 140.f, y - 8.f});
                menu->addChild(plus);

                refreshValue(static_cast<int>(i));
            }
        }
    }

    void refreshValue(int index) {
        auto it = m_valueLabels.find(index);
        if (it == m_valueLabels.end()) return;

        auto const& f = srank::features()[index];
        double v = srank::val(f.id);
        std::string text = f.step >= 1.0 ? fmt::format("{:.0f}", v) : fmt::format("{:.2f}", v);
        it->second->setString(text.c_str());
    }

    void stepValue(int index, int direction) {
        auto const& f = srank::features()[index];
        double v = srank::val(f.id) + direction * f.step;
        v = std::clamp(v, f.min, f.max);
        v = std::round(v / f.step) * f.step;   // avoid floating point drift
        v = std::clamp(v, f.min, f.max);
        srank::set(f.id, v);
        srank::notifyChanged(f.id);
        refreshValue(index);
    }

    void onTab(CCObject* sender) {
        buildTab(static_cast<CCNode*>(sender)->getTag());
    }

    void onToggle(CCObject* sender) {
        int index = static_cast<CCNode*>(sender)->getTag();
        auto const& f = srank::features()[index];
        srank::set(f.id, srank::on(f.id) ? 0.0 : 1.0);
        srank::notifyChanged(f.id);
    }

    void onMinus(CCObject* sender) {
        stepValue(static_cast<CCNode*>(sender)->getTag(), -1);
    }

    void onPlus(CCObject* sender) {
        stepValue(static_cast<CCNode*>(sender)->getTag(), 1);
    }

    void onClose(CCObject*) {
        srank::toggleMenu();
    }
};

// Small helper node so the open-button has an object to call back into
class SrankOpener : public CCNode {
public:
    static SrankOpener* create() {
        auto ret = new SrankOpener();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    void onOpen(CCObject*) {
        srank::toggleMenu();
    }
};

namespace srank {

    void toggleMenu() {
        auto scene = CCDirector::get()->getRunningScene();
        if (!scene) return;

        // Already open in this scene -> close it
        if (auto existing = scene->getChildByID("srank-menu"_spr)) {
            existing->removeFromParent();
            return;
        }

        auto menu = SrankMenu::create();
        if (!menu) return;
        menu->setID("srank-menu"_spr);
        scene->addChild(menu, 100000);
    }

    CCMenu* makeOpenButton(CCNode* parent, float scale, unsigned char opacity) {
        auto opener = SrankOpener::create();
        parent->addChild(opener);

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        parent->addChild(menu, 100);

        if (auto icon = CCSprite::create("srank-icon.png"_spr)) {
            icon->setScale(scale);
            icon->setOpacity(opacity);
            auto button = CCMenuItemSpriteExtra::create(icon, opener, menu_selector(SrankOpener::onOpen));
            menu->addChild(button);
        }
        return menu;
    }
}
