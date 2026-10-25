"""Offline test for grow_models.py with synthetic CodeX-style RDR2 XML files."""

import shutil
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import grow_models as gm  # noqa: E402

FAILS = []


def check(ok, what):
    if not ok:
        FAILS.append(what)
        print("FAIL", what)


def vertex_row(semantics, pos, i):
    attrs = []
    for letter in semantics:
        if letter == "P":
            attrs.append(" ".join(f"{v:.7f}" for v in pos))
        elif letter == "N":
            attrs.append("0.0000000 -1.0000000 0.0000000 1.0000000")
        elif letter == "W":
            attrs.append("255 0 0 0")
        elif letter == "I":
            attrs.append(f"{i % 4} 0 0 0")
        elif letter == "T":
            attrs.append(f"0.{i:07d} 0.5000000")
    return "   ".join(attrs)


def make_xml(path, name, lods, semantics="PNWIT", hash_text=None):
    """lods: {"LodHigh": [[x, y, z], ...], ...}. Like CodeX, the drawable box
    sits 1 m above the vertex data and the Name carries a '.#dd' suffix."""
    hash_text = hash_text if hash_text is not None else f"0x{gm.joaat(name):08X}"
    out = ['<?xml version="1.0" encoding="UTF-8"?>', '<RDR2DrawableDictionary version="1">', "  <Drawables>",
           '    <Item version="1">', f"      <Name>{name}.#dd</Name>", f"      <Hash>{hash_text}</Hash>",
           '      <BoundingSphereCenter x="0" y="0" z="2.6" />', '      <BoundingSphereRadius value="0.25" />',
           '      <BoundingBoxMin x="-0.1" y="-0.1" z="2.4" />', '      <BoundingBoxMax x="0.1" y="0.1" z="2.8" />']
    for lod, points in lods.items():
        rows = "\n".join("                  " + vertex_row(semantics, p, i) for i, p in enumerate(points))
        out += [f"      <{lod}>", "        <Models>", "          <Item>", '            <Flags value="0" />',
                '            <BoundingBoxMin x="-0.1" y="-0.1" z="1.4" />',
                '            <BoundingBoxMax x="0.1" y="0.1" z="1.8" />', "            <Geometries>", "              <Item>",
                '                <BoundingBoxMin x="-0.1" y="-0.1" z="1.4" />',
                '                <BoundingBoxMax x="0.1" y="0.1" z="1.8" />', '                <ShaderID value="0" />',
                "                <VertexLayout>", '                  <NonInterleaved value="0" />',
                f"                  <Semantics>{semantics}</Semantics>", "                </VertexLayout>",
                "                <Vertices>", rows, "                </Vertices>",
                "                <Indices>0 1 2</Indices>", "              </Item>", "            </Geometries>",
                "          </Item>", "        </Models>", f"      </{lod}>"]
    out += ["    </Item>", "  </Drawables>", "</RDR2DrawableDictionary>", ""]
    path.write_text("\n".join(out), encoding="utf-8")


def rows_of(path, lod):
    root = ET.parse(path).getroot()
    text = root.find(f"./Drawables/Item/{lod}/Models/Item/Geometries/Item/Vertices").text
    return [line.split() for line in text.strip().splitlines()]


def positions_of(path, lod, offset):
    return [[float(v) for v in row[offset:offset + 3]] for row in rows_of(path, lod)]


HIGH = [[0.0, 0.0, 1.80], [0.05, 0.0, 1.75], [0.0, 0.08, 1.60], [0.0, 0.1, 1.45], [0.02, 0.1, 1.40]]
MED = [[0.0, 0.0, 1.80], [0.0, 0.08, 1.60], [0.02, 0.1, 1.40]]


