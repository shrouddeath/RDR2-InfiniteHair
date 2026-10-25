// Infinite Hair - Arthur's hair and beard keep growing, length by length.
//
// RDR2 has no "hair length" native. A shop item only picks the style; the
// length is a separate drawable per style (see groom.h). We read the drawables
// Arthur is wearing, keep a target length per part (hair, chin, mustache,
// chops) and swap in the drawable of the target length with _SET_META_PED_TAG,
// keeping the current textures and tint.

#include "script.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "config.h"
#include "groom.h"
#include "log.h"
#include "natives.h"

static HMODULE g_module = nullptr;
static std::string g_dir;
static Config g_cfg;
static State g_state;
static bool g_paused = false;

static std::string g_message;
static DWORD g_messageUntil = 0;

// The barber, shops, cutscenes and menus swap hair/beard themselves. We stay
// hands-off while one of them runs and for a moment afterwards.
static const DWORD kSettleMs = 3000;
static DWORD g_busyUntil = 0;
static bool g_wasBusy = false;

// A swap needs a moment to show up in GET_META_PED_ASSET_GUIDS.
static const DWORD kApplyWaitMs = 1500;
static const int kMaxFailedApplies = 3;

struct PartState {
    int pendingLength = -1;   // length we just applied and wait to see
    DWORD pendingUntil = 0;
    int failures = 0;
    int blockedStyle = -1;    // style/length that never showed up after applying
    int blockedLength = -1;
    bool growthOwed = false;  // a growth step waits for a probe answer
    bool maxAnnounced = false;
    bool duplicateWarned = false;
    int lastSeen = -1;        // length seen in the previous tick
    bool cutWhileBusy = false;  // got shorter at the barber / camp mirror
    int savedMinutes = 0;     // minutes value last written to the state file
};
static PartState g_parts[kPartCount];

// In-game minutes that passed since the last tick that handled growth.
static int g_minutesBank = 0;

static const Hash kArthurModel = joaat("player_zero");
static const char* kVersion = "1.0.0";

static int& targetOf(Part part) { return g_state[std::string(partKey(part)) + "Length"]; }
static int& minutesOf(Part part) { return g_state[std::string(partKey(part)) + "Minutes"]; }

// Days since 1970-01-01 for a proleptic Gregorian date (H. Hinnant's algorithm).
static long long daysFromCivil(int y, int m, int d) {
    y -= m <= 2 ? 1 : 0;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const int yoe = y - era * 400;
    const int doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<long long>(era) * 146097 + doe - 719468;
}

// In-game minutes since the previous call. Sleeping, waiting and travelling
// advance the clock just like the game's own hair growth expects. Jumps
// backwards or by more than two days (loading another save) don't count.
static int gameMinutesElapsed() {
    static long long last = -1;
    int month = GET_CLOCK_MONTH() + 1;   // the clock natives count months from 0
    long long now = daysFromCivil(GET_CLOCK_YEAR(), month, GET_CLOCK_DAY_OF_MONTH()) * 1440 +
                    GET_CLOCK_HOURS() * 60 + GET_CLOCK_MINUTES();
    long long delta = last < 0 ? 0 : now - last;
    last = now;
    return delta > 0 && delta <= 2 * 1440 ? static_cast<int>(delta) : 0;
}

void ScriptSetModule(HMODULE module) { g_module = module; }

static std::string moduleDirectory() {
    char path[MAX_PATH] = {};
    GetModuleFileNameA(g_module, path, MAX_PATH);
    std::string dir = path;
    size_t slash = dir.find_last_of("\\/");
    return slash == std::string::npos ? "." : dir.substr(0, slash);
}

static std::string hex(Hash h) {
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%08X", h);
    return buf;
}

static void notify(const std::string& text) {
    logfile::write("%s", text.c_str());
    if (!g_cfg.showNotifications) return;
    g_message = "Infinite Hair: " + text;
    g_messageUntil = GetTickCount() + 4000;
}

static void drawText(const std::string& text, float y) {
    _SET_TEXT_COLOR(255, 255, 255, 230);
    _DISPLAY_TEXT(VAR_STRING_LITERAL(text.c_str()), 0.02f, y);
}

// Edge-triggered key press (fires once per press, not while held).
static bool keyPressed(int vk) {
    static bool down[256] = {};
    if (vk <= 0 || vk > 255) return false;
    bool now = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool pressed = now && !down[vk];
    down[vk] = now;
    return pressed;
}

