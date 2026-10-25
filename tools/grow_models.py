"""Generate longer hair/beard models for Infinite Hair (step 2 of the project).

Input:  CodeX XML exports (*.ydd.xml) of one of Arthur's grooming drawables at
        its longest vanilla length, e.g. player_zero_hair_s04_l09_p0.ydd.xml.
        Optionally the previous length too (player_zero_hair_s04_l08_p0).
Output: XML files for the next lengths (l10, l11, ... / _11, _12, ...), ready
        to be imported back into CodeX and streamed with LML. The script only
        touches vertex positions, bounding boxes, Name and Hash; skinning,
        shaders and textures stay exactly as in the original.

Two ways to grow:
  extrapolate  continue each vertex's own growth from the previous length to
               the longest one (both files need the same mesh topology)
  stretch      pull everything below an anchor height further down
"auto" (default) picks extrapolate when a matching --prev file is given.

XML layout (from Sollumz_RDR, cwxml/drawable.py + drawable_RDR.py):
  RDR2DrawableDictionary/Drawables/Item          one drawable: Name, Hash, bounds
    LodHigh|LodMed|LodLow|LodVeryLow/Models/Item  model: bounds
      Geometries/Item                             geometry: bounds,
        VertexLayout/Semantics                    one letter per vertex attribute
        Vertices                                  one text row per vertex
"""

import argparse
import copy
import math
import re
import struct
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# Components per vertex attribute and per format code (Sollumz_RDR drawable_RDR.py).
ATTR_COMPONENTS = {"P": 3, "N": 4, "X": 4, "Y": 4, "W": 4, "I": 4, "C": 4, "T": 2}
FORMAT_COMPONENTS = {"1": 1, "2": 2, "3": 3, "4": 4, "5": 4, "6": 4, "7": 2, "8": 4, "9": 4,
                     "A": 4, "B": 2, "C": 2}
LODS = ("LodHigh", "LodMed", "LodLow", "LodVeryLow")

NAME_PATTERNS = [
    re.compile(r"^(player_zero_hair_s\d\d_l)(\d\d)(_p\d)$", re.IGNORECASE),
    re.compile(r"^(player_zero_beard_(?:shv_)?(?:chin|stache|chops)_\d{3}_)(\d\d)()$", re.IGNORECASE),
]


def joaat(text: str) -> int:
    h = 0
    for c in text.lower().encode():
        h = (h + c) & 0xFFFFFFFF
        h = (h + (h << 10)) & 0xFFFFFFFF
        h ^= h >> 6
    h = (h + (h << 3)) & 0xFFFFFFFF
    h ^= h >> 11
    return (h + (h << 15)) & 0xFFFFFFFF


def split_name(name: str):
    """'player_zero_hair_s04_l09_p0' -> ('player_zero_hair_s04_l', 9, '_p0')."""
    for pattern in NAME_PATTERNS:
        m = pattern.match(name)
        if m:
            return m.group(1), int(m.group(2)), m.group(3)
    raise ValueError(f"'{name}' is not a known hair/beard drawable name")


def model_name(path: Path) -> str:
    name = path.name
    for suffix in (".ydd.xml", ".xml", ".ydd"):
        if name.lower().endswith(suffix):
            return name[: -len(suffix)]
    return path.stem