def test_stretch(tmp):
    base = tmp / "player_zero_hair_s04_l09_p0.ydd.xml"
    # CodeX writes the hash as the plain name
    make_xml(base, "player_zero_hair_s04_l09_p0", {"LodHigh": HIGH, "LodMed": MED},
             hash_text="player_zero_hair_s04_l09_p0")
    out = tmp / "out_stretch"
    check(gm.main([str(base), "--count", "3", "--out", str(out)]) == 0, "stretch exit code")
    names = sorted(p.name for p in out.iterdir())
    check(names == [f"player_zero_hair_s04_l{n}_p0.ydd.xml" for n in (10, 11, 12)], f"stretch names {names}")

    anchor = 1.80 - (1.80 - 1.40) * 0.35
    ox, oy = gm.outward_direction(HIGH)
    check(oy > 0.9, f"outward points where the tips hang ({ox:.2f}, {oy:.2f})")
    lowest = []
    for n, step in ((10, 1), (11, 2), (12, 3)):
        new_name = f"player_zero_hair_s04_l{n}_p0"
        f = out / f"{new_name}.ydd.xml"
        root = ET.parse(f).getroot()
        item = root.find("./Drawables/Item")
        check(item.findtext("Name") == f"{new_name}.#dd", f"inner name keeps suffix l{n}: {item.findtext('Name')}")
        check(item.findtext("Hash") == new_name, f"plain-name hash l{n}: {item.findtext('Hash')}")
        for lod, src in (("LodHigh", HIGH), ("LodMed", MED)):
            got = positions_of(f, lod, 0)
            check(len(got) == len(src), f"vertex count {lod} l{n}")
            for (x, y, z), (gx, gy, gz) in zip(src, got):
                if z < anchor:
                    wz = anchor - (anchor - z) * (1 + 0.15 * step)
                    drop = z - wz
                    want = [x + ox * 0.25 * drop, y + oy * 0.25 * drop, wz]
                else:
                    want = [x, y, z]
                check(all(abs(a - b) < 1e-6 for a, b in zip([gx, gy, gz], want)), f"pos {lod} l{n} z={z}")
            base_rows, new_rows = rows_of(base, lod), rows_of(f, lod)
            check(all(a[3:] == b[3:] for a, b in zip(base_rows, new_rows)), f"other attributes untouched {lod} l{n}")
        all_new = positions_of(f, "LodHigh", 0) + positions_of(f, "LodMed", 0)
        low = min(p[2] for p in all_new)
        lowest.append(low)
        drop = 1.40 - low
        # drawable box lives 1 m above the data: it must move by the drop, not swallow the raw points
        bmin = item.find("BoundingBoxMin")
        check(abs(float(bmin.get("z")) - (2.4 - drop)) < 1e-6, f"drawable box shifted l{n}: {bmin.get('z')}")
        geom_min = item.find("./LodHigh/Models/Item/Geometries/Item/BoundingBoxMin")
        check(abs(float(geom_min.get("z")) - low) < 1e-6, f"geometry box covers l{n}")
        geom_max = item.find("./LodHigh/Models/Item/Geometries/Item/BoundingBoxMax")
        check(float(geom_max.get("y")) >= max(p[1] for p in positions_of(f, "LodHigh", 0)) - 1e-6,
              f"geometry box covers outward move l{n}")
        cz = float(item.find("BoundingSphereCenter").get("z"))
        radius = float(item.find("BoundingSphereRadius").get("value"))
        check(abs(cz - (2.6 - drop / 2)) < 1e-6, f"sphere centre follows l{n}: {cz}")
        check(radius >= 0.25 + drop / 2 - 1e-6, f"sphere radius grows l{n}: {radius}")
    check(lowest[0] > lowest[1] > lowest[2], f"grows every step {lowest}")


def test_extrapolate(tmp):
    base = tmp / "player_zero_beard_shv_chin_000_10.ydd.xml"
    prev = tmp / "player_zero_beard_shv_chin_000_09.ydd.xml"
    shorter = [[x, y, z + (1.80 - z) * 0.1] for x, y, z in HIGH]   # tips 10 % higher one length earlier
    make_xml(base, "player_zero_beard_shv_chin_000_10", {"LodHigh": HIGH}, semantics="NPWIT")
    make_xml(prev, "player_zero_beard_shv_chin_000_09", {"LodHigh": shorter}, semantics="NPWIT")
    out = tmp / "out_extra"
    check(gm.main([str(base), "--prev", str(prev), "--count", "2", "--out", str(out)]) == 0, "extrapolate exit")
    for n, step in ((11, 1), (12, 2)):
        f = out / f"player_zero_beard_shv_chin_000_{n}.ydd.xml"
        check(f.exists(), f"beard file _{n}")
        got = positions_of(f, "LodHigh", 4)   # N has 4 components before P
        for b, a, g in zip(HIGH, shorter, got):
            want = [bb + (bb - aa) * step for aa, bb in zip(a, b)]
            check(all(abs(x - y) < 1e-6 for x, y in zip(g, want)), f"extrapolated _{n} {g} vs {want}")


