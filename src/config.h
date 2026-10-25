#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

using Hash = uint32_t;

// Rockstar's "one at a time" hash, same as GET_HASH_KEY (lower-cases input).
Hash joaat(const std::string& text);

// Accepts "0x1234ABCD", a decimal number, or a name that gets joaat-hashed.
Hash parseHash(const std::string& text);

struct PartConfig {
    bool enabled = true;
    int maxLength = 0;   // 0 = up to the longest known drawable
};

struct Config {
    bool enabled = true;
    bool onlyArthur = true;
    // natural: the game grows hair as usual; when it hasn't grown a part for
    // hoursPerStep in-game hours (e.g. at the vanilla maximum), the mod grows it.
    // turbo: one step every growIntervalSeconds real seconds.
    bool turbo = false;
    float hoursPerStep = 48.0f;          // natural: in-game hours per length step
    float growIntervalSeconds = 30.0f;   // turbo: real seconds per length step
    bool lockBarber = false;             // undo haircuts/shaves done by the game
    bool showNotifications = true;
    bool recordMode = false;             // log every hair/beard change

    int toggleKey = 0x79;   // F10
    int dumpKey = 0x78;     // F9
    int growNowKey = 0x23;  // End
    int testCutKey = 0x2E;  // Delete: cut everything to the shortest length

    PartConfig parts[4];                       // indexed by Part (groom.h)
    std::vector<std::string> extraDrawables;   // custom longer models (step 2)
    std::vector<std::pair<std::string, std::string>> chains;   // [Chains] virtual length = borrowed slot
};

// Reads InfiniteHair.ini.
Config loadConfig(const std::string& dir);

// Persisted per part: "<Part>Length" (target length) and "<Part>Minutes"
// (in-game minutes since the last growth step), so growth survives restarts.
using State = std::map<std::string, int>;

State loadState(const std::string& dir);
void saveState(const std::string& dir, const State& state);