class Geometry:
    """Vertex rows of one <Geometries>/<Item>, with positions editable in place."""

    def __init__(self, elem: ET.Element):
        self.elem = elem
        layout = elem.find("VertexLayout")
        semantics = (layout.findtext("Semantics") or "").strip() if layout is not None else ""
        formats = (layout.findtext("Formats") or "").strip() if layout is not None else ""
        if "P" not in semantics:
            raise ValueError(f"geometry without position semantic: '{semantics}'")
        self.offset = 0
        for i, letter in enumerate(semantics):
            if letter == "P":
                break
            comps = ATTR_COMPONENTS[letter]
            if formats and i < len(formats):
                comps = FORMAT_COMPONENTS.get(formats[i], comps)
            self.offset += comps

        self.vertices = elem.find("Vertices")
        self.rows = []        # per row: (indent, [token, sep, token, sep, ..., token])
        self.positions = []   # per row: [x, y, z]
        text = self.vertices.text or ""
        self.leading, body, self.trailing = re.match(r"(\s*)(.*?)(\s*)$", text, re.DOTALL).groups()
        for line in body.splitlines() if body else []:
            core = line.strip()
            if not core:
                continue
            indent = line[: len(line) - len(line.lstrip())]
            parts = re.split(r"(\s+)", core)
            tokens = parts[0::2]
            if len(tokens) < self.offset + 3:
                raise ValueError(f"vertex row too short: '{core[:80]}'")
            self.rows.append((indent, parts))
            self.positions.append([float(t) for t in tokens[self.offset:self.offset + 3]])

    def write(self, positions):
        lines = []
        for (indent, parts), pos in zip(self.rows, positions):
            parts = list(parts)
            for k in range(3):
                parts[2 * (self.offset + k)] = f"{pos[k]:.7f}"
            lines.append(indent + "".join(parts))
        # The first row's indent is part of self.leading already.
        if lines:
            lines[0] = lines[0].lstrip()
        self.vertices.text = self.leading + "\n".join(lines) + self.trailing


class Model:
    def __init__(self, path: Path):
        self.path = path
        self.tree = ET.parse(path)
        root = self.tree.getroot()
        if "RDR2" not in root.tag:
            raise ValueError(f"{path.name}: root <{root.tag}> is not an RDR2 CodeX export")
        self.drawables = root.findall("./Drawables/Item")
        if not self.drawables:
            raise ValueError(f"{path.name}: no <Drawables>/<Item> found")
        self.geoms = {}   # (drawable index, lod, model index, geometry index) -> Geometry
        for d, drawable in enumerate(self.drawables):
            for lod in LODS:
                for m, model in enumerate(drawable.findall(f"./{lod}/Models/Item")):
                    for g, geom in enumerate(model.findall("./Geometries/Item")):
                        self.geoms[(d, lod, m, g)] = Geometry(geom)

    def topology(self):
        return {key: len(g.positions) for key, g in self.geoms.items()}

    def report(self):
        print(f"{self.path.name}:")
        for d, drawable in enumerate(self.drawables):
            print(f"  drawable {d}: Name={drawable.findtext('Name')!r} Hash={drawable.findtext('Hash')!r}")
        for key, g in self.geoms.items():
            semantics = self.geoms[key].elem.find("VertexLayout").findtext("Semantics").strip()
            zs = [p[2] for p in g.positions] or [0.0]
            print(f"  {key}: {len(g.positions)} vertices, semantics {semantics}, z {min(zs):.3f}..{max(zs):.3f}")


def vec_attrs(elem, xyz):
    elem.set("x", f"{xyz[0]:.7f}")
    elem.set("y", f"{xyz[1]:.7f}")
    elem.set("z", f"{xyz[2]:.7f}")


def read_vec(elem):
    return [float(elem.get(axis, "0")) for axis in "xyz"]


def bounds_of(points):
    lo = [min(p[i] for p in points) for i in range(3)]
    hi = [max(p[i] for p in points) for i in range(3)]
    return lo, hi


def grow_box(elem_parent, old_points, new_points):
    """Widen BoundingBoxMin/Max of elem_parent by how far the points moved past
    their old extent. Works in whatever space the box is stored in (the
    drawable box of Arthur's hair sits 1 m above the vertex data)."""
    if not old_points:
        return None
    olo, ohi = bounds_of(old_points)
    nlo, nhi = bounds_of(new_points)
    bmin, bmax = elem_parent.find("BoundingBoxMin"), elem_parent.find("BoundingBoxMax")
    if bmin is not None:
        vec_attrs(bmin, [b + min(0.0, n - o) for b, n, o in zip(read_vec(bmin), nlo, olo)])
    if bmax is not None:
        vec_attrs(bmax, [b + max(0.0, n - o) for b, n, o in zip(read_vec(bmax), nhi, ohi)])
    return (olo, ohi), (nlo, nhi)


