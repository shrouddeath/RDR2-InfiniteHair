#pragma once
// Arthur's grooming drawables.
//
// A shop item only picks the *style* (CLOTHING_ITEM_HAIR_SWEPT_BACK,
// CLOTHING_ITEM_BEARD_SINGLE, ...). The *length* is a separate drawable per
// style, named like (names from femga/rdr3_discoveries):
//   player_zero_hair_s02_l07_p0        hair style 02, length 07, pomade off
//   player_zero_beard_shv_chin_000_10  chin beard style 000, length 10
// Lengths beyond the vanilla maximum (step 2 of the project) can be registered
// as extra drawables once they exist as files.

#include <cstdint>
#include <string>

using Hash = uint32_t;

enum Part { kHair, kBeardChin, kBeardMustache, kBeardChops, kPartCount };

struct DrawableInfo {
    Part part = kHair;
    int style = 0;    // hair: sNN, beard: NNN
    int length = 0;   // hair: lNN, beard: _NN
    int flag = 0;     // hair: pN (pomade); beard: 0 = "shv" naming, 1 = plain naming
};

const char* partKey(Part part);     // ini section / state key, e.g. "BeardChin"
const char* partLabel(Part part);   // shown on screen, e.g. "Kinnbart"

std::string drawableName(const DrawableInfo& info);
bool parseDrawable(Hash drawable, DrawableInfo& out);

// True if the drawable is known to exist: documented vanilla range or extra list.
bool drawableKnown(const DrawableInfo& info);
// Registers a custom drawable (e.g. a longer model added with LML). Returns
// false if the name doesn't follow the naming scheme above.
bool addExtraDrawable(const std::string& name);

// Lengths past the vanilla maximum can't use new names: the metaped system
// only knows drawables from its own archives (game test 25.10.2026). They
// borrow existing drawables of styles without cloth files (s05-s08) instead,
// replaced via LML, e.g. player_zero_hair_s04_l10_p0 -> player_zero_hair_s08_l03_p1.
// The slot can be any drawable name (Arthur's or John's); the virtual name must
// follow the naming scheme, otherwise this returns false.
bool addChain(const std::string& virtualName, const std::string& slotName);

// Drawable to put on the ped for `info`: its borrowed slot, or its own name.
Hash drawableHash(const DrawableInfo& info);

// Shortest / longest known length for the style of `info`, or -1 if none.
int minKnownLength(const DrawableInfo& info);
int maxKnownLength(const DrawableInfo& info);

// Highest length we look at when parsing and probing (names use two digits).
const int kMaxProbeLength = 99;
