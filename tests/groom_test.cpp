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

    // 8) the game's own maximum per style (natural growth hands over there)
    DrawableInfo v9; v9.part = kHair; v9.style = 4; v9.length = 5; v9.flag = 0;
    check(vanillaMaxLength(v9) == 9, "hair s04 vanilla max 9 (chains don't count)");
    DrawableInfo vb; vb.part = kBeardChin; vb.style = 0; vb.length = 5; vb.flag = 0;
    check(vanillaMaxLength(vb) == 10, "chin beard vanilla max 10");
    DrawableInfo vx; vx.part = kHair; vx.style = 12; vx.length = 5; vx.flag = 0;
    check(vanillaMaxLength(vx) == -1, "unknown style has no vanilla max");

    // 9) other styles grow on into the extension style (barber cut -> s01)
    int es = -1, ef = -1;
    check(extensionStyle(kHair, es, ef) && es == 4 && ef == 0, "hair extension style s04 p0");
    DrawableInfo s01; s01.part = kHair; s01.style = 1; s01.length = 9; s01.flag = 0;
    check(maxReachableLength(s01) == 10, "s01 reaches the s04 chain");
    DrawableInfo r = resolveLength(s01, 10);
    check(r.style == 4 && r.flag == 0 && r.length == 10, "s01 length 10 -> s04 l10");
    check(drawableHash(r) == joaat("player_zero_hair_s08_l03_p1"), "s01 length 10 applies the s04 slot");
    r = resolveLength(s01, 5);
    check(r.style == 1 && r.length == 5, "s01 keeps its own lengths");
    DrawableInfo s02p1; s02p1.part = kHair; s02p1.style = 2; s02p1.length = 9; s02p1.flag = 1;
    check(maxReachableLength(s02p1) == 10, "pomade style reaches the chain too");
    DrawableInfo stache; stache.part = kBeardMustache; stache.style = 0; stache.length = 5; stache.flag = 0;
    check(maxReachableLength(stache) == 10, "no mustache chains: own max");
    DrawableInfo chin1; chin1.part = kBeardChin; chin1.style = 1; chin1.length = 10; chin1.flag = 0;
    check(maxReachableLength(chin1) == 18 && resolveLength(chin1, 11).style == 0, "chin style 001 grows into 000");

    printf(fails ? "FAILED: %d\n" : "ALL OK\n", fails);
    return fails ? 1 : 0;
}