def test_topology_fallback(tmp):
    base = tmp / "player_zero_hair_s02_l09_p1.ydd.xml"
    prev = tmp / "player_zero_hair_s02_l08_p1.ydd.xml"
    make_xml(base, "player_zero_hair_s02_l09_p1", {"LodHigh": HIGH})
    make_xml(prev, "player_zero_hair_s02_l08_p1", {"LodHigh": MED})
    out = tmp / "out_fallback"
    check(gm.main([str(base), "--prev", str(prev), "--count", "1", "--out", str(out)]) == 0, "fallback exit")
    check((out / "player_zero_hair_s02_l10_p1.ydd.xml").exists(), "fallback wrote stretch result")
    check(gm.main([str(base), "--prev", str(prev), "--mode", "extrapolate", "--out", str(out)]) == 2,
          "forced extrapolate refuses mismatch")


def test_slots(tmp):
    base = tmp / "player_zero_hair_s04_l09_p0.ydd.xml"
    make_xml(base, "player_zero_hair_s04_l09_p0", {"LodHigh": HIGH}, hash_text="player_zero_hair_s04_l09_p0")
    out = tmp / "out_slots"
    slots = ["player_zero_hair_s08_l03_p1", "player_zero_hair_s08_l04_p1"]
    check(gm.main([str(base), "--slots", ",".join(slots), "--out", str(out)]) == 0, "slots exit code")
    names = sorted(p.name for p in out.iterdir())
    check(names == sorted(f"{s}.ydd.xml" for s in slots), f"slot file names {names}")
    lows = []
    for slot in slots:
        item = ET.parse(out / f"{slot}.ydd.xml").getroot().find("./Drawables/Item")
        check(item.findtext("Name") == f"{slot}.#dd", f"slot inner name {item.findtext('Name')}")
        check(item.findtext("Hash") == slot, f"slot hash {item.findtext('Hash')}")
        lows.append(min(p[2] for p in positions_of(out / f"{slot}.ydd.xml", "LodHigh", 0)))
    check(lows[0] < 1.40 and lows[1] < lows[0], f"slots hold growing lengths {lows}")


def morph_item(p):
    """CodeX morph entry: float32 x,y,z,x as uint16 halves, zero values left out."""
    out = ["      <Item>"]
    for k, f in zip((7, 9, 11, 13), (p[0], p[1], p[2], p[0])):
        lo, hi = gm._halves(f)
        if lo:
            out.append(f'       <Unknown{k} value="{lo}" />')
        if hi:
            out.append(f'       <Unknown{k + 1} value="{hi}" />')
    out.append("      </Item>")
    return "\n".join(out)


def make_cloth(path, name, points, morph_points):
    def verts(tag):
        rows = [f'     <{tag}>'] + [f'      <Item><Position x="{x}" y="{y}" z="{z}" /><UnknownC value="512" /></Item>'
                                     for x, y, z in points] + [f'     </{tag}>']
        return "\n".join(rows)
    d2 = lambda i, j: sum((a - b) ** 2 for a, b in zip(points[i], points[j]))
    text = "\n".join([
        '<?xml version="1.0" encoding="UTF-8"?>', '<RDR2ClothDictionary version="0">', " <Clothes>", "  <Item>",
        f"   <Name>{name}</Name>", "   <Controller>", "    <Name>CLOTHSIM_Playerhair_L09</Name>",
        "    <MorphController>", "     <Unknown18>", *[morph_item(p) for p in morph_points], "     </Unknown18>",
        "    </MorphController>", "    <VerletCloth1>", verts("Vertices"), verts("Vertices2"), "     <Constraints>",
        # first constraint leaves out Unknown00 (= point 0), like CodeX does for zeros
        f'      <Item><Unknown02 value="1" /><Unknown04 value="{d2(0, 1):.6f}" /><Unknown08 value="0.5" /></Item>',
        f'      <Item><Unknown00 value="1" /><Unknown02 value="2" /><Unknown04 value="{d2(1, 2):.6f}" /></Item>',
        # padding entry as in the game files: point 0 to itself, value 1e8
        '      <Item><Unknown04 value="100000000" /><Unknown10 value="1" /></Item>',
        "     </Constraints>", "    </VerletCloth1>", verts("Vertices"), "   </Controller>",
        '   <Bounds type="Composite" version="2">', '    <Mass value="1" />', "    <Bounds>",
        '     <Item type="TaperedCapsule" version="2"><SphereRadius value="0.15" />',
        "      <Transform>1 0 0 0\n        0 1 0 0\n        0 0 1 0\n        0.025 -0.016 0 1</Transform></Item>",
        '     <Item type="TaperedCapsule" version="2"><SphereRadius value="0.187" />',
        "      <Transform>1 0 0 0\n        0 -0.16 0.98 1.67\n        0 -0.98 -0.16 -0.67\n"
        "        -0.0001 -0.044 -0.02 1</Transform><HalfHeight value=\"0.091\" /></Item>",
        "    </Bounds>", "   </Bounds>", "   <BoneIds>21030 15082</BoneIds>", "  </Item>",
        " </Clothes>", "</RDR2ClothDictionary>", ""])
    path.write_text(text, encoding="utf-8")


