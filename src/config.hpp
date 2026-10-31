#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

namespace srank {

    enum class Kind { Toggle, Slider };

    struct Feature {
        char const* id;
        char const* label;
        char const* tab;
        Kind kind;
        double def;
        double min;
        double max;
        double step;
    };

    // Every feature shown in the menu, in display order
    std::vector<Feature> const& features();
    std::vector<std::string> const& tabs();

    // Load saved values (call once when the mod loads)
    void loadConfig();

    // Read a value. Toggles are 0 or 1.
    double val(std::string const& id);
    bool on(std::string const& id);

    // Write a value and save it
    void set(std::string const& id, double value);

    // Called by the menu after a value changes (defined in hacks.cpp)
    void notifyChanged(std::string const& id);
}