static bool playerUsable(Ped& outPed) {
    if (NETWORK_IS_SESSION_STARTED()) return false;   // never touch Red Dead Online
    if (IS_LOADING_SCREEN_VISIBLE() || IS_SCREEN_FADED_OUT()) return false;
    if (!IS_PLAYER_PLAYING(PLAYER_ID())) return false;

    Ped ped = PLAYER_PED_ID();
    if (!DOES_ENTITY_EXIST(ped) || IS_ENTITY_DEAD(ped)) return false;
    if (g_cfg.onlyArthur && GET_ENTITY_MODEL(ped) != kArthurModel) return false;

    outPed = ped;
    return true;
}

// True while the game itself is likely changing hair or beard (barber chair,
// shop menus, cutscenes, pause menu), and for kSettleMs afterwards.
static bool gameIsBusy(Ped ped) {
    bool busy = !IS_PLAYER_CONTROL_ON(PLAYER_ID()) || IS_PED_USING_ANY_SCENARIO(ped) || IS_PAUSE_MENU_ACTIVE();
    DWORD now = GetTickCount();
    if (busy) g_busyUntil = now + kSettleMs;
    bool result = busy || static_cast<LONG>(g_busyUntil - now) > 0;
    if (result != g_wasBusy) logfile::write(result ? "[busy] game in control - mod waits" : "[busy] free again");
    g_wasBusy = result;
    return result;
}

// ---- reading the ped ---------------------------------------------------------

struct Asset {
    int index = 0;
    Hash shopItem = 0, category = 0, wearable = 0;
    bool hasGuids = false;
    Hash drawable = 0, albedo = 0, normal = 0, material = 0, palette = 0;
    int tint0 = 0, tint1 = 0, tint2 = 0;
    bool groom = false;   // drawable is a hair/beard length we understand
    DrawableInfo info;
};

static std::vector<Asset> readAssets(Ped ped) {
    std::vector<Asset> out;
    int metaPedType = _GET_META_PED_TYPE(ped);
    int count = _GET_NUM_COMPONENTS_IN_PED(ped);
    for (int i = 0; i < count; ++i) {
        Asset a;
        a.index = i;
        a.shopItem = _GET_SHOP_ITEM_COMPONENT_AT_INDEX(ped, i, &a.wearable);
        if (a.shopItem) a.category = _GET_SHOP_ITEM_COMPONENT_CATEGORY(a.shopItem, metaPedType, false);
        a.hasGuids = GET_META_PED_ASSET_GUIDS(ped, i, a.drawable, a.albedo, a.normal, a.material);
        if (a.hasGuids) {
            GET_META_PED_ASSET_TINT(ped, i, a.palette, a.tint0, a.tint1, a.tint2);
            a.groom = parseDrawable(a.drawable, a.info);
        }
        out.push_back(a);
    }
    return out;
}

static const Asset* findPart(const std::vector<Asset>& assets, Part part, int* count = nullptr) {
    const Asset* first = nullptr;
    int n = 0;
    for (const Asset& a : assets) {
        if (!a.groom || a.info.part != part) continue;
        if (!first) first = &a;
        ++n;
    }
    if (count) *count = n;
    return first;
}

static std::string drawableLabel(const Asset& a) {
    return a.groom ? drawableName(a.info) : hex(a.drawable);
}

static std::string categoryName(Hash category) {
    static const char* known[] = {"hair", "beards_complete", "beards_chin", "beards_mustache", "beards_chops",
                                  "heads", "eyes", "teeth", "hats", "eyebrows", "bodies_upper", "bodies_lower"};
    for (const char* name : known)
        if (joaat(name) == category) return name;
    return hex(category);
}

// ---- probing: does a drawable exist? -----------------------------------------
// Requesting a drawable that isn't in the game returns -1 at once; an existing
// one loads within a few frames (verified in game on 25.10.2026). Longer
// models added with LML are found this way and then grown into automatically.

enum class Probe { Unknown, Pending, Exists, Missing };

struct ProbeEntry {
    std::string name;
    Probe result = Probe::Unknown;
    int request = -1;
    DWORD started = 0;
};
static std::map<Hash, ProbeEntry> g_probes;   // per session; LML only loads files at startup