def update_bounds(model: Model, old_positions, new_positions):
    for d, drawable in enumerate(model.drawables):
        old_all, new_all = [], []
        for lod in LODS:
            for m, item in enumerate(drawable.findall(f"./{lod}/Models/Item")):
                old_model, new_model = [], []
                for g, geom in enumerate(item.findall("./Geometries/Item")):
                    key = (d, lod, m, g)
                    grow_box(geom, old_positions[key], new_positions[key])
                    old_model += old_positions[key]
                    new_model += new_positions[key]
                grow_box(item, old_model, new_model)
                old_all += old_model
                new_all += new_model
        extents = grow_box(drawable, old_all, new_all)
        center_elem = drawable.find("BoundingSphereCenter")
        radius_elem = drawable.find("BoundingSphereRadius")
        if extents and center_elem is not None and radius_elem is not None:
            (olo, ohi), (nlo, nhi) = extents
            shift = [((a + b) - (c + e)) / 2 for a, b, c, e in zip(nlo, nhi, olo, ohi)]
            half_old = math.dist(olo, ohi) / 2
            half_new = math.dist(nlo, nhi) / 2
            center = [c + s for c, s in zip(read_vec(center_elem), shift)]
            radius = float(radius_elem.get("value", "0")) + math.hypot(*shift) + max(0.0, half_new - half_old)
            vec_attrs(center_elem, center)
            radius_elem.set("value", f"{radius:.7f}")


def rename(model: Model, old: str, new: str):
    for drawable in model.drawables:
        name_elem = drawable.find("Name")
        if name_elem is not None:
            text = (name_elem.text or "").strip()
            # CodeX writes e.g. "player_zero_hair_s04_l09_p0.#dd"; keep such suffixes.
            if text.lower().startswith(old.lower()):
                name_elem.text = new + text[len(old):]
            else:
                print(f"  warning: <Name>{name_elem.text}</Name> doesn't match '{old}', left unchanged",
                      file=sys.stderr)
        hash_elem = drawable.find("Hash")
        if hash_elem is None or not (hash_elem.text or "").strip():
            continue
        text = hash_elem.text.strip()
        old_h, new_h = joaat(old), joaat(new)
        signed = lambda h: h - (1 << 32) if h & 0x80000000 else h
        formats = [
            (f"0x{old_h:08X}", f"0x{new_h:08X}"), (f"0x{old_h:08x}", f"0x{new_h:08x}"),
            (f"{old_h:08X}", f"{new_h:08X}"), (f"{old_h:08x}", f"{new_h:08x}"),
            (str(old_h), str(new_h)), (str(signed(old_h)), str(signed(new_h))),
            (old, new), (old.lower(), new.lower()),
        ]
        for before, after in formats:
            if text == before:
                hash_elem.text = after
                break
        else:
            print(f"  warning: <Hash>{text}</Hash> doesn't match '{old}', left unchanged", file=sys.stderr)


