#include "config.hpp"

#include <unordered_map>

using namespace geode::prelude;

namespace srank {

    namespace {
        std::unordered_map<std::string, double>& store() {
            static std::unordered_map<std::string, double> values;
            return values;
        }
    }

    std::vector<std::string> const& tabs() {
        static std::vector<std::string> const t = {
            "Gameplay", "Practice", "Visual", "HUD", "General"
        };
        return t;
    }

    std::vector<Feature> const& features() {
        constexpr Kind T = Kind::Toggle;
        constexpr Kind S = Kind::Slider;

        // id, label, tab, kind, default, min, max, step
        static std::vector<Feature> const f = {
            // ---------------- Gameplay
            {"noclip",          "Noclip",               "Gameplay", T, 0, 0, 1, 1},
            {"noclip-p1",       "Noclip Player 1",      "Gameplay", T, 1, 0, 1, 1},
            {"noclip-p2",       "Noclip Player 2",      "Gameplay", T, 1, 0, 1, 1},
            {"noclip-accuracy", "Accuracy Label",       "Gameplay", T, 1, 0, 1, 1},
            {"noclip-min-acc",  "Die Below Accuracy %", "Gameplay", S, 0, 0, 100, 1},
            {"jump-hack",       "Jump Hack",            "Gameplay", T, 0, 0, 1, 1},
            {"speedhack",       "Speedhack",            "Gameplay", T, 0, 0, 1, 1},
            {"speed",           "Speed",                "Gameplay", S, 1, 0.1, 10, 0.1},
            {"speed-pitch",     "Speed Affects Music",  "Gameplay", T, 1, 0, 1, 1},
            {"auto-coins",      "Auto Pickup Coins",    "Gameplay", T, 0, 0, 1, 1},
            {"frame-step",      "Frame Stepper",        "Gameplay", T, 0, 0, 1, 1},
            {"safe-mode",       "Safe Mode",            "Gameplay", T, 1, 0, 1, 1},

            // ---------------- Practice
            {"auto-checkpoint",     "Auto Checkpoints",     "Practice", T, 0, 0, 1, 1},
            {"checkpoint-interval", "Checkpoint Every (s)", "Practice", S, 2, 0.5, 10, 0.5},
            {"startpos",            "StartPos Switcher",    "Practice", T, 0, 0, 1, 1},
            {"respawn",             "Custom Respawn Time",  "Practice", T, 0, 0, 1, 1},
            {"respawn-time",        "Respawn Time (s)",     "Practice", S, 1, 0, 3, 0.1},

            // ---------------- Visual
            {"no-particles", "No Particles",   "Visual", T, 0, 0, 1, 1},
            {"no-shake",     "No Screen Shake","Visual", T, 0, 0, 1, 1},
            {"hide-player",  "Hide Player",    "Visual", T, 0, 0, 1, 1},
            {"hide-ui",      "Hide UI",        "Visual", T, 0, 0, 1, 1},

            // ---------------- HUD
            {"cps-counter",      "CPS Counter",        "HUD", T, 1, 0, 1, 1},
            {"click-indicator",  "Click Indicator",    "HUD", T, 1, 0, 1, 1},
            {"fps-counter",      "FPS Counter",        "HUD", T, 0, 0, 1, 1},
            {"total-clicks",     "Total Clicks",       "HUD", T, 0, 0, 1, 1},
            {"session-timer",    "Session Timer",      "HUD", T, 0, 0, 1, 1},
            {"accurate-percent", "Accurate Percentage","HUD", T, 0, 0, 1, 1},

            // ---------------- General
            {"fps-enabled", "FPS Change",        "General", T, 0, 0, 1, 1},
            {"fps",         "FPS",               "General", S, 240, 30, 1000, 10},
            {"unlock-all",  "Unlock All Icons",  "General", T, 0, 0, 1, 1},
        };
        return f;
    }

    void loadConfig() {
        for (auto const& f : features()) {
            store()[f.id] = Mod::get()->getSavedValue<double>(f.id, f.def);
        }
    }

    double val(std::string const& id) {
        auto& s = store();
        auto it = s.find(id);
        if (it != s.end()) return it->second;

        // Not loaded yet: fall back to the default
        for (auto const& f : features()) {
            if (id == f.id) return f.def;
        }
        return 0.0;
    }

    bool on(std::string const& id) {
        return val(id) > 0.5;
    }

    void set(std::string const& id, double value) {
        store()[id] = value;
        Mod::get()->setSavedValue<double>(id, value);
    }
}