static Probe probeDrawable(const std::string& name, const Asset& a) {
    ProbeEntry& e = g_probes[joaat(name)];
    if (e.result != Probe::Unknown) return e.result;
    e.name = name;
    e.request = REQUEST_META_PED_ASSET_SET(joaat(name), a.albedo, a.normal, a.material);
    if (e.request < 0) {
        e.result = Probe::Missing;
        logfile::write("[probe] %s: missing", name.c_str());
    } else {
        e.result = Probe::Pending;
        e.started = GetTickCount();
    }
    return e.result;
}

static void pollProbes() {
    for (auto& [hash, e] : g_probes) {
        if (e.result != Probe::Pending) continue;
        bool loaded = _HAS_META_PED_ASSET_LOADED(e.request);
        bool valid = _IS_META_PED_ASSET_VALID(e.request);
        if (!loaded && valid && GetTickCount() - e.started <= 5000) continue;

        e.result = loaded ? Probe::Exists : Probe::Missing;
        _RELEASE_META_PED_ASSET_REQUEST(e.request);
        DrawableInfo info;
        bool isNew = loaded && parseDrawable(hash, info) && !drawableKnown(info);
        if (isNew) addExtraDrawable(e.name);
        logfile::write("[probe] %s: %s", e.name.c_str(),
                       !loaded ? "missing" : isNew ? "exists - NEW length found" : "exists");
    }
}

// ---- diagnostics -------------------------------------------------------------

// `announce`: confirm on screen (F9). The automatic dump after loading is silent.
static void dumpAssets(Ped ped, const std::vector<Asset>& assets, bool announce) {
    logfile::write("---- dump (%d entries, model 0x%08X) ----", static_cast<int>(assets.size()), GET_ENTITY_MODEL(ped));
    for (const Asset& a : assets) {
        logfile::write("#%02d item=%s cat=%s wear=%s | drawable=%s ab=%s nm=%s ma=%s | pal=%s tint=%d/%d/%d", a.index,
                       hex(a.shopItem).c_str(), categoryName(a.category).c_str(), hex(a.wearable).c_str(),
                       a.hasGuids ? drawableLabel(a).c_str() : "-", hex(a.albedo).c_str(), hex(a.normal).c_str(),
                       hex(a.material).c_str(), hex(a.palette).c_str(), a.tint0, a.tint1, a.tint2);
        if (!a.groom) continue;

        int minLen = minKnownLength(a.info), maxLen = maxKnownLength(a.info);
        logfile::write("    -> %s style %d, length %d (known: %d..%d)", partLabel(a.info.part), a.info.style,
                       a.info.length, minLen, maxLen);
        // Control probe (should exist) plus a few lengths past the known maximum.
        // Borrowed slots answer under the slot's name, not the virtual one.
        if (drawableHash(a.info) == joaat(drawableName(a.info))) probeDrawable(drawableName(a.info), a);
        DrawableInfo next = a.info;
        for (next.length = std::max(maxLen, a.info.length) + 1;
             next.length <= std::min(kMaxProbeLength, std::max(maxLen, a.info.length) + 3); ++next.length)
            probeDrawable(drawableName(next), a);
    }
    if (announce) notify("parts written to InfiniteHair.log");
}

// RecordMode: log every change of a shop item or a hair/beard drawable.
static void recordChanges(const std::vector<Asset>& assets) {
    static std::map<std::string, std::string> previous;
    std::map<std::string, std::string> now;
    for (const Asset& a : assets) {
        if (a.shopItem) now["item " + categoryName(a.category)] += hex(a.shopItem) + "/" + hex(a.wearable) + " ";
        if (a.groom) now[std::string("drawable ") + partLabel(a.info.part)] += drawableName(a.info) + " ";
    }
    if (!previous.empty()) {
        for (const auto& [key, value] : now) {
            auto it = previous.find(key);
            if (it == previous.end() || it->second != value)
                logfile::write("[change] %s: %s-> %s", key.c_str(), it == previous.end() ? "- " : it->second.c_str(),
                               value.c_str());
        }
        for (const auto& [key, value] : previous)
            if (!now.count(key)) logfile::write("[change] %s: %s-> -", key.c_str(), value.c_str());
    }
    previous = std::move(now);
}

// ---- growth ------------------------------------------------------------------

// Textures last seen on the extension style, for switching another style into it.
struct Textures {
    bool known = false;
    Hash albedo = 0, normal = 0, material = 0, palette = 0;
    int tint0 = 0, tint1 = 0, tint2 = 0;
};
static Textures g_extTextures[kPartCount];

