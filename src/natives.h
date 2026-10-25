#pragma once
// Minimal native wrappers, called by hash through the ScriptHookRDR2 SDK
// (nativeInit / nativePush64 / nativeCall from main.h). Hashes and signatures
// come from the RDR3 NativeDB: https://alloc8or.re/rdr3/nativedb/
//
// We deliberately don't depend on the SDK's generated natives.h, because its
// names for undocumented (_UNDERSCORE) natives change between SDK versions.

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "main.h"

using Ped = int;
using Player = int;
using Hash = uint32_t;

namespace native {

template <typename T>
inline void push(T value) {
    static_assert(sizeof(T) <= sizeof(uint64_t), "native argument too large");
    uint64_t raw = 0;
    std::memcpy(&raw, &value, sizeof(T));
    nativePush64(raw);
}

template <typename R, typename... Args>
inline R invoke(uint64_t hash, Args... args) {
    nativeInit(hash);
    (push(args), ...);
    uint64_t* result = reinterpret_cast<uint64_t*>(nativeCall());
    if constexpr (std::is_void_v<R>) {
        return;
    } else {
        R value{};
        std::memcpy(&value, result, sizeof(R));
        return value;
    }
}

} // namespace native

// ---- PLAYER / ENTITY / MISC ------------------------------------------------
inline Player PLAYER_ID() { return native::invoke<Player>(0x217E9DC48139933D); }
inline Ped PLAYER_PED_ID() { return native::invoke<Ped>(0x096275889B8E0EE0); }
inline bool IS_PLAYER_PLAYING(Player p) { return native::invoke<int>(0xBFFB35986CAAE58C, p) != 0; }
inline bool DOES_ENTITY_EXIST(int e) { return native::invoke<int>(0xD42BD6EB2E0F1677, e) != 0; }
inline bool IS_ENTITY_DEAD(int e) { return native::invoke<int>(0x7D5B1F88E7504BBA, e) != 0; }
inline Hash GET_ENTITY_MODEL(int e) { return native::invoke<Hash>(0xDA76A9F39210D365, e); }
inline bool IS_LOADING_SCREEN_VISIBLE() { return native::invoke<int>(0xB54ADBE65D528FCB) != 0; }
inline bool IS_SCREEN_FADED_OUT() { return native::invoke<int>(0xF5472C80DF2FF847) != 0; }
inline bool NETWORK_IS_SESSION_STARTED() { return native::invoke<int>(0x9DE624D2FC4B603F) != 0; }
inline bool IS_PLAYER_CONTROL_ON(Player p) { return native::invoke<int>(0x7964097FCE4C244B, p) != 0; }
inline bool IS_PED_USING_ANY_SCENARIO(Ped ped) { return native::invoke<int>(0x57AB4A3080F85143, ped) != 0; }
inline bool IS_PAUSE_MENU_ACTIVE() { return native::invoke<int>(0x535384D6067BA42E) != 0; }
inline bool IS_ANY_UIAPP_ACTIVE() { return native::invoke<int>(0xAC959AB99AAF3D9F) != 0; }

// ---- PED: MetaPed shop items (hair, beard, clothing all live here) ----------
inline int _GET_META_PED_TYPE(Ped ped) { return native::invoke<int>(0xEC9A1261BF0CE510, ped); }
inline int _GET_NUM_COMPONENTS_IN_PED(Ped ped) { return native::invoke<int>(0x90403E8107B60E81, ped); }

// resolveSelection = false: use the cached entry (the singleplayer path).
inline Hash _GET_SHOP_ITEM_COMPONENT_AT_INDEX(Ped ped, int index, Hash* outWearableState = nullptr) {
    alignas(8) uint64_t statusFlag = 0;
    alignas(8) uint64_t wearableState = 0;
    Hash comp = native::invoke<Hash>(0x77BA37622E22023B, ped, index, 0, &statusFlag, &wearableState);
    if (outWearableState) *outWearableState = static_cast<Hash>(wearableState);
    return comp;
}

// Wearable states: per-component variants (bandana up/down, sleeves, ...).
// Suspected to hold the trim length of Arthur's beard parts.
inline int _GET_SHOP_ITEM_NUM_WEARABLE_STATES(Hash component) {
    return native::invoke<int>(0xFFCC2DB2D9953401, component, 0, 1);
}
inline Hash _GET_SHOP_ITEM_WEARABLE_STATE_BY_INDEX(Hash component, int index) {
    return native::invoke<Hash>(0x6243635AF2F1B826, component, index, 0, 1);
}
inline void _UPDATE_SHOP_ITEM_WEARABLE_STATE(Ped ped, Hash component, Hash wearableState) {
    native::invoke<void>(0x66B957AAC2EAAEAB, ped, component, wearableState, 0, 0, 1);
}

