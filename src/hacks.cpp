#include <Geode/Geode.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include "config.hpp"
#include "menu.hpp"

#ifdef GEODE_IS_WINDOWS
#include <Windows.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <string>
#include <vector>

using namespace geode::prelude;
using namespace std::chrono_literals;

namespace {
    using Clock = std::chrono::steady_clock;

    // Click state shared between the input hook and the HUD
    struct ClickState {
        std::deque<Clock::time_point> clicks;
        bool held = false;
        int total = 0;
    };
    ClickState g_click;

    // Requests from keys / on-screen buttons
    struct Control {
        bool step = false;        // advance one frame (frame stepper)
        int startposDelta = 0;    // -1 / +1 (startpos switcher)
    };
    Control g_ctl;

    // ---------------------------------------------------------------- FPS
    void applyFps() {
        int target = 60;
        if (srank::on("fps-enabled")) {
            target = static_cast<int>(srank::val("fps"));
        }
        target = std::max(target, 1);

        if (auto gm = GameManager::get()) {
            gm->m_customFPSTarget = target;
        }
        CCDirector::get()->setAnimationInterval(1.0 / target);
    }

    // -------------------------------------------------------- music pitch
    void applyPitch(float speed) {
        static float last = 1.f;
        float target = srank::on("speed-pitch") ? speed : 1.f;
        if (target == last) return;
        last = target;

        auto engine = FMODAudioEngine::sharedEngine();
        if (!engine || !engine->m_system) return;

        FMOD::ChannelGroup* group = nullptr;
        if (engine->m_system->getMasterChannelGroup(&group) == FMOD_OK && group) {
            group->setPitch(target);
        }
    }

    // ------------------------------------------------------------ hotkeys
    // Windows only. Android uses the on-screen buttons instead.
    //   Tab = open/close menu, . = step one frame, Left/Right = switch StartPos
    void pollKeys() {
#ifdef GEODE_IS_WINDOWS
        auto pressed = [](int vk, bool& was) {
            bool down = (GetAsyncKeyState(vk) & 0x8000) != 0 && GetActiveWindow() != nullptr;
            bool edge = down && !was;
            was = down;
            return edge;
        };

        static bool tab = false, period = false, left = false, right = false;
        if (pressed(VK_TAB, tab)) srank::toggleMenu();
        if (pressed(VK_OEM_PERIOD, period)) g_ctl.step = true;
        if (pressed(VK_LEFT, left)) g_ctl.startposDelta = -1;
        if (pressed(VK_RIGHT, right)) g_ctl.startposDelta = 1;
#endif
    }

    void setText(CCLabelBMFont* label, std::string& cache, std::string const& text) {
        if (text == cache) return;
        cache = text;
        label->setString(text.c_str());
    }
}

namespace srank {
    void notifyChanged(std::string const& id) {
        if (id == "fps" || id == "fps-enabled") applyFps();
    }
}

$on_mod(Loaded) {
    srank::loadConfig();
}

// ======================================================================
// Speedhack + hotkeys: runs every frame
// ======================================================================
class $modify(HPScheduler, CCScheduler) {
    virtual void update(float dt) {
        float speed = 1.f;
        if (srank::on("speedhack")) {
            speed = std::max(static_cast<float>(srank::val("speed")), 0.01f);
        }
        applyPitch(speed);
        CCScheduler::update(dt * speed);
        pollKeys();
    }
};

// ======================================================================
// Particles
// ======================================================================
class $modify(HPParticles, CCParticleSystemQuad) {
    virtual void draw() {
        if (srank::on("no-particles")) return;
        CCParticleSystemQuad::draw();
    }
};

// ======================================================================
// Jump hack
// ======================================================================
class $modify(HPPlayer, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);
        if (srank::on("jump-hack")) {
            m_isOnGround = true;
        }
    }
};

// ======================================================================
// Input tracking, screen shake, frame stepper
// ======================================================================
class $modify(HPBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);

        // button 1 = jump/click. Only player 1 so dual mode isn't counted twice.
        if (button != 1 || !isPlayer1) return;

        if (down) {
            g_click.clicks.push_back(Clock::now());
            g_click.held = true;
            g_click.total++;
        } else {
            g_click.held = false;
        }
    }

    void shakeCamera(float duration, float strength, float interval) {
        if (srank::on("no-shake")) return;
        GJBaseGameLayer::shakeCamera(duration, strength, interval);
    }

    void update(float dt) {
        bool inLevel = static_cast<GJBaseGameLayer*>(PlayLayer::get()) == this;
        if (inLevel && srank::on("frame-step")) {
            // Frozen until a step is requested, then run exactly one 240 TPS tick
            if (!g_ctl.step) return;
            g_ctl.step = false;
            GJBaseGameLayer::update(1.f / 240.f);
            return;
        }
        GJBaseGameLayer::update(dt);
    }
};

