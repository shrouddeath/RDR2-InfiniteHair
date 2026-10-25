# Native-Recherche

Quellen:
- [RDR3 NativeDB](https://alloc8or.re/rdr3/nativedb/) (Rohdaten `alloc8or/rdr3-nativedb-data`)
- [femga/rdr3_discoveries](https://github.com/femga/rdr3_discoveries):
  `clothes/cloth_hash_names.lua` (Shop-Items) und
  `clothes/cloth_drawable_albedo_normal_material_TEMPORARY.lua` (Drawables)
- Eigene Logs und Barbier-Screenshots vom 25.10.2026 (in diesem Ordner gesichert)

## Wie RDR2 Haare und Bart aufbaut

| Ebene | Beispiel | Bestimmt |
|---|---|---|
| Shop-Item (Kategorie `hair`) | `0xA83C1A4A` = `CLOTHING_ITEM_HAIR_SWEPT_BACK`, `0x264A371E` = `CLOTHING_ITEM_HAIR_BALD` | Frisur |
| Shop-Item (Kategorien `beards_chin` / `beards_mustache` / `beards_chops`) | `CLOTHING_ITEM_BEARD_SINGLE`, `…_FORKED`, `CLOTHING_ITEM_STACHE_NORMAL`, `…_HANDLEBAR`, `…_002`, `CLOTHING_ITEM_CHOPS_NORMAL`, `…_DUNBEARY` | Bartstil je Teil |
| Drawable | `player_zero_hair_s02_l07_p0`, `player_zero_beard_shv_chin_000_10` | **Länge** (+ Pomade bei Haaren) |

Belege:
- Beim Barbier blieb das Shop-Item während „Länge 7 → 1“ gleich (Log + Screenshots
  11:00:44–11:01:43), obwohl die Haare sichtbar kürzer wurden.
- Die 11 Einträge der Barbier-Liste („x von 11“) entsprechen den aufgezeichneten Haar-Items:
  Pomade, Seitenscheitel links `0xCF748206`, … ausrasiert `0x4C43EAB8`, Mittelscheitel
  `0x1F992CE6`, … bis Glatze `0x264A371E`.
- Alle 181 Drawable-Namen aus der femga-Liste ergeben mit joaat exakt die gelisteten Hashes.
  Das prüft `test.bat` (tests/groom_test.cpp, tests/femga_player_zero_groom.txt).

Bekannte Längen (femga, Liste ist unvollständig):
- Haare: Frisur s01 = l01–l09, s02 = l02–l09, s03–s08 = l03–l09, jeweils p0 und p1.
  Das sind 7 Längen bei s03–s08, passend zu „Länge x von 7“ beim Barbier. **l09 ist also
  vermutlich schon Arthurs Maximum.**
- Bart (`shv`-Satz): chin 000/001, chops 000/001, stache 000/001 jeweils _02–_10,
  stache 002 _04–_10.

## Benutzte Natives

| Zweck | Native | Hash |
|---|---|---|
| Anzahl Teile | `_GET_NUM_COMPONENTS_IN_PED` | `0x90403E8107B60E81` |
| Shop-Item an Index (+ Wearable State) | `_GET_SHOP_ITEM_COMPONENT_AT_INDEX` | `0x77BA37622E22023B` |
| Kategorie eines Shop-Items | `_GET_SHOP_ITEM_COMPONENT_CATEGORY` | `0x5FF9A878C3D115B8` |
| Drawable + Texturen an Index | `GET_META_PED_ASSET_GUIDS` | `0xA9C28516A6DC9D56` |
| Farbe an Index | `GET_META_PED_ASSET_TINT` | `0xE7998FEC53A33BBE` |
| Drawable setzen (ersetzt Slot) | `_SET_META_PED_TAG` | `0xBC6DF00D7A4A6819` |
| Ped neu aufbauen | `_UPDATE_PED_VARIATION` | `0xCC8CA3E88256E58F` |
| Existenz prüfen (Probe) | `0x3FCBB5FCFD968698` + `_IS_META_PED_ASSET_VALID` / `_HAS_META_PED_ASSET_LOADED` / `_RELEASE_META_PED_ASSET_REQUEST` | `0x93FFD92F05EC32FD`, `0xB0B2C6D170B0E8E5`, `0x13E7320C762F0477` |
| Barbier/Menü erkennen | `IS_PLAYER_CONTROL_ON`, `IS_PED_USING_ANY_SCENARIO`, `IS_PAUSE_MENU_ACTIVE` | `0x7964097FCE4C244B`, `0x57AB4A3080F85143`, `0x535384D6067BA42E` |

## Im Spiel bestätigt (25.10.2026, `docs/test-2026-10-25-drawables.log`)

- `GET_META_PED_ASSET_GUIDS` für Index 0…N-1 (N aus `_GET_NUM_COMPONENTS_IN_PED`) liefert
  die Drawables, und zwar mit demselben Index wie das zugehörige Shop-Item.
- `_SET_META_PED_TAG` **ersetzt** das Drawable. Es entstehen keine Doppel, und der Wechsel
  ist nach unter einer Sekunde sichtbar. Haare 3→9 und Bart 2→10 liefen ohne Rücksetzer.
- Arthur trägt `CLOTHING_ITEM_HAIR_SWEPT_BACK` = Frisur **s04**. Haare enden bei **l09**,
  der Bart bei **10**.
- Existenz-Probe: Ein vorhandenes Drawable lädt sofort („existiert“). Ein nicht
  vorhandenes (l10–l12, Bart 11–13) wird sofort mit `-1` abgelehnt. Damit erkennt der Mod
  neue Modelle automatisch.
- Die Wearable States der Bart-Items ändern sich mit der Länge (z. B. `0xE13B7C97` bei
  Schnurrbart 09, `0x47214797` bei 10). Für den Mod ist das nicht nötig.

## Schritt 2: neue Modelle

- LML kann neue Dateien streamen: `<StreamingFiles>ordner</StreamingFiles>` in
  `install.xml`. Vorbild ist „Ambient Gang“, im `vfs.log` als `addonDLC:/…` sichtbar.
- XML-Format von CodeX für RDR2 (aus Sollumz_RDR `cwxml/drawable.py`, `drawable_RDR.py`):
  `RDR2DrawableDictionary/Drawables/Item` → `LodHigh|LodMed|LodLow|LodVeryLow/Models/Item`
  → `Geometries/Item` → `VertexLayout/Semantics` (ein Buchstabe pro Attribut: P, N, X, Y,
  W, I, C, T) + `Vertices` (eine Zeile pro Punkt).
- Echter Export (25.10.2026, `player_zero_hair_s04_l09_p0`): nur `LodHigh`, eine Geometrie
  (`ped_standard_hair`, 11093 Punkte, Semantics `PNXWWIICCT`, Formats `3995566552`, also
  37 Zahlen pro Zeile, Position vorne). Name `….#dd`, Hash als Klartext-Name. Die
  Drawable-Box liegt 1 m über den Vertex-Daten (z 1,52 statt 0,52). **+Y zeigt nach hinten.**
  l08 hat 11446 Punkte, also eine andere Topologie: kein Fortsetzen möglich, wir strecken.
- Zu jedem Haar gehört eine **Cloth-Datei** (`.yld`, `RDR2ClothDictionary`, Controller
  `CLOTHSIM_Playerhair_L09`): 216 Verlet-Punkte, 528 Constraints, ein MorphController mit
  3502 gespeicherten Positionen (je vier uint16-Paare, also float32 xyzw). Eine kopierte
  `.yld` würde die Haare vermutlich auf die alte Form zurückziehen. Erster Test daher
  **ohne** `.yld`: Die Haare hängen dann ohne Physik am Kopf.
- Test 25.10.2026 (`docs/test-2026-10-25-lml-probe.log` und Log danach): LML registriert
  `player_zero_hair_s04_l10_p0.ydd` usw. als neue Streaming-Dateien („Registered … with
  6e35“). Trotzdem liefert die Metaped-Anfrage `-1`, und `_SET_META_PED_TAG` mit dem neuen
  Hash wird **dreimal ignoriert** („laesst sich nicht setzen“). Auf den Screenshots ist
  keine Änderung zu sehen, die Haare verschwinden aber auch nicht. **Das Metaped-System
  kennt nur Drawables aus seinem eigenen Verzeichnis.**
- `player_zero.ymt` enthält nur 206 Outfit-Vorlagen (`explicitAssets`, meist `S01_L05_P0`),
  keine Liste, die Drawables freischaltet. s04 kommt darin gar nicht vor.
- Ablage im Spiel (CodeX): `packs_1.rpf\packs\base\models\metaped_player.rpf\` und, neuer,
  `x64\dlcpacks\patchpack001\dlc.rpf\x64\packs\base\models\component_models.rpf\`. Beide haben
  eine `_manifest.ymf`. **`.yld` gibt es nur für s01–s04 bei l08/l09. s05–s08 haben keine
  Cloth-Dateien.** Deshalb leihen sich Längen ab 10 Plätze von s05–s08 (`[Chains]` in der
  `.ini`, `--slots` im Generator). Erster Test: s04 l10–l12 → s08_l03–l05_p1.
- **Test 25.10.2026 13:08 (`docs/test-2026-10-25-slots.log`): Geliehene Plätze
  funktionieren.** s04 l10–l12 auf s08_l03–l05_p1 werden gesetzt und zurückgemeldet. Danach
  meldet das Spiel das Shop-Item `0x57B7420F` (das von s08). Optisch hängen sie ohne Cloth
  steif und wirken kürzer als das physik-simulierte Original l09.
- Cloth (.yld): `Constraints*/Unknown04` = quadrierte Ruhelänge (569/569 echte Federn). 84
  Platzhalter (Punkt 0–0, Wert 1e8) bleiben unverändert. MorphController/Unknown18 = 3502
  Positionen der cloth-gesteuerten Haar-Vertices (float32 x,y,z,x in uint16-Hälften). Der
  Generator streckt die `.yld` mit derselben Formel mit.
- **Test 25.10.2026 13:21 (`docs/test-2026-10-25-cloth.log` + Screenshots):** l10 auf
  s08_l03_p1 mit **neuer** `.yld` und l11 auf s03_l09_p1 mit **ersetzter** `.yld` zeigen
  beide lange, physik-simulierte Haare. l12 ohne `.yld` sieht aus wie eine „halbe Glatze“:
  Der cloth-gesteuerte untere Teil verschwindet ohne Cloth-Datei. **Jede Länge braucht
  also ihre `.yld`.** LML registriert neue `.yld`-Namen („Registered … .yld with 3535c“).
- Ausbau: 20 Längen (l10–l29, je +3 cm, l29 ≈ Hüfte) auf s08 p1, s08 p0 und s07 p1
  (l03–l08), alle mit `.yld`. Der Platz s03_l09_p1 ist wieder frei.
- **Test 25.10.2026 13:48 (`docs/test-2026-10-25-l29.log`, `docs/laenge-16-20-28-29.png`):
  Alle 20 Längen funktionieren.** LML registriert 40 Dateien ohne Fehler, der Mod wächst
  ohne Rücksetzer von 12 bis 29. Die Haare hängen mit Physik bis zum Gürtel. Noch frei für
  mehr Längen: s07 p0 (7) sowie s05 und s06 (je 14), also bis l64 möglich.
- **Problem: Bei den längsten Längen verschwinden Haare im Rücken.** Zwei Ursachen:
  1. Nur 3654 von 11093 Vertices sind cloth-gesteuert (alle mit z < 0,70; zwischen 0,64 und
     0,70 nur 59 %). Der Rest ist starr an Kopf und Hals gebunden. Der alte Generator hat
     auch starre Punkte gestreckt (Anker 0,723, Faktor bis 4). Diese starren langen Strähnen
     drehen mit dem Kopf in den Rücken. **Fix:** Nur cloth-gesteuerte Vertices wachsen
     (Abgleich über MorphController-Positionen, Toleranz 0,5 mm, alle 3502 treffen). Der
     Anker sitzt an der Oberkante des Cloth-Teils (0,699). Die Drift-Richtung kommt weiter
     vom ganzen Modell, denn der Cloth-Teil allein ergibt +X, also zur Seite.
  2. Kollisionskapseln gibt es nur für skel_head, skel_l/r_clavicle und
     hlp_LongHairHighCollarCol (`Clothes/Item/BoneIds` 21030 30226 54802 15082, je ein
     TaperedCapsule in `Bounds/Bounds`). **Fix:** Die Kragen-Kapsel wird auf skel_spine3/2/1/0
     (14413/14412/14411/14410) kopiert, mit gleicher Form und Drehung, Versatz 0.
- **Test 25.10.2026 14:27 (`docs/test-2026-10-25-collision.log`): Haare bleiben auf dem
  Rücken**, bis Länge 29, im Stehen, Gehen und Reiten. Beim Aufsteigen setzt das Spiel
  Outfit und Hut neu und dabei die Haare auf l09 zurück (3×). LockBarber stellt l29 sofort
  wieder her.
- Bart: keine `.yld`. Freie Plätze laut CodeX: chin_001 (02,03,05,06,08,09,10), chops_001
  (02–10), stache_001 (02,04–10), stache_002 (04–10). Erzeugt: je 7 Längen (11–17) mit
  `--direction forward --outward 0.3`. Die automatische Drift zeigt beim Schnurrbart nach
  hinten und bei den Koteletten zur Seite.
- Ausbau nach Wunsch „gleich bis 29“: **Kinnbart 11–29** auf chin_001 (7) und danach auf
  **Johns** Plätzen `player_three_ms1_beard_shv_chin_000_*` (8) und `…chin_001_02–05` (4),
  Drift 0,1. Der Mod nimmt jetzt beliebige Platz-Namen an, im Epilog trägt John dann
  Arthurs Bart. **Koteletten nur 11–14** (chops_001_02–05, Drift 0,5): Ihre Unterkante
  sitzt seitlich am Hals (x ±0,13, y −0,06…−0,13), länger würden sie durch Schultern und
  Brust gehen. **Schnurrbart 11–17** (stache_001). Vorschau: `docs/bart-10-17-23-29-vorne.png`,
  `docs/arthur-9-vs-29-seite.png`. Noch nicht im Spiel getestet, auch nicht, ob Johns
  Drawables an Arthur angenommen werden. Sonst stoppt der Mod bei 17 („laesst sich nicht
  setzen“).
- Frühere Idee: Arthurs Metaped-Definition `player_zero.ymt` (laut Community per LML
  streambar) mit CodeX als XML exportieren, die neuen Längen eintragen und gestreamt
  ersetzen. Rückfall-Option: unbenutzte vorhandene Plätze anderer Frisuren belegen. Dabei
  ist die Cloth-Datei (`.yld`) des Platzes zu beachten.
- Sackgasse: Shop-Items als Längen zu sortieren (erste Versionen) war falsch. Die
  Vorschau-Nummern aus den Screenshots vom 25.10. sind Frisuren, keine Längen.