static void rememberExtensionTextures(const std::vector<Asset>& assets) {
    for (const Asset& a : assets) {
        int style = 0, flag = 0;
        if (!a.groom || !extensionStyle(a.info.part, style, flag) || a.info.style != style || a.info.flag != flag)
            continue;
        g_extTextures[a.info.part] = {true, a.albedo, a.normal, a.material, a.palette, a.tint0, a.tint1, a.tint2};
    }
}

static void applyLength(Ped ped, const Asset& a, int length) {
    DrawableInfo want = resolveLength(a.info, length);
    Textures t{true, a.albedo, a.normal, a.material, a.palette, a.tint0, a.tint1, a.tint2};
    bool switchesStyle = want.style != a.info.style || want.flag != a.info.flag;
    if (switchesStyle && g_extTextures[a.info.part].known) {
        t = g_extTextures[a.info.part];
        t.palette = a.palette;   // keep the hair colour the player has now
        t.tint0 = a.tint0;
        t.tint1 = a.tint1;
        t.tint2 = a.tint2;
    }
    _SET_META_PED_TAG(ped, drawableHash(want), t.albedo, t.normal, t.material, t.palette, t.tint0, t.tint1, t.tint2);
    _UPDATE_PED_VARIATION(ped, false, true, true, true, false);
    logfile::write("%s: %s -> %s%s", partLabel(a.info.part), drawableName(a.info).c_str(), drawableName(want).c_str(),
                   switchesStyle ? " (long style)" : "");
}

enum class Growth { None, Grew, ReachedMax };

// Keeps one part at its target length and grows it when growNow is set or, in
// natural mode, when its in-game growth clock runs out.
static Growth updatePart(Ped ped, Part part, const Asset* a, bool growNow, bool cutNow, int elapsedMinutes,
                         bool& stateDirty) {
    const PartConfig& pc = g_cfg.parts[part];
    if (!pc.enabled || !a) return Growth::None;   // disabled, shaved off, or hidden under a hat

    PartState& ps = g_parts[part];
    int& target = targetOf(part);
    int& minutes = minutesOf(part);
    int current = a->info.length;
    DWORD now = GetTickCount();

    // Wait until our last swap shows up; give up on a length that never does.
    if (ps.pendingLength >= 0) {
        if (current == ps.pendingLength) {
            ps.pendingLength = -1;
            ps.failures = 0;
        } else if (static_cast<LONG>(ps.pendingUntil - now) > 0) {
            return Growth::None;
        } else {
            ps.pendingLength = -1;
            if (++ps.failures >= kMaxFailedApplies) {
                logfile::write("%s: length %d can't be set - staying at %d", partLabel(part), target, current);
                ps.blockedStyle = a->info.style;
                ps.blockedLength = target;
                target = current;
                stateDirty = true;
                ps.failures = 0;
                return Growth::None;
            }
        }
    }

    int minLen = minKnownLength(a->info);
    int maxLen = maxReachableLength(a->info);
    if (pc.maxLength > 0) maxLen = std::min(maxLen, pc.maxLength);
    if (ps.blockedStyle == a->info.style && ps.blockedLength > 0) maxLen = std::min(maxLen, ps.blockedLength - 1);
    if (minLen < 0 || maxLen < 0) return Growth::None;   // style we have no lengths for

    if (target < 0 || current > target) {
        // First sight, or the game grew it (natural growth, hair tonic): adopt
        // it and restart our own growth clock.
        if (target >= 0) minutes = 0;
        target = current;
        stateDirty = true;
    } else if (current < target) {
        // Shorter than it should be. A real haircut or shave only happens at the
        // barber or the camp mirror (game busy) and doesn't land exactly on the
        // game's own maximum while the hair is longer than that: that is the
        // game re-applying the length it tracks (outfit change, mounting a horse,
        // loading), which we undo like LockBarber does.
        int vanillaMax = vanillaMaxLength(a->info);
        bool haircut = ps.cutWhileBusy && !g_cfg.lockBarber &&
                       (vanillaMax < 0 || current < vanillaMax || target <= vanillaMax);
        if (haircut) {
            logfile::write("%s: haircut %d -> %d accepted", partLabel(part), target, current);
            target = current;
            minutes = 0;
            stateDirty = true;
        }
    }
    ps.cutWhileBusy = false;

    // At the longest known length: ask the game whether a longer model exists
    // (e.g. one added with LML). Found ones count as known from then on.
    Probe nextProbe = Probe::Missing;
    bool capAllows = pc.maxLength == 0 || target + 1 <= pc.maxLength;
    bool blocked = ps.blockedStyle == a->info.style && ps.blockedLength == target + 1;
    if (target >= maxLen && capAllows && !blocked && target + 1 <= kMaxProbeLength) {
        DrawableInfo next = a->info;
        next.length = target + 1;
        nextProbe = probeDrawable(drawableName(next), *a);
    }

    // Natural mode: count in-game time. When the game hasn't grown this part for
    // HoursPerStep hours (at its own maximum, or stuck without hair tonic), grow
    // it one step ourselves - at the game's pace, up to our longest length.
    bool clockGrow = false;
    if (!g_cfg.turbo) {
        int stepMinutes = std::max(1, static_cast<int>(g_cfg.hoursPerStep * 60.0f));
        if (target >= maxLen && nextProbe != Probe::Pending) {
            minutes = std::min(minutes + elapsedMinutes, stepMinutes);   // nothing left to grow into
        } else {
            minutes += elapsedMinutes;
            if (minutes >= stepMinutes) {
                clockGrow = true;
                minutes = std::min(minutes - stepMinutes, stepMinutes - 1);   // one step per tick
            }
        }
        if (clockGrow || std::abs(minutes - ps.savedMinutes) >= 60) stateDirty = true;
    }

    Growth result = Growth::None;
    bool wantGrow = growNow || clockGrow || ps.growthOwed;
    if (cutNow) {
        target = minLen;
        ps.maxAnnounced = false;
        ps.growthOwed = false;
        stateDirty = true;
    } else if (wantGrow) {
        if (target < maxLen) {
            ++target;
            result = Growth::Grew;
            ps.maxAnnounced = false;
            ps.growthOwed = false;
            stateDirty = true;
        } else if (nextProbe == Probe::Pending) {
            ps.growthOwed = true;   // grow as soon as the game answers
        } else {
            ps.growthOwed = false;
            if (!ps.maxAnnounced) {
                logfile::write("%s: longest available length %d reached", partLabel(part), maxLen);
                ps.maxAnnounced = true;
                result = Growth::ReachedMax;
            }
        }
    }
    target = std::min(target, std::max(maxLen, current));

    if (target != current) {
        DrawableInfo want = resolveLength(a->info, target);
        if (!drawableKnown(want)) {
            logfile::write("%s: %s is unknown - not set", partLabel(part), drawableName(want).c_str());
            target = current;
            stateDirty = true;
            return Growth::None;
        }
        applyLength(ped, *a, target);
        ps.pendingLength = target;
        ps.pendingUntil = now + kApplyWaitMs;
    }
    return result;
}