// ======================================================================
// Unlock all icons and colors
// ======================================================================
class $modify(HPGameManager, GameManager) {
    bool isIconUnlocked(int id, IconType type) {
        if (srank::on("unlock-all")) return true;
        return GameManager::isIconUnlocked(id, type);
    }

    bool isColorUnlocked(int id, UnlockType type) {
        if (srank::on("unlock-all")) return true;
        return GameManager::isColorUnlocked(id, type);
    }
};

// ======================================================================
// Main menu: apply FPS; on Android show an icon to open the menu
// ======================================================================
class $modify(HPMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        applyFps();

#ifdef GEODE_IS_ANDROID
        auto win = CCDirector::get()->getWinSize();
        auto openMenu = srank::makeOpenButton(this, 0.4f, 255);
        openMenu->setPosition({32.f, win.height - 32.f});
#endif
        return true;
    }
};

// ======================================================================
// Pause menu: banner + "Hacks" button
// ======================================================================
class $modify(HPPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        if (auto banner = CCSprite::create("srank-banner.png"_spr)) {
            auto size = CCDirector::get()->getWinSize();
            banner->setScale(0.22f);
            banner->setOpacity(210);
            banner->setPosition({size.width / 2.f, 22.f});
            this->addChild(banner, 10);
        }

        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Hacks"),
            this,
            menu_selector(HPPauseLayer::onHacks)
        );

        if (auto menu = this->getChildByID("right-button-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        } else {
            auto win = CCDirector::get()->getWinSize();
            auto fallback = CCMenu::create();
            fallback->addChild(btn);
            fallback->setPosition({win.width - 60.f, 40.f});
            this->addChild(fallback);
        }
    }

    void onHacks(CCObject*) {
        srank::toggleMenu();
    }
};

// ======================================================================
// Gameplay hooks: noclip, accuracy, HUD, coins, practice tools
// ======================================================================
class $modify(HPPlayLayer, PlayLayer) {
    struct Fields {
        // noclip accuracy
        int totalFrames = 0;
        int deathFrames = 0;
        bool diedThisFrame = false;

        // HUD
        CCLabelBMFont* cpsLabel = nullptr;
        CCLabelBMFont* fpsLabel = nullptr;
        CCLabelBMFont* clicksLabel = nullptr;
        CCLabelBMFont* timerLabel = nullptr;
        CCLabelBMFont* percentLabel = nullptr;
        CCLabelBMFont* accLabel = nullptr;
        CCLabelBMFont* startposLabel = nullptr;
        std::vector<CCLabelBMFont*> hudLabels;
        std::array<std::string, 7> lastText;
        CCDrawNode* indicator = nullptr;
        int lastIndicatorState = -1;

        // on-screen buttons (Android friendly)
        CCMenuItemSpriteExtra* stepBtn = nullptr;
        CCMenuItemSpriteExtra* prevBtn = nullptr;
        CCMenuItemSpriteExtra* nextBtn = nullptr;

        // coins
        std::vector<GameObject*> coins;
        std::vector<GameObject*> collected;

        // startpos switcher
        std::vector<StartPosObject*> startPositions;
        int startIndex = -1;

        // timers
        float checkpointTimer = 0.f;
        float deadTimer = 0.f;
        float sessionTime = 0.f;
        float fpsSmooth = 60.f;
        bool wasHidingPlayer = false;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        applyFps();
        g_click = ClickState{};

        auto f = m_fields.self();
        auto win = CCDirector::get()->getWinSize();

        // ---- HUD labels (stacked top-left, laid out every frame)
        auto makeLabel = [&]() {
            auto label = CCLabelBMFont::create("", "bigFont.fnt");
            label->setScale(0.4f);
            label->setAnchorPoint({0.f, 1.f});
            label->setOpacity(170);
            label->setPosition({8.f, win.height - 8.f});
            this->addChild(label, 100);
            f->hudLabels.push_back(label);
            return label;
        };
        f->cpsLabel = makeLabel();
        f->fpsLabel = makeLabel();
        f->clicksLabel = makeLabel();
        f->timerLabel = makeLabel();
        f->percentLabel = makeLabel();
        f->accLabel = makeLabel();
        f->startposLabel = makeLabel();

        // ---- Click indicator (bottom center)
        f->indicator = CCDrawNode::create();
        f->indicator->setPosition({win.width / 2.f, 14.f});
        this->addChild(f->indicator, 100);

        // ---- Faint Srank watermark (bottom-right)
        if (auto mark = CCSprite::create("srank-icon.png"_spr)) {
            mark->setScale(0.28f);
            mark->setOpacity(70);
            mark->setPosition({win.width - 24.f, 24.f});
            this->addChild(mark, 99);
        }