def test_cloth(tmp):
    folder = tmp / "cloth"
    folder.mkdir()
    base = folder / "player_zero_hair_s04_l09_p0.ydd.xml"
    make_xml(base, "player_zero_hair_s04_l09_p0", {"LodHigh": HIGH}, hash_text="player_zero_hair_s04_l09_p0")
    sim = [[0.0, 0.0, 1.80], [0.0, 0.08, 1.60], [0.02, 0.1, 1.40]]
    morph = [[0.02, 0.1, 1.40], [0.0, 0.08, 1.45]]   # second has x = 0: its halves get left out
    make_cloth(folder / "player_zero_hair_s04_l09_p0.yld.xml", "player_zero_hair_s04_l09_p0", sim, morph)
    out = tmp / "out_cloth"
    check(gm.main([str(base), "--slots", "player_zero_hair_s08_l03_p1", "--out", str(out)]) == 0, "cloth exit")
    f = out / "player_zero_hair_s08_l03_p1.yld.xml"
    check(f.exists(), "cloth file written under the slot name")
    if not f.exists():
        return
    # only cloth-driven vertices grow, starting at the top of the cloth-driven part
    stretch = gm.Stretch(morph, 1, 0.15, 0.35, 0.25, anchor=max(p[2] for p in morph),
                         direction=gm.outward_direction(HIGH))
    check(stretch.oy > 0.9, f"drift points backwards like the whole model ({stretch.ox:.2f}, {stretch.oy:.2f})")
    gfx = positions_of(out / "player_zero_hair_s08_l03_p1.ydd.xml", "LodHigh", 0)
    close = lambda a, b: all(abs(x - y) < 1e-6 for x, y in zip(a, b))
    for src, got in zip(HIGH, gfx):
        want = stretch(src) if src == [0.02, 0.1, 1.40] else src
        check(close(got, want), f"gfx vertex {src}: {got} vs {want}")
    check(gfx[4][2] < 1.40, "cloth-driven vertex grew")
    check(close(gfx[3], [0.0, 0.1, 1.45]), "rigid vertex below the old anchor stays put")
    root = ET.parse(f).getroot()
    item = root.find("./Clothes/Item")
    check(item.findtext("Name") == "player_zero_hair_s08_l03_p1", "cloth renamed")
    ctrl = item.find("Controller")
    for list_path in ("VerletCloth1/Vertices", "VerletCloth1/Vertices2", "Vertices"):
        got = [gm.read_vec(p) for p in ctrl.findall(f"{list_path}/Item/Position")]
        check(all(close(g, stretch(p)) for g, p in zip(got, sim)), f"cloth points moved in {list_path}")
    # rest lengths must match the points as written to the file (7 decimals)
    moved = [gm.read_vec(p) for p in ctrl.findall("VerletCloth1/Vertices/Item/Position")]
    for item_c, (i, j) in zip(ctrl.findall("VerletCloth1/Constraints/Item"), ((0, 1), (1, 2))):
        want = sum((a - b) ** 2 for a, b in zip(moved[i], moved[j]))
        check(abs(float(item_c.find("Unknown04").get("value")) - want) < 1e-9, f"rest length {i}-{j} recomputed")
        moves = min(sim[i][2], sim[j][2]) < stretch.anchor
        changed = abs(want - sum((a - b) ** 2 for a, b in zip(sim[i], sim[j]))) > 1e-6
        check(changed == moves, f"rest length {i}-{j} changes only below the anchor")
    padding = ctrl.findall("VerletCloth1/Constraints/Item")[2]
    check(padding.find("Unknown04").get("value") == "100000000", "padding constraint left alone")
    for m_item, p in zip(ctrl.findall("MorphController/Unknown18/Item"), morph):
        vals = [gm._f32(gm._value(m_item, f"Unknown{k}"), gm._value(m_item, f"Unknown{k + 1}")) for k in (7, 9, 11, 13)]
        want = stretch(p)
        check(close(vals[:3], want) and abs(vals[3] - vals[0]) < 1e-9, f"morph position moved {vals} vs {want}")
    check(ctrl.find("MorphController/Unknown18/Item[2]") is not None, "morph keeps all entries")

    # back collision: four collar copies on the spine bones, centred on the bone
    ids = [int(v) for v in item.findtext("BoneIds").split()]
    check(ids == [21030, 15082, 14413, 14412, 14411, 14410], f"spine bones appended {ids}")
    capsules = item.findall("Bounds/Bounds/Item")
    check(len(capsules) == 6, f"capsule count {len(capsules)}")
    for capsule in capsules[2:]:
        check(capsule.find("HalfHeight").get("value") == "0.091", "copy keeps the collar shape")
        rows = [line.split() for line in capsule.findtext("Transform").strip().splitlines()]
        check(rows[1] == ["0", "-0.16", "0.98", "1.67"], "copy keeps the collar rotation")
        check(rows[3] == ["0", "0", "0", "1"], f"copy is centred on its bone {rows[3]}")
    collar_rows = [line.split() for line in capsules[1].findtext("Transform").strip().splitlines()]
    check(collar_rows[3] == ["-0.0001", "-0.044", "-0.02", "1"], "original collar capsule unchanged")

    # --collide '' leaves the bounds alone
    out2 = tmp / "out_cloth_nocollide"
    check(gm.main([str(base), "--slots", "player_zero_hair_s08_l03_p1", "--collide", "", "--out", str(out2)]) == 0,
          "no-collide exit")
    item2 = ET.parse(out2 / "player_zero_hair_s08_l03_p1.yld.xml").getroot().find("./Clothes/Item")
    check(item2.findtext("BoneIds") == "21030 15082", "no-collide keeps bone ids")


