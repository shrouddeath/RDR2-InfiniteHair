// Offline check of groom.cpp against the femga drawable list.
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include "config.h"
#include "groom.h"

int main(int, char** argv) {
    int fails = 0;
    auto check = [&](bool ok, const std::string& what) { if (!ok) { ++fails; printf("FAIL %s\n", what.c_str()); } };

    // 1) round trip
    DrawableInfo d; d.part = kHair; d.style = 7; d.length = 9; d.flag = 1;
    check(drawableName(d) == "player_zero_hair_s07_l09_p1", "name hair");
    DrawableInfo back;
    check(parseDrawable(joaat("player_zero_hair_s07_l09_p1"), back) && back.style == 7 && back.length == 9 && back.flag == 1, "parse hair");
    check(parseDrawable(joaat("PLAYER_ZERO_BEARD_SHV_CHIN_000_10"), back) && back.part == kBeardChin && back.length == 10 && back.flag == 0, "parse beard upper");
    check(!parseDrawable(joaat("player_zero_hair_bald_000"), back), "bald not a length");

    // 2) every femga name must parse and count as known (optional, see tests/README.md)
    std::ifstream in(argv[1] ? argv[1] : "");
    if (!in) printf("femga list not found - skipping that part (see tests/README.md)\n");
    std::string line, hexs, name; int n = 0, unknown = 0;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        if (!(ls >> hexs >> name)) continue;
        if (hexs.rfind("0x", 0) != 0) continue;
        unsigned long h = std::stoul(hexs.substr(2), nullptr, 16);
        if (h != joaat(name)) { check(false, "hash " + name); continue; }
        DrawableInfo info;
        if (!parseDrawable(h, info)) { if (name.find("_c0_") == std::string::npos && name.find("fauxbeard") == std::string::npos && name != "player_zero_hair_bald_000" && name != "player_zero_beard_shv_merge") check(false, "parse " + name); continue; }
        ++n;
        if (drawableName(info) != name) check(false, "roundtrip " + name);
        if (!drawableKnown(info)) { ++unknown; printf("not in vanilla range: %s\n", name.c_str()); }
    }
    printf("femga lengths parsed: %d, outside vanilla range: %d\n", n, unknown);

    // 3) ranges
    d.part = kHair; d.style = 2; d.flag = 0;
    printf("hair s02: %d..%d\n", minKnownLength(d), maxKnownLength(d));
    d.style = 9; printf("hair s09: %d..%d\n", minKnownLength(d), maxKnownLength(d));
    d.part = kBeardMustache; d.style = 2; d.flag = 0;
    printf("stache 002: %d..%d\n", minKnownLength(d), maxKnownLength(d));

    // 4) extra drawables extend the range
    check(addExtraDrawable("player_zero_hair_s07_l10_p1"), "extra add");
    check(!addExtraDrawable("my_custom_hair"), "extra reject");
    d.part = kHair; d.style = 7; d.flag = 1;
    printf("hair s07 p1 with extra: %d..%d\n", minKnownLength(d), maxKnownLength(d));

    // 5) borrowed slots: s04 length 10 lives on s08_l03_p1
    check(addChain("player_zero_hair_s04_l10_p0", "player_zero_hair_s08_l03_p1"), "chain add");
    check(!addChain("my_hair_11", "player_zero_hair_s08_l04_p1"), "chain rejects bad virtual name");
    check(!addChain("player_zero_hair_s04_l11_p0", ""), "chain rejects empty slot");
    DrawableInfo v;
    check(parseDrawable(joaat("player_zero_hair_s08_l03_p1"), v) && v.style == 4 && v.length == 10 && v.flag == 0,
          "slot reads as virtual length");
    v.part = kHair; v.style = 4; v.length = 10; v.flag = 0;
    check(drawableHash(v) == joaat("player_zero_hair_s08_l03_p1"), "virtual length applies slot");
    v.length = 9;
    check(drawableHash(v) == joaat("player_zero_hair_s04_l09_p0"), "vanilla length unchanged");
    printf("hair s04 p0 with chain: %d..%d\n", minKnownLength(v), maxKnownLength(v));
    check(maxKnownLength(v) == 10, "chain extends range");

    // 6) beard chains: chin 000 length 11 lives on chin 001 length 02
    check(addChain("player_zero_beard_shv_chin_000_11", "player_zero_beard_shv_chin_001_02"), "beard chain add");
    DrawableInfo b;
    check(parseDrawable(joaat("player_zero_beard_shv_chin_001_02"), b) && b.part == kBeardChin && b.style == 0 &&
              b.length == 11,
          "beard slot reads as chin 000 length 11");
    b.part = kBeardChin; b.style = 0; b.length = 11; b.flag = 0;
    check(drawableHash(b) == joaat("player_zero_beard_shv_chin_001_02"), "beard length 11 applies slot");
    check(maxKnownLength(b) == 11, "beard chain extends range");

    // 7) John's beard as a slot once Arthur's run out
    check(addChain("player_zero_beard_shv_chin_000_18", "player_three_ms1_beard_shv_chin_000_02"), "john slot add");
    check(parseDrawable(joaat("player_three_ms1_beard_shv_chin_000_02"), b) && b.part == kBeardChin &&
              b.length == 18,
          "john slot reads as chin 000 length 18");
    b.part = kBeardChin; b.style = 0; b.length = 18; b.flag = 0;
    check(drawableHash(b) == joaat("player_three_ms1_beard_shv_chin_000_02"), "length 18 applies john slot");

    printf(fails ? "FAILED: %d\n" : "ALL OK\n", fails);
    return fails ? 1 : 0;
}