        // ---- On-screen buttons: STEP, < and > (shown only when the feature is on)
        auto controls = CCMenu::create();
        controls->setPosition({0.f, 0.f});
        auto makeButton = [&](char const* text, SEL_MenuHandler selector, CCPoint pos) {
            auto sprite = ButtonSprite::create(text);
            sprite->setScale(0.5f);
            auto item = CCMenuItemSpriteExtra::create(sprite, this, selector);
            item->setPosition(pos);
            controls->addChild(item);
            return item;
        };
        f->stepBtn = makeButton("STEP", menu_selector(HPPlayLayer::onStepButton), {win.width - 50.f, 90.f});
        f->prevBtn = makeButton("<", menu_selector(HPPlayLayer::onPrevStartPos), {win.width - 80.f, 56.f});
        f->nextBtn = makeButton(">", menu_selector(HPPlayLayer::onNextStartPos), {win.width - 36.f, 56.f});
        this->addChild(controls, 101);

#ifdef GEODE_IS_ANDROID
        // No keyboard on Android: small icon to open the menu
        auto openMenu = srank::makeOpenButton(this, 0.3f, 130);
        openMenu->setPosition({24.f, 24.f});
#endif

        // ---- Cache coins
        f->coins.clear();
        f->collected.clear();
        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (obj->m_objectType == GameObjectType::SecretCoin ||
                obj->m_objectType == GameObjectType::UserCoin) {
                f->coins.push_back(obj);
            }
        }