struct Input {
    bool dump = false;
    bool growNow = false;
    bool testCut = false;
};

static void tick(DWORD& nextGrowAt, const Input& in) {
    pollProbes();

    Ped ped = 0;
    if (!playerUsable(ped)) return;

    auto assets = readAssets(ped);
    rememberExtensionTextures(assets);

    // One automatic dump shortly after loading (once Arthur is fully built),
    // so every log shows what he wears.
    static DWORD usableSince = 0;
    static bool dumpedOnce = false;
    if (!usableSince) usableSince = GetTickCount();
    bool autoDump = !dumpedOnce && GetTickCount() - usableSince > 10000;
    if (in.dump || autoDump) {
        dumpAssets(ped, assets, in.dump);
        dumpedOnce = true;
    }

    if (g_cfg.recordMode) recordChanges(assets);
    g_minutesBank += gameMinutesElapsed();
    if (g_paused) {
        g_minutesBank = 0;   // paused means paused: no catching up afterwards
        return;
    }

    // Fighting the barber's own preview every tick made the hair flicker. We
    // only note whether something got shorter meanwhile (a possible haircut).
    DWORD now = GetTickCount();
    bool busy = gameIsBusy(ped);
    for (int p = 0; p < kPartCount; ++p) {
        PartState& ps = g_parts[p];
        const Asset* a = findPart(assets, static_cast<Part>(p));
        int seen = a ? a->info.length : -1;
        if (busy && seen >= 0 && ps.lastSeen >= 0 && seen < ps.lastSeen) ps.cutWhileBusy = true;
        if (seen >= 0) ps.lastSeen = seen;
    }
    if (busy) {
        if (g_cfg.turbo && static_cast<LONG>(now - nextGrowAt) >= 0) nextGrowAt = now + kSettleMs;
        for (PartState& ps : g_parts) ps.pendingLength = -1;   // the game took over; not our failure
        return;
    }

    bool growNow = in.growNow;
    if (g_cfg.turbo && static_cast<LONG>(now - nextGrowAt) >= 0) {
        growNow = true;
        nextGrowAt = now + static_cast<DWORD>(g_cfg.growIntervalSeconds * 1000.0f);
    }
    int elapsed = g_minutesBank;
    g_minutesBank = 0;

    bool dirty = false;
    std::string grown, maxed;
    for (int p = 0; p < kPartCount; ++p) {
        Part part = static_cast<Part>(p);
        int count = 0;
        const Asset* asset = findPart(assets, part, &count);
        if (count > 1) {
            // _SET_META_PED_TAG added a layer instead of replacing one: stop touching this part.
            if (!g_parts[p].duplicateWarned) {
                logfile::write("[warn] %s: %d drawables at once - leaving this part alone", partLabel(part), count);
                g_parts[p].duplicateWarned = true;
            }
            continue;
        }
        Growth growth = updatePart(ped, part, asset, growNow, in.testCut, elapsed, dirty);
        if (growth == Growth::None) continue;
        std::string& list = growth == Growth::Grew ? grown : maxed;
        if (!list.empty()) list += ", ";
        list += std::string(partLabel(part)) + " " + std::to_string(targetOf(part));
    }
    if (in.testCut) notify("test: everything cut to the shortest length");
    if (!grown.empty()) notify("growing: " + grown);
    else if (!maxed.empty()) notify("longest length reached: " + maxed);
    if (dirty) {
        saveState(g_dir, g_state);
        for (int p = 0; p < kPartCount; ++p) g_parts[p].savedMinutes = minutesOf(static_cast<Part>(p));
    }
}

