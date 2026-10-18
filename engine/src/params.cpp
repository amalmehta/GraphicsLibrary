#include "gfx/params.hpp"

#include <cmath>
#include <cstdio>

namespace gfx {

namespace {
std::string esc(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}
std::string num(float v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.6g", v);
    return buf;
}
}  // namespace

Param& ParamSet::add(Param p) {
    p.group = group_;
    p.defaultValue = p.value;
    params_.push_back(std::move(p));
    return params_.back();
}

Param& ParamSet::slider(const std::string& id, const std::string& label, float value, float min,
                        float max, float step, const std::string& unit) {
    Param p;
    p.id = id; p.label = label; p.kind = Param::Kind::Slider;
    p.value = value; p.min = min; p.max = max; p.step = step; p.unit = unit;
    return add(p);
}

Param& ParamSet::toggle(const std::string& id, const std::string& label, bool value) {
    Param p;
    p.id = id; p.label = label; p.kind = Param::Kind::Toggle;
    p.value = value ? 1.f : 0.f; p.min = 0; p.max = 1; p.step = 1;
    return add(p);
}

Param& ParamSet::choice(const std::string& id, const std::string& label,
                        std::vector<std::string> options, int value) {
    Param p;
    p.id = id; p.label = label; p.kind = Param::Kind::Choice;
    p.max = float(options.size() - 1); p.step = 1;
    p.options = std::move(options);
    p.value = float(value);
    return add(p);
}

bool ParamSet::set(const std::string& id, float v) {
    for (auto& p : params_) {
        if (p.id != id) continue;
        if (std::isnan(v)) return false;
        switch (p.kind) {
            case Param::Kind::Slider: p.value = v < p.min ? p.min : (v > p.max ? p.max : v); break;
            case Param::Kind::Toggle: p.value = v >= 0.5f ? 1.f : 0.f; break;
            case Param::Kind::Choice: {
                float r = std::round(v);
                p.value = r < 0 ? 0 : (r > p.max ? p.max : r);
                break;
            }
        }
        return true;
    }
    return false;
}

float ParamSet::get(const std::string& id) const {
    for (const auto& p : params_)
        if (p.id == id) return p.value;
    return 0.f;
}

void ParamSet::resetAll() {
    for (auto& p : params_) p.value = p.defaultValue;
}

std::string ParamSet::toJson() const {
    std::string s = "[";
    for (size_t i = 0; i < params_.size(); ++i) {
        const Param& p = params_[i];
        const char* kind = p.kind == Param::Kind::Slider ? "slider"
                         : p.kind == Param::Kind::Toggle ? "toggle" : "choice";
        s += "{\"id\":\"" + esc(p.id) + "\",\"label\":\"" + esc(p.label) + "\",\"group\":\"" +
             esc(p.group) + "\",\"unit\":\"" + esc(p.unit) + "\",\"help\":\"" + esc(p.help) +
             "\",\"kind\":\"" + kind + "\",\"value\":" + num(p.value) +
             ",\"default\":" + num(p.defaultValue) + ",\"min\":" + num(p.min) +
             ",\"max\":" + num(p.max) + ",\"step\":" + num(p.step) + ",\"options\":[";
        for (size_t j = 0; j < p.options.size(); ++j)
            s += (j ? ",\"" : "\"") + esc(p.options[j]) + "\"";
        s += "]}";
        if (i + 1 < params_.size()) s += ",";
    }
    return s + "]";
}

}  // namespace gfx