        // ---- Cache start positions (object id 31), sorted left to right
        f->startPositions.clear();
        f->startIndex = -1;
        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (obj->m_objectID == 31) {
                f->startPositions.push_back(static_cast<StartPosObject*>(obj));
            }
        }
        std::sort(f->startPositions.begin(), f->startPositions.end(),
            [](StartPosObject* a, StartPosObject* b) {
                return a->getPositionX() < b->getPositionX();
            });
    }

    void onStepButton(CCObject*) { g_ctl.step = true; }
    void onPrevStartPos(CCObject*) { g_ctl.startposDelta = -1; }
    void onNextStartPos(CCObject*) { g_ctl.startposDelta = 1; }

    void resetLevel() {
        PlayLayer::resetLevel();
        auto f = m_fields.self();
        f->totalFrames = 0;
        f->deathFrames = 0;
        f->diedThisFrame = false;
        f->checkpointTimer = 0.f;
        f->deadTimer = 0.f;
        f->collected.clear();
    }

    // Noclip: swallow deaths (but keep the anti-cheat spike working)
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (object == m_anticheatSpike || !srank::on("noclip")) {
            return PlayLayer::destroyPlayer(player, object);
        }

        bool isPlayer2 = player == m_player2;
        if ((isPlayer2 && !srank::on("noclip-p2")) || (!isPlayer2 && !srank::on("noclip-p1"))) {
            return PlayLayer::destroyPlayer(player, object);
        }

        m_fields->diedThisFrame = true;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto f = m_fields.self();
        auto win = CCDirector::get()->getWinSize();

        bool noclip = srank::on("noclip");
        bool fast = srank::on("speedhack") && srank::val("speed") != 1.0;
        bool coins = srank::on("auto-coins");
        bool jump = srank::on("jump-hack");
        bool alive = m_player1 && !m_player1->m_isDead;

        // ---- Safe mode: flag the run as a test run so cheated progress isn't saved
        if (srank::on("safe-mode") && (noclip || fast || coins || jump)) {
            m_isTestMode = true;
        }

        // ---- Noclip accuracy
        if (noclip && alive) {
            f->totalFrames++;
            if (f->diedThisFrame) f->deathFrames++;
        }
        f->diedThisFrame = false;

        float accuracy = 100.f;
        if (f->totalFrames > 0) {
            accuracy = 100.f * (1.f - static_cast<float>(f->deathFrames) / static_cast<float>(f->totalFrames));
        }

        double minAcc = srank::val("noclip-min-acc");
        if (noclip && alive && minAcc > 0.0 && f->totalFrames > 120 && accuracy < minAcc) {
            PlayLayer::destroyPlayer(m_player1, m_anticheatSpike);
        }

        // ---- Custom respawn time
        if (srank::on("respawn") && m_player1 && m_player1->m_isDead) {
            f->deadTimer += dt;
            if (f->deadTimer >= static_cast<float>(srank::val("respawn-time"))) {
                f->deadTimer = 0.f;
                this->stopActionByTag(0x10);   // cancel the game's own delayed respawn
                this->resetLevel();
            }
        } else {
            f->deadTimer = 0.f;
        }

        // ---- Auto checkpoints (practice mode only)
        if (srank::on("auto-checkpoint") && m_isPracticeMode && alive) {
            f->checkpointTimer += dt;
            if (f->checkpointTimer >= static_cast<float>(srank::val("checkpoint-interval"))) {
                f->checkpointTimer = 0.f;
                this->createCheckpoint();
            }
        }

        // ---- StartPos switcher
        int delta = g_ctl.startposDelta;
        g_ctl.startposDelta = 0;
        if (srank::on("startpos") && delta != 0 && !f->startPositions.empty()) {
            int n = static_cast<int>(f->startPositions.size());
            f->startIndex = ((f->startIndex + delta) % n + n) % n;
            this->setStartPosObject(f->startPositions[f->startIndex]);
            this->resetLevel();
        }

        // ---- Auto pickup coins
        if (coins && alive) {
            for (auto obj : f->coins) {
                if (std::find(f->collected.begin(), f->collected.end(), obj) != f->collected.end()) continue;
                f->collected.push_back(obj);
                m_player1->collidedWithObject(dt, obj, obj->getObjectRect(), false);
            }
        }

        // ---- Visual toggles
        if (srank::on("hide-player")) {
            if (m_player1) m_player1->setVisible(false);
            if (m_player2) m_player2->setVisible(false);
            f->wasHidingPlayer = true;
        } else if (f->wasHidingPlayer) {
            if (m_player1) m_player1->setVisible(true);
            f->wasHidingPlayer = false;
        }
        m_uiLayer->setVisible(!srank::on("hide-ui"));

        // ---- CPS
        auto now = Clock::now();
        while (!g_click.clicks.empty() && now - g_click.clicks.front() > 1s) {
            g_click.clicks.pop_front();
        }

        // ---- HUD
        f->sessionTime += dt;
        if (dt > 0.f) {
            f->fpsSmooth = f->fpsSmooth * 0.9f + (1.f / dt) * 0.1f;
        }

        auto show = [](CCLabelBMFont* label, bool visible) { label->setVisible(visible); };

        bool showCps = srank::on("cps-counter");
        show(f->cpsLabel, showCps);
        if (showCps) setText(f->cpsLabel, f->lastText[0], fmt::format("CPS: {}", g_click.clicks.size()));

        bool showFps = srank::on("fps-counter");
        show(f->fpsLabel, showFps);
        if (showFps) setText(f->fpsLabel, f->lastText[1], fmt::format("FPS: {:.0f}", f->fpsSmooth));

        bool showClicks = srank::on("total-clicks");
        show(f->clicksLabel, showClicks);
        if (showClicks) setText(f->clicksLabel, f->lastText[2], fmt::format("Clicks: {}", g_click.total));

        bool showTimer = srank::on("session-timer");
        show(f->timerLabel, showTimer);
        if (showTimer) {
            int total = static_cast<int>(f->sessionTime);
            setText(f->timerLabel, f->lastText[3], fmt::format("Time: {}:{:02}", total / 60, total % 60));
        }

        bool showPercent = srank::on("accurate-percent");
        show(f->percentLabel, showPercent);
        if (showPercent) setText(f->percentLabel, f->lastText[4], fmt::format("Progress: {:.2f}%", this->getCurrentPercent()));

        bool showAcc = noclip && srank::on("noclip-accuracy");
        show(f->accLabel, showAcc);
        if (showAcc) setText(f->accLabel, f->lastText[5], fmt::format("Accuracy: {:.2f}%", accuracy));

        bool showStartPos = srank::on("startpos") && !f->startPositions.empty();
        show(f->startposLabel, showStartPos);
        if (showStartPos) {
            setText(f->startposLabel, f->lastText[6],
                fmt::format("StartPos: {}/{}", f->startIndex + 1, f->startPositions.size()));
        }

        // Stack visible labels with no gaps
        float y = 8.f;
        for (auto label : f->hudLabels) {
            if (!label->isVisible()) continue;
            label->setPositionY(win.height - y);
            y += 15.f;
        }

        // On-screen buttons only when their feature is on
        f->stepBtn->setVisible(srank::on("frame-step"));
        f->prevBtn->setVisible(srank::on("startpos"));
        f->nextBtn->setVisible(srank::on("startpos"));

        // ---- Click indicator
        bool showIndicator = srank::on("click-indicator");
        f->indicator->setVisible(showIndicator);
        int state = g_click.held ? 1 : 0;
        if (showIndicator && state != f->lastIndicatorState) {
            f->lastIndicatorState = state;
            f->indicator->clear();
            ccColor4F color = state
                ? ccc4f(0.25f, 1.f, 0.45f, 0.95f)
                : ccc4f(1.f, 1.f, 1.f, 0.25f);
            f->indicator->drawDot({0.f, 0.f}, 8.f, color);
        }
    }
};
