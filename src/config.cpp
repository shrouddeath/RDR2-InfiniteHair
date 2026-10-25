#include "config.h"

#include <windows.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "groom.h"

Hash joaat(const std::string& text) {
    uint32_t hash = 0;
    for (unsigned char c : text) {
        hash += static_cast<uint32_t>(std::tolower(c));
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

Hash parseHash(const std::string& raw) {
    std::string text = trim(raw);
    if (text.empty()) return 0;
    if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
        return static_cast<Hash>(std::strtoul(text.c_str() + 2, nullptr, 16));
    bool numeric = text.find_first_not_of("-0123456789") == std::string::npos;
    if (numeric) return static_cast<Hash>(std::strtoll(text.c_str(), nullptr, 10));
    return joaat(text);
}

static std::string readString(const std::string& file, const char* section, const char* key, const char* fallback) {
    char buffer[2048];
    GetPrivateProfileStringA(section, key, fallback, buffer, sizeof(buffer), file.c_str());
    // Strip trailing ";" comments, which GetPrivateProfileString keeps.
    std::string value = buffer;
    size_t comment = value.find(';');
    if (comment != std::string::npos) value.resize(comment);
    return trim(value);
}

static bool readBool(const std::string& file, const char* section, const char* key, bool fallback) {
    std::string v = readString(file, section, key, fallback ? "1" : "0");
    return v == "1" || _stricmp(v.c_str(), "true") == 0 || _stricmp(v.c_str(), "yes") == 0;
}

static float readFloat(const std::string& file, const char* section, const char* key, float fallback) {
    std::string v = readString(file, section, key, "");
    return v.empty() ? fallback : static_cast<float>(std::atof(v.c_str()));
}

static int readInt(const std::string& file, const char* section, const char* key, int fallback) {
    std::string v = readString(file, section, key, "");
    return v.empty() ? fallback : std::atoi(v.c_str());
}

// "F1".."F24", a few named keys, "0x7A", "122", or "0"/"" for disabled.
static int parseKey(const std::string& text) {
    if (text.empty()) return 0;
    static const struct { const char* name; int vk; } named[] = {
        {"Home", VK_HOME}, {"End", VK_END}, {"PageUp", VK_PRIOR}, {"PageDown", VK_NEXT},
        {"Insert", VK_INSERT}, {"Delete", VK_DELETE},
    };
    for (const auto& k : named)
        if (_stricmp(text.c_str(), k.name) == 0) return k.vk;
    if ((text[0] == 'F' || text[0] == 'f') && text.size() > 1) {
        int n = std::atoi(text.c_str() + 1);
        if (n >= 1 && n <= 24) return VK_F1 + n - 1;
    }
    if (text.size() == 1 && std::isalnum(static_cast<unsigned char>(text[0])))
        return std::toupper(static_cast<unsigned char>(text[0]));
    return static_cast<int>(parseHash(text));
}

static std::vector<std::string> splitList(const std::string& text) {
    std::vector<std::string> out;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) out.push_back(item);
    }
    return out;
}

Config loadConfig(const std::string& dir) {
    Config cfg;
    std::string settings = dir + "\\InfiniteHair.ini";

    cfg.enabled = readBool(settings, "General", "Enabled", cfg.enabled);
    cfg.onlyArthur = readBool(settings, "General", "OnlyArthur", cfg.onlyArthur);
    cfg.turbo = _stricmp(readString(settings, "General", "GrowthMode", "natural").c_str(), "turbo") == 0;
    cfg.hoursPerStep = readFloat(settings, "General", "HoursPerStep", cfg.hoursPerStep);
    if (cfg.hoursPerStep < 0.1f) cfg.hoursPerStep = 0.1f;
    cfg.growIntervalSeconds = readFloat(settings, "General", "GrowIntervalSeconds", cfg.growIntervalSeconds);
    if (cfg.growIntervalSeconds < 1.0f) cfg.growIntervalSeconds = 1.0f;
    cfg.lockBarber = readBool(settings, "General", "LockBarber", cfg.lockBarber);
    cfg.showNotifications = readBool(settings, "General", "ShowNotifications", cfg.showNotifications);
    cfg.recordMode = readBool(settings, "Debug", "RecordMode", cfg.recordMode);

    cfg.toggleKey = parseKey(readString(settings, "Keys", "Toggle", "F10"));
    cfg.dumpKey = parseKey(readString(settings, "Keys", "DumpComponents", "F9"));
    cfg.growNowKey = parseKey(readString(settings, "Keys", "GrowNow", "End"));
    cfg.testCutKey = parseKey(readString(settings, "Keys", "TestCut", "Delete"));

    for (int p = 0; p < kPartCount; ++p) {
        const char* section = partKey(static_cast<Part>(p));
        cfg.parts[p].enabled = readBool(settings, section, "Enabled", true);
        cfg.parts[p].maxLength = readInt(settings, section, "MaxLength", 0);
    }

    cfg.extraDrawables = splitList(readString(settings, "Extra", "Drawables", ""));

    // [Chains] lines come back as "key=value\0key=value\0\0".
    std::vector<char> section(64 * 1024);
    DWORD size = GetPrivateProfileSectionA("Chains", section.data(), static_cast<DWORD>(section.size()),
                                           settings.c_str());
    for (const char* line = section.data(); line < section.data() + size && *line; line += strlen(line) + 1) {
        std::string entry = line;
        size_t eq = entry.find('=');
        if (entry[0] == ';' || eq == std::string::npos) continue;
        std::string value = entry.substr(eq + 1);
        size_t comment = value.find(';');
        if (comment != std::string::npos) value.resize(comment);
        cfg.chains.emplace_back(trim(entry.substr(0, eq)), trim(value));
    }
    return cfg;
}

State loadState(const std::string& dir) {
    std::string file = dir + "\\InfiniteHair.state";
    State s;
    for (int p = 0; p < kPartCount; ++p) {
        std::string part = partKey(static_cast<Part>(p));
        s[part + "Length"] = GetPrivateProfileIntA("State", (part + "Length").c_str(), -1, file.c_str());
        s[part + "Minutes"] = GetPrivateProfileIntA("State", (part + "Minutes").c_str(), 0, file.c_str());
    }
    return s;
}

void saveState(const std::string& dir, const State& state) {
    std::string file = dir + "\\InfiniteHair.state";
    for (const auto& [key, value] : state)
        WritePrivateProfileStringA("State", key.c_str(), std::to_string(value).c_str(), file.c_str());
}