void ScriptMain() {
    g_dir = moduleDirectory();
    logfile::open(g_dir + "\\InfiniteHair.log");
    g_cfg = loadConfig(g_dir);
    g_state = loadState(g_dir);

    if (g_cfg.turbo)
        logfile::write("Infinite Hair %s by shrouddeath loaded. mode: turbo, %.0f s per step, lockBarber: %d", kVersion,
                       g_cfg.growIntervalSeconds, g_cfg.lockBarber ? 1 : 0);
    else
        logfile::write("Infinite Hair %s by shrouddeath loaded. mode: natural, %.1f in-game hours per step, "
                       "lockBarber: %d", kVersion, g_cfg.hoursPerStep, g_cfg.lockBarber ? 1 : 0);
    for (int p = 0; p < kPartCount; ++p) g_parts[p].savedMinutes = minutesOf(static_cast<Part>(p));
    for (const std::string& name : g_cfg.extraDrawables) {
        if (addExtraDrawable(name))
            logfile::write("  extra drawable: %s", name.c_str());
        else
            logfile::write("  extra drawable IGNORED (name doesn't match the scheme): %s", name.c_str());
    }
    for (const auto& [virtualName, slotName] : g_cfg.chains) {
        if (addChain(virtualName, slotName))
            logfile::write("  length %s uses slot %s", virtualName.c_str(), slotName.c_str());
        else
            logfile::write("  [Chains] IGNORED (name doesn't match the scheme): %s=%s", virtualName.c_str(),
                           slotName.c_str());
    }

    if (!g_cfg.enabled) {
        logfile::write("Disabled in InfiniteHair.ini.");
        return;
    }

    DWORD nextGrowAt = GetTickCount() + static_cast<DWORD>(g_cfg.growIntervalSeconds * 1000.0f);
    DWORD nextTickAt = 0;
    Input pending;
    for (;;) {
        // Keys are polled every frame so short taps aren't missed between ticks.
        if (keyPressed(g_cfg.toggleKey)) {
            g_paused = !g_paused;
            notify(g_paused ? "paused" : "active");
        }
        pending.dump |= keyPressed(g_cfg.dumpKey);
        pending.growNow |= keyPressed(g_cfg.growNowKey);
        pending.testCut |= keyPressed(g_cfg.testCutKey);

        DWORD now = GetTickCount();
        if (static_cast<LONG>(now - nextTickAt) >= 0) {
            tick(nextGrowAt, pending);
            pending = Input{};
            nextTickAt = now + 250;
        }
        if (!g_message.empty() && GetTickCount() <= g_messageUntil) drawText(g_message, 0.92f);
        scriptWait(0);
    }
}
