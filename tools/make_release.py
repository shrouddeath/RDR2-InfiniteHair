"""Build the Nexus release zip for Infinite Hair.

Collects the built .asi, the release .ini and the model files that LML streams
(converted with CodeX into <game>/lml/InfiniteHair/stream), checks that every
[Chains] slot in the .ini has its model (hair also its cloth .yld), and zips it:

  python tools/make_release.py --game "C:/Program Files (x86)/Steam/steamapps/common/Red Dead Redemption 2"

The model files are edited Rockstar assets: fine to share as a mod on Nexus,
but they are not part of the source repository.
"""

import argparse
import re
import shutil
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def version_of(script: Path) -> str:
    m = re.search(r'kVersion\s*=\s*"([^"]+)"', script.read_text(encoding="utf-8"))
    if not m:
        raise SystemExit("could not find kVersion in src/script.cpp")
    return m.group(1)


def chain_slots(ini: Path):
    slots = []
    for line in ini.read_text(encoding="utf-8-sig").splitlines():
        m = re.match(r"^(player_zero_\w+)\s*=\s*(\w+)\s*$", line.strip())
        if m:
            slots.append((m.group(1), m.group(2)))
    return slots


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", type=Path, required=True, help="game folder with lml/InfiniteHair/stream")
    ap.add_argument("--out", type=Path, default=ROOT / "release")
    args = ap.parse_args(argv)

    version = version_of(ROOT / "src" / "script.cpp")
    asi = ROOT / "build" / "InfiniteHair.asi"
    ini = ROOT / "dist" / "InfiniteHair.ini"
    readme = ROOT / "dist" / "README_release.txt"
    install_xml = ROOT / "lml" / "InfiniteHair" / "install.xml"
    stream = args.game / "lml" / "InfiniteHair" / "stream"

    problems = []
    for f in (asi, ini, readme, install_xml):
        if not f.exists():
            problems.append(f"missing {f}")
    if not stream.is_dir():
        problems.append(f"missing stream folder {stream}")
    if problems:
        print("\n".join(problems), file=sys.stderr)
        return 1

    ini_text = ini.read_text(encoding="utf-8-sig")
    if re.search(r"^RecordMode\s*=\s*1", ini_text, re.M):
        problems.append("dist/InfiniteHair.ini has RecordMode=1 (debug logging) - set it to 0 for a release")

    present = {p.name.lower(): p for p in stream.iterdir() if p.is_file()}
    needed = set()
    slots = chain_slots(ini)
    for virtual, slot in slots:
        files = [f"{slot}.ydd"] + ([f"{slot}.yld"] if "_hair_" in slot else [])
        for name in files:
            needed.add(name.lower())
            if name.lower() not in present:
                problems.append(f"{virtual}: {name} is missing in {stream}")
    extra = sorted(set(present) - needed)
    if problems:
        print("\n".join(problems), file=sys.stderr)
        return 1

    name = f"InfiniteHair-{version}"
    staging = args.out / name
    if staging.exists():
        shutil.rmtree(staging)
    (staging / "lml" / "InfiniteHair" / "stream").mkdir(parents=True)
    shutil.copy2(asi, staging / asi.name)
    shutil.copy2(ini, staging / ini.name)
    shutil.copy2(readme, staging / "README.txt")
    shutil.copy2(install_xml, staging / "lml" / "InfiniteHair" / "install.xml")
    for key in sorted(needed):
        shutil.copy2(present[key], staging / "lml" / "InfiniteHair" / "stream" / present[key].name)

    archive = args.out / f"{name}.zip"
    if archive.exists():
        archive.unlink()
    # Files at the zip root (InfiniteHair.asi, lml/...): Vortex and manual installs
    # both expect the archive to mirror the game folder.
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(staging.rglob("*")):
            if f.is_file():
                z.write(f, f.relative_to(staging))

    print(f"{archive}  ({archive.stat().st_size / 1e6:.1f} MB)")
    print(f"  {len(slots)} chains, {len(needed)} model files, version {version}")
    if extra:
        print(f"  note: {len(extra)} file(s) in the stream folder are not used and were left out: {', '.join(extra)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