def outward_direction(points):
    """Horizontal direction in which the lowest tenth of the points hangs away
    from the middle: backwards for hair, forwards for a beard."""
    by_height = sorted(points, key=lambda p: p[2])
    low = by_height[: max(1, len(by_height) // 10)]
    mx = sum(p[0] for p in points) / len(points)
    my = sum(p[1] for p in points) / len(points)
    dx = sum(p[0] for p in low) / len(low) - mx
    dy = sum(p[1] for p in low) / len(low) - my
    length = math.hypot(dx, dy)
    return (dx / length, dy / length) if length > 1e-6 else (0.0, 0.0)


class Stretch:
    """Pulls everything below an anchor height down by `factor`, drifting a bit
    in the direction the tips already hang. A pure function of position, so
    the model and its cloth file stay in step.

    `reference` are the points that grow: the whole model, or for hair with a
    cloth file only the cloth-driven vertices. `anchor` defaults to the top
    `anchor_frac` of the reference."""

    def __init__(self, reference, step: int, grow: float, anchor_frac: float, outward: float, anchor=None,
                 direction=None):
        zs = [p[2] for p in reference]
        top, bottom = max(zs), min(zs)
        self.anchor = anchor if anchor is not None else top - (top - bottom) * anchor_frac
        self.ox, self.oy = direction if direction is not None else outward_direction(reference)
        self.factor = 1.0 + grow * step
        self.outward = outward

    def __call__(self, p):
        x, y, z = p
        if z >= self.anchor:
            return [x, y, z]
        new_z = self.anchor - (self.anchor - z) * self.factor
        drop = z - new_z
        return [x + self.ox * self.outward * drop, y + self.oy * self.outward * drop, new_z]


class PointSet:
    """Positions with a tolerance lookup (cloth stores them as float32, the
    model XML with fewer digits: they agree to about 0.1 mm)."""

    def __init__(self, points, tolerance=5e-4):
        self.points = list(points)
        self.tolerance = tolerance
        self.grid = {}
        for p in self.points:
            self.grid.setdefault(self._cell(p), []).append(p)

    def _cell(self, p):
        return tuple(int(math.floor(x / self.tolerance)) for x in p)

    def __contains__(self, p):
        cx, cy, cz = self._cell(p)
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    for q in self.grid.get((cx + dx, cy + dy, cz + dz), ()):
                        if math.dist(p, q) <= self.tolerance:
                            return True
        return False


DIRECTIONS = {"forward": (0.0, -1.0), "back": (0.0, 1.0)}   # model space: +Y is the back of the head


def drift_direction(points, choice):
    """'auto' guesses from where the tips hang. That works for hair but not for
    beard parts (mustache tips point back into the face, chops are lopsided),
    so those use 'forward'."""
    return DIRECTIONS[choice] if choice in DIRECTIONS else outward_direction(points)


def make_stretch(base: Model, step: int, args, cloth_driven: PointSet = None):
    high = [p for (d, lod, m, g), geom in base.geoms.items() if lod == "LodHigh" for p in geom.positions]
    whole = high or [p for g in base.geoms.values() for p in g.positions]
    direction = drift_direction(whole, args.direction)
    if cloth_driven:
        # Only cloth-driven vertices may grow: the others are skinned rigidly to
        # head and neck, and stretched rigid strands swing into the back (game
        # test 25.10.2026). Growing starts where the cloth-driven part starts.
        # The drift direction still comes from the whole model: the cloth-driven
        # part alone is lopsided and pointed sideways.
        top = max(p[2] for p in cloth_driven.points)
        return Stretch(cloth_driven.points, step, args.grow, args.anchor, args.outward, anchor=top,
                       direction=direction)
    return Stretch(whole, step, args.grow, args.anchor, args.outward, direction=direction)


def stretch_positions(base: Model, stretch: Stretch, cloth_driven: PointSet = None):
    return {key: [stretch(p) if cloth_driven is None or p in cloth_driven else list(p) for p in geom.positions]
            for key, geom in base.geoms.items()}


def _value(item, tag):
    elem = item.find(tag)
    return int(elem.get("value")) if elem is not None else 0   # CodeX leaves out zero values


def _set_value(item, tag, value, after=()):
    """Set <tag value=.../>, inserting it after the first existing tag in `after`."""
    elem = item.find(tag)
    if elem is None:
        elem = ET.Element(tag)
        position = 0
        for name in after:
            prev = item.find(name)
            if prev is not None:
                position = list(item).index(prev) + 1
                break
        item.insert(position, elem)
    elem.set("value", value)


def _f32(lo, hi):
    return struct.unpack("<f", struct.pack("<I", ((hi & 0xFFFF) << 16) | (lo & 0xFFFF)))[0]


def _halves(f):
    u = struct.unpack("<I", struct.pack("<f", f))[0]
    return u & 0xFFFF, u >> 16


# Bone ids from femga/rdr3_discoveries boneNames/player_zero__boneNames.lua.
COLLAR_BONE = 15082   # hlp_LongHairHighCollarCol: the lowest collision capsule vanilla hair has
SPINE_BONES = {"skel_spine3": 14413, "skel_spine2": 14412, "skel_spine1": 14411, "skel_spine0": 14410}


def add_back_collision(cloth: ET.Element, bones):
    """Vanilla hair only collides with head, clavicles and collar, so longer hair
    falls through the back (game test 25.10.2026). Copy the collar capsule onto
    spine bones: same shape and orientation, centred on the bone. Returns the
    number of capsules added."""
    ids_elem = cloth.find("BoneIds")
    items = cloth.findall("Bounds/Bounds/Item")
    if ids_elem is None or not items:
        return 0
    ids = [int(v) for v in (ids_elem.text or "").split()]
    if len(ids) != len(items) or COLLAR_BONE not in ids:
        return 0
    template = items[ids.index(COLLAR_BONE)]
    parent = cloth.find("Bounds/Bounds")
    added = 0
    for bone in bones:
        if bone in ids:
            continue
        capsule = copy.deepcopy(template)
        transform = capsule.find("Transform")
        if transform is not None and transform.text:
            # 4 rows "a b c d"; the last row holds the offset from the bone.
            lines = transform.text.split("\n")
            rows = [i for i, line in enumerate(lines) if line.strip()]
            if len(rows) == 4 and len(lines[rows[3]].split()) == 4:
                line = lines[rows[3]]
                indent = line[: len(line) - len(line.lstrip())]
                lines[rows[3]] = indent + "0 0 0 " + line.split()[3]   # the collar offset belongs to the helper bone
                transform.text = "\n".join(lines)
        parent.append(capsule)
        ids.append(bone)
        added += 1
    ids_elem.text = " ".join(str(i) for i in ids)
    return added


def cloth_driven_points(path: Path):
    """Positions of the hair vertices the cloth simulation moves (MorphController)."""
    points = []
    for item in ET.parse(path).getroot().findall("./Clothes/Item/Controller/MorphController/Unknown18/Item"):
        points.append(tuple(_f32(_value(item, f"Unknown{k}"), _value(item, f"Unknown{k + 1}")) for k in (7, 9, 11)))
    return points


def stretch_cloth(path: Path, transform, new_name: str, target: Path, collide=()):
    """Apply the model's stretch to its cloth file (CodeX .yld.xml).

    Cloth layout (RDR2ClothDictionary/Clothes/Item/Controller), checked against
    player_zero_hair_s04_l09_p0 on 25.10.2026:
      VerletCloth1/Vertices, VerletCloth1/Vertices2, Vertices   simulated points (Position x/y/z)
      VerletCloth1/Constraints(2)/Item   Unknown00/02 point indices, Unknown04 squared rest length
      MorphController/Unknown18/Item     a hair vertex position as float32 x,y,z,x split into
                                         uint16 halves Unknown7..Unknown14 (low half first)
    """
    tree = ET.parse(path)
    root = tree.getroot()
    if "Cloth" not in root.tag:
        raise ValueError(f"{path.name}: root <{root.tag}> is not a CodeX cloth export")
    for cloth in root.findall("./Clothes/Item"):
        name = cloth.find("Name")
        if name is not None:
            name.text = new_name
        if collide and not add_back_collision(cloth, collide):
            print(f"  warning: {path.name}: no collar capsule found, back collision not added", file=sys.stderr)
        ctrl = cloth.find("Controller")
        if ctrl is None:
            continue
        before = [read_vec(pos) for pos in ctrl.findall("VerletCloth1/Vertices/Item/Position")]
        for list_path in ("VerletCloth1/Vertices", "VerletCloth1/Vertices2", "Vertices"):
            for pos in ctrl.findall(f"{list_path}/Item/Position"):
                vec_attrs(pos, transform(read_vec(pos)))

        # Only real springs get a new rest length: their stored value equals the
        # squared distance of their points. Padding entries (point 0 to itself,
        # value 1e8) must stay as they are.
        points = [read_vec(pos) for pos in ctrl.findall("VerletCloth1/Vertices/Item/Position")]
        dist2 = lambda pts, i, j: sum((a - b) ** 2 for a, b in zip(pts[i], pts[j]))
        for constraints in ("VerletCloth1/Constraints", "VerletCloth1/Constraints2"):
            for item in ctrl.findall(f"{constraints}/Item"):
                i, j = _value(item, "Unknown00"), _value(item, "Unknown02")
                stored = item.find("Unknown04")
                if i == j or i >= len(points) or j >= len(points) or stored is None:
                    continue
                old = dist2(before, i, j)
                if abs(float(stored.get("value")) - old) > 1e-4 + 0.02 * old:
                    continue
                stored.set("value", f"{dist2(points, i, j):.9g}")

        for item in ctrl.findall("MorphController/Unknown18/Item"):
            x, y, z, _ = (_f32(_value(item, f"Unknown{k}"), _value(item, f"Unknown{k + 1}")) for k in (7, 9, 11, 13))
            nx, ny, nz = transform([x, y, z])
            for child in list(item):
                item.remove(child)
            for k, f in zip((7, 9, 11, 13), (nx, ny, nz, nx)):
                lo, hi = _halves(f)
                ET.SubElement(item, f"Unknown{k}").set("value", str(lo))
                ET.SubElement(item, f"Unknown{k + 1}").set("value", str(hi))
    tree.write(target, encoding="UTF-8", xml_declaration=True)


def extrapolate_positions(base: Model, prev: Model, step: int, gain: float):
    out = {}
    for key, geom in base.geoms.items():
        before = prev.geoms[key].positions
        out[key] = [[b + (b - a) * step * gain for a, b in zip(pa, pb)]
                    for pa, pb in zip(before, geom.positions)]
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("base", type=Path, help="CodeX export of the longest length (*.ydd.xml)")
    ap.add_argument("--prev", type=Path, help="CodeX export of the length before it (for extrapolate)")
    ap.add_argument("--count", type=int, default=10, help="how many new lengths to create (default 10)")
    ap.add_argument("--mode", choices=("auto", "extrapolate", "stretch"), default="auto")
    ap.add_argument("--grow", type=float, default=0.15,
                    help="stretch: extra length per step, as a fraction of the hanging part (default 0.15)")
    ap.add_argument("--anchor", type=float, default=0.35,
                    help="stretch: top fraction of the model that stays fixed (default 0.35)")
    ap.add_argument("--outward", type=float, default=0.25,
                    help="stretch: sideways move per metre of drop, in the direction the tips already hang "
                         "(keeps hair off the back and beards off the chest; default 0.25)")
    ap.add_argument("--direction", choices=("auto", "forward", "back"), default="auto",
                    help="stretch: drift direction; 'auto' for hair, 'forward' for beard parts (default auto)")
    ap.add_argument("--gain", type=float, default=1.0, help="extrapolate: growth per step vs. last vanilla step")
    ap.add_argument("--out", type=Path, default=None, help="output folder (default: next to base)")
    ap.add_argument("--slots", default="",
                    help="comma-separated existing drawable names to write the new lengths into, one per step "
                         "(the game ignores new names, so longer models borrow unused vanilla slots)")
    ap.add_argument("--cloth", choices=("auto", "off"), default="auto",
                    help="auto: also stretch the cloth file (<base>.yld.xml next to base) when present")
    ap.add_argument("--collide", default=",".join(SPINE_BONES),
                    help="cloth: spine bones that get a copy of the collar collision capsule so long hair "
                         f"stays on the back; '' for none (default {','.join(SPINE_BONES)})")
    ap.add_argument("--report", action="store_true", help="only print the structure of the input files")
    args = ap.parse_args(argv)
    slots = [s.strip() for s in args.slots.split(",") if s.strip()]
    if slots:
        args.count = len(slots)
    collide = []
    for bone in (b.strip() for b in args.collide.split(",") if b.strip()):
        if bone not in SPINE_BONES:
            print(f"error: unknown --collide bone '{bone}' (known: {', '.join(SPINE_BONES)})", file=sys.stderr)
            return 2
        collide.append(SPINE_BONES[bone])

    base_name = model_name(args.base)
    prefix, length, suffix = split_name(base_name)
    base = Model(args.base)
    prev = Model(args.prev) if args.prev else None
    if args.report:
        base.report()
        if prev:
            prev.report()
        return 0

    mode = args.mode
    if mode in ("auto", "extrapolate") and prev is not None:
        if prev.topology() != base.topology():
            if mode == "extrapolate":
                print("error: --prev has a different mesh topology, can't extrapolate", file=sys.stderr)
                return 2
            print("note: --prev has a different mesh topology, using stretch instead")
            mode = "stretch"
        else:
            mode = "extrapolate"
    elif mode == "auto":
        mode = "stretch"
    elif mode == "extrapolate":
        print("error: extrapolate needs --prev", file=sys.stderr)
        return 2

    out_dir = args.out or args.base.parent
    out_dir.mkdir(parents=True, exist_ok=True)
    width = len(f"{length:02d}")
    cloth = args.base.parent / f"{base_name}.yld.xml"
    if args.cloth == "off" or not cloth.exists():
        cloth = None
    elif mode != "stretch":
        print("note: cloth files are only adjusted in stretch mode; skipping the .yld")
        cloth = None
    print(f"{base_name}: {len(base.geoms)} geometries, mode {mode}" + (f", cloth {cloth.name}" if cloth else ""))
    cloth_driven = None
    if cloth:
        cloth_driven = PointSet(cloth_driven_points(cloth))
        if not cloth_driven.points:
            print("note: cloth file has no cloth-driven vertices; stretching the whole model")
            cloth_driven = None
        else:
            all_points = [p for g in base.geoms.values() for p in g.positions]
            moving = sum(1 for p in all_points if p in cloth_driven)
            print(f"  {moving} of {len(all_points)} vertices are cloth-driven and will grow")
    chain_lines = []
    for step in range(1, args.count + 1):
        new_length = length + step
        if new_length > 99:
            print("stopping at length 99 (names use two digits)")
            break
        virtual_name = f"{prefix}{new_length:0{width}d}{suffix}"
        new_name = slots[step - 1] if slots else virtual_name
        model = Model(args.base)   # fresh copy of the original for every step
        if mode == "extrapolate":
            positions = extrapolate_positions(base, prev, step, args.gain)
            info = f"gain {args.gain * step:.2f}x last step"
        else:
            stretch = make_stretch(base, step, args, cloth_driven)
            positions = stretch_positions(base, stretch, cloth_driven)
            info = (f"anchor z {stretch.anchor:.3f}, factor {stretch.factor:.2f}, "
                    f"outward ({stretch.ox:+.2f}, {stretch.oy:+.2f})")
            if cloth:
                stretch_cloth(cloth, stretch, new_name, out_dir / f"{new_name}.yld.xml", collide)
                info += ", cloth" + (f" +{len(collide)} back capsules" if collide else "")
        original = {key: geom.positions for key, geom in model.geoms.items()}
        for key, geom in model.geoms.items():
            geom.write(positions[key])
        update_bounds(model, original, positions)
        rename(model, base_name, new_name)
        drop = min(p[2] for pts in positions.values() for p in pts)
        target = out_dir / f"{new_name}.ydd.xml"
        model.tree.write(target, encoding="UTF-8", xml_declaration=True)
        print(f"  {target.name}: lowest point z {drop:.3f} ({info})")
        if slots:
            chain_lines.append(f"{virtual_name}={new_name}")
    if chain_lines:
        print("\nFor InfiniteHair.ini, section [Chains]:")
        print("\n".join(chain_lines))
    return 0


if __name__ == "__main__":
    sys.exit(main())
