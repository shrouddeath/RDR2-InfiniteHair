# Tests

Two offline tests, no game needed:

- `test.bat` builds and runs `groom_test.cpp`: drawable naming, length ranges and
  borrowed slots (`[Chains]`) used by the mod.
- `python tools/test_grow_models.py`: the model generator on synthetic CodeX XML
  (stretching, slots, cloth files, back collision, drift direction).

`groom_test.cpp` can also check every name in femga's drawable list. That list is
not in this repository (femga/rdr3_discoveries has no license). To use it, create
`tests/femga_player_zero_groom.txt` with one `0xHASH name` line per
`player_zero_hair_*` / `player_zero_beard_*` entry of
`clothes/cloth_drawable_albedo_normal_material_TEMPORARY.lua` from
https://github.com/femga/rdr3_discoveries. Without the file that part is skipped.
