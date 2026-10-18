// Parameter registry: every tweakable value a scene exposes. The web UI builds
// its controls from toJson(), so a new scene gets sliders without touching JS.
#pragma once
#include <string>
#include <vector>

namespace gfx {

struct Param {
    enum class Kind { Slider, Toggle, Choice };
    std::string id, label, group, unit, help;
    Kind kind = Kind::Slider;
    float value = 0, defaultValue = 0, min = 0, max = 1, step = 0.01f;
    std::vector<std::string> options;  // Choice only; value is the index
};

class ParamSet {
public:
    Param& slider(const std::string& id, const std::string& label, float value, float min,
                  float max, float step, const std::string& unit = "");
    Param& toggle(const std::string& id, const std::string& label, bool value);
    Param& choice(const std::string& id, const std::string& label,
                  std::vector<std::string> options, int value);

    // Returns false for unknown ids. Sliders clamp, toggles snap to 0/1,
    // choices round to a valid index.
    bool set(const std::string& id, float value);
    float get(const std::string& id) const;
    bool flag(const std::string& id) const { return get(id) >= 0.5f; }
    void resetAll();

    const std::vector<Param>& all() const { return params_; }
    std::string toJson() const;

    // Groups added after this call are tagged with the given group name.
    void group(const std::string& name) { group_ = name; }

private:
    Param& add(Param p);
    std::vector<Param> params_;
    std::string group_ = "General";
};

}  // namespace gfx
