#include "groom.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <set>

#include "config.h"

namespace {

const char* kBeardPart[kPartCount] = {"", "chin", "stache", "chops"};

std::map<Hash, DrawableInfo>& table() {
    static std::map<Hash, DrawableInfo> names;
    if (names.empty()) {
        DrawableInfo info;
        info.part = kHair;
        for (info.style = 1; info.style <= 20; ++info.style)
            for (info.length = 1; info.length <= kMaxProbeLength; ++info.length)
                for (info.flag = 0; info.flag <= 1; ++info.flag) names[joaat(drawableName(info))] = info;

        for (int part = kBeardChin; part < kPartCount; ++part) {
            info.part = static_cast<Part>(part);
            for (info.style = 0; info.style <= 9; ++info.style)
                for (info.length = 1; info.length <= kMaxProbeLength; ++info.length)
                    for (info.flag = 0; info.flag <= 1; ++info.flag) names[joaat(drawableName(info))] = info;
        }
    }
    return names;
}

std::set<Hash>& extras() {
    static std::set<Hash> hashes;
    return hashes;
}

std::map<Hash, Hash>& slotOfVirtual() {
    static std::map<Hash, Hash> slots;
    return slots;
}

std::map<Hash, DrawableInfo>& virtualOfSlot() {
    static std::map<Hash, DrawableInfo> virtuals;
    return virtuals;
}

// Part -> chain info with the highest length (its style/flag is the extension style).
std::map<int, DrawableInfo>& longestChain() {
    static std::map<int, DrawableInfo> chains;
    return chains;
}

// Ranges documented in femga/rdr3_discoveries (cloth_drawable_..._TEMPORARY.lua).
bool inVanillaRange(const DrawableInfo& d) {
    if (d.part == kHair) {
        if (d.style < 1 || d.style > 8 || d.flag < 0 || d.flag > 1) return false;
        int minLength = d.style == 1 ? 1 : d.style == 2 ? 2 : 3;
        return d.length >= minLength && d.length <= 9;
    }
    if (d.flag != 0) return false;   // only the "shv" beard set is documented
    int maxStyle = d.part == kBeardMustache ? 2 : 1;
    int minLength = (d.part == kBeardMustache && d.style == 2) ? 4 : 2;
    return d.style >= 0 && d.style <= maxStyle && d.length >= minLength && d.length <= 10;
}

} // namespace

const char* partKey(Part part) {
    static const char* keys[kPartCount] = {"Hair", "BeardChin", "BeardMustache", "BeardChops"};
    return keys[part];
}

const char* partLabel(Part part) {
    static const char* labels[kPartCount] = {"Hair", "Chin beard", "Mustache", "Sideburns"};
    return labels[part];
}

std::string drawableName(const DrawableInfo& d) {
    char buf[64];
    if (d.part == kHair)
        snprintf(buf, sizeof(buf), "player_zero_hair_s%02d_l%02d_p%d", d.style, d.length, d.flag);
    else
        snprintf(buf, sizeof(buf), "player_zero_beard_%s%s_%03d_%02d", d.flag == 0 ? "shv_" : "", kBeardPart[d.part],
                 d.style, d.length);
    return buf;
}

bool parseDrawable(Hash drawable, DrawableInfo& out) {
    auto borrowed = virtualOfSlot().find(drawable);
    if (borrowed != virtualOfSlot().end()) {
        out = borrowed->second;
        return true;
    }
    auto it = table().find(drawable);
    if (it == table().end()) return false;
    out = it->second;
    return true;
}

bool addChain(const std::string& virtualName, const std::string& slotName) {
    Hash virtualHash = joaat(virtualName), slotHash = joaat(slotName);
    auto v = table().find(virtualHash);
    // The slot may be any existing drawable, e.g. John's beard
    // (player_three_ms1_beard_...) once Arthur's own free slots run out.
    if (v == table().end() || slotName.empty() || slotHash == virtualHash) return false;
    slotOfVirtual()[virtualHash] = slotHash;
    virtualOfSlot()[slotHash] = v->second;
    extras().insert(virtualHash);
    auto longest = longestChain().find(v->second.part);
    if (longest == longestChain().end() || v->second.length > longest->second.length)
        longestChain()[v->second.part] = v->second;
    return true;
}

bool extensionStyle(Part part, int& style, int& flag) {
    auto it = longestChain().find(part);
    if (it == longestChain().end()) return false;
    style = it->second.style;
    flag = it->second.flag;
    return true;
}

DrawableInfo resolveLength(const DrawableInfo& worn, int length) {
    DrawableInfo own = worn;
    own.length = length;
    int style = 0, flag = 0;
    if (drawableKnown(own) || !extensionStyle(worn.part, style, flag)) return own;
    if (length <= maxKnownLength(worn)) return own;   // a gap in the worn style, not past its end
    DrawableInfo ext = own;
    ext.style = style;
    ext.flag = flag;
    return drawableKnown(ext) ? ext : own;
}

int maxReachableLength(const DrawableInfo& worn) {
    int own = maxKnownLength(worn);
    int style = 0, flag = 0;
    if (own < 0 || !extensionStyle(worn.part, style, flag)) return own;
    DrawableInfo ext = worn;
    ext.style = style;
    ext.flag = flag;
    ext.length = own + 1;
    // Only if the extension continues right where the worn style ends.
    return drawableKnown(ext) ? std::max(own, maxKnownLength(ext)) : own;
}

Hash drawableHash(const DrawableInfo& info) {
    Hash own = joaat(drawableName(info));
    auto slot = slotOfVirtual().find(own);
    return slot == slotOfVirtual().end() ? own : slot->second;
}

bool drawableKnown(const DrawableInfo& info) {
    if (inVanillaRange(info)) return true;
    return !extras().empty() && extras().count(joaat(drawableName(info))) > 0;
}

bool addExtraDrawable(const std::string& name) {
    Hash h = joaat(name);
    DrawableInfo info;
    if (!parseDrawable(h, info)) return false;
    extras().insert(h);
    return true;
}

int vanillaMaxLength(const DrawableInfo& info) {
    DrawableInfo d = info;
    for (d.length = kMaxProbeLength; d.length >= 1; --d.length)
        if (inVanillaRange(d)) return d.length;
    return -1;
}

int minKnownLength(const DrawableInfo& info) {
    DrawableInfo d = info;
    for (d.length = 1; d.length <= kMaxProbeLength; ++d.length)
        if (drawableKnown(d)) return d.length;
    return -1;
}

int maxKnownLength(const DrawableInfo& info) {
    DrawableInfo d = info;
    for (d.length = kMaxProbeLength; d.length >= 1; --d.length)
        if (drawableKnown(d)) return d.length;
    return -1;
}