inline Hash _GET_SHOP_ITEM_COMPONENT_CATEGORY(Hash component, int metaPedType, bool isMp) {
    return native::invoke<Hash>(0x5FF9A878C3D115B8, component, metaPedType, isMp ? 1 : 0);
}

inline bool _IS_META_PED_USING_COMPONENT(Ped ped, Hash component) {
    return native::invoke<int>(0xFB4891BD7578CDC1, ped, component) != 0;
}

inline void _APPLY_SHOP_ITEM_TO_PED(Ped ped, Hash component, bool immediately, bool isMp, bool p4) {
    native::invoke<void>(0xD3A7B003ED343FD9, ped, component, immediately ? 1 : 0, isMp ? 1 : 0, p4 ? 1 : 0);
}

// ---- PED: MetaPed assets (the drawables actually shown, e.g. hair length) ----
inline bool GET_META_PED_ASSET_GUIDS(Ped ped, int index, Hash& drawable, Hash& albedo, Hash& normal, Hash& material) {
    alignas(8) uint64_t d = 0, a = 0, n = 0, m = 0;
    bool ok = native::invoke<int>(0xA9C28516A6DC9D56, ped, index, &d, &a, &n, &m) != 0;
    drawable = static_cast<Hash>(d);
    albedo = static_cast<Hash>(a);
    normal = static_cast<Hash>(n);
    material = static_cast<Hash>(m);
    return ok;
}

inline bool GET_META_PED_ASSET_TINT(Ped ped, int index, Hash& palette, int& tint0, int& tint1, int& tint2) {
    alignas(8) uint64_t p = 0, t0 = 0, t1 = 0, t2 = 0;
    bool ok = native::invoke<int>(0xE7998FEC53A33BBE, ped, index, &p, &t0, &t1, &t2) != 0;
    palette = static_cast<Hash>(p);
    tint0 = static_cast<int>(t0);
    tint1 = static_cast<int>(t1);
    tint2 = static_cast<int>(t2);
    return ok;
}

// Replaces the asset in the drawable's slot (e.g. swaps one hair length for another).
inline void _SET_META_PED_TAG(Ped ped, Hash drawable, Hash albedo, Hash normal, Hash material, Hash palette,
                              int tint0, int tint1, int tint2) {
    native::invoke<void>(0xBC6DF00D7A4A6819, ped, drawable, albedo, normal, material, palette, tint0, tint1, tint2);
}

// Streams a drawable/texture set (used by R* for dropped hats). We use it to
// check whether a drawable exists before showing it.
inline int REQUEST_META_PED_ASSET_SET(Hash drawable, Hash albedo, Hash normal, Hash material) {
    return native::invoke<int>(0x3FCBB5FCFD968698, drawable, albedo, normal, material, 0);
}
inline bool _HAS_META_PED_ASSET_LOADED(int requestId) { return native::invoke<int>(0xB0B2C6D170B0E8E5, requestId) != 0; }
inline bool _IS_META_PED_ASSET_VALID(int requestId) { return native::invoke<int>(0x93FFD92F05EC32FD, requestId) != 0; }
inline void _RELEASE_META_PED_ASSET_REQUEST(int requestId) { native::invoke<void>(0x13E7320C762F0477, requestId); }

inline void _UPDATE_PED_VARIATION(Ped ped, bool p1, bool p2, bool p3, bool p4, bool p5) {
    native::invoke<void>(0xCC8CA3E88256E58F, ped, p1 ? 1 : 0, p2 ? 1 : 0, p3 ? 1 : 0, p4 ? 1 : 0, p5 ? 1 : 0);
}

// ---- HUD ---------------------------------------------------------------------
// VAR_STRING(10, "LITERAL_STRING", text) wraps a plain C string for UI natives.
inline const char* VAR_STRING_LITERAL(const char* text) {
    return native::invoke<const char*>(0xFA925AC00EB830B9, 10, "LITERAL_STRING", text);
}
inline void _SET_TEXT_COLOR(int r, int g, int b, int a) { native::invoke<void>(0x50A41AD966910F03, r, g, b, a); }
// Must be called every frame for the text to stay visible.
inline void _DISPLAY_TEXT(const char* text, float x, float y) { native::invoke<void>(0xD79334A4BB99BAD1, text, x, y); }