def test_direction(tmp):
    # tips that hang backwards (like a mustache) must still drift forward with --direction forward
    base = tmp / "player_zero_beard_shv_stache_000_10.ydd.xml"
    make_xml(base, "player_zero_beard_shv_stache_000_10", {"LodHigh": HIGH})
    check(gm.outward_direction(HIGH)[1] > 0.9, "auto would drift backwards here")
    out = tmp / "out_dir"
    check(gm.main([str(base), "--slots", "player_zero_beard_shv_stache_001_02", "--direction", "forward",
                   "--out", str(out)]) == 0, "direction exit")
    got = positions_of(out / "player_zero_beard_shv_stache_001_02.ydd.xml", "LodHigh", 0)
    low_src, low_new = HIGH[4], got[4]
    check(low_new[1] < low_src[1] and abs(low_new[0] - low_src[0]) < 1e-9, f"drifts forward only {low_new}")
    check(not (out / "player_zero_beard_shv_stache_001_02.yld.xml").exists(), "beard gets no cloth file")


def test_names():
    check(gm.split_name("player_zero_hair_s04_l09_p0") == ("player_zero_hair_s04_l", 9, "_p0"), "split hair")
    check(gm.split_name("player_zero_beard_shv_stache_002_10") == ("player_zero_beard_shv_stache_002_", 10, ""),
          "split beard")
    try:
        gm.split_name("my_hat")
        check(False, "bad name rejected")
    except ValueError:
        pass
    check(gm.joaat("player_zero_beard_shv_chin_000_02") == 0xB0E20518, "joaat matches femga")


if __name__ == "__main__":
    tmp = Path(tempfile.mkdtemp(prefix="grow_models_test_"))
    try:
        test_names()
        test_stretch(tmp)
        test_extrapolate(tmp)
        test_topology_fallback(tmp)
        test_slots(tmp)
        test_cloth(tmp)
        test_direction(tmp)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("FAILED: %d" % len(FAILS) if FAILS else "ALL OK")
    sys.exit(1 if FAILS else 0)
