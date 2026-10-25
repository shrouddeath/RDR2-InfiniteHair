# Nexus Mods – page text for Infinite Hair

Copy the parts below into the Nexus upload form (nexusmods.com → Red Dead Redemption 2 →
Upload a mod). The description editor understands the BBCode in section 4.

---

## 1. Basics

- **Mod name:** Infinite Hair - Hair and Beard Grow Beyond the Limit
- **Version:** 1.0.0
- **Author:** shrouddeath
- **Category:** Characters (or "Visuals and Graphics" if Characters doesn't fit)
- **Main file:** `release/InfiniteHair-1.0.0.zip` (name it "Infinite Hair 1.0.0")
- **Adult content:** no

## 2. Brief overview (max. 255 characters)

Arthur's hair and beard keep growing in turbo speed - far past the vanilla limit. Hair
down to the belt with working hair physics, a beard to match. Barber lock, hotkeys, fully
configurable. Story mode only.

## 3. Requirements (Nexus "Requirements" tab)

- ScriptHookRDR2 – off-site: http://www.dev-c.com/rdr2/scripthookrdr2/
- Lenny's Mod Loader – off-site: https://www.rdr2mods.com/downloads/rdr2/tools/76-lennys-mod-loader-rdr/

## 4. Description (BBCode)

```
[size=5][b]Infinite Hair[/b][/size]
[i]Hair and beard that just don't stop growing.[/i]

In vanilla RDR2 Arthur's hair stops at collar length and his beard at chest length. This mod lets both grow in turbo speed - and then keeps going: [b]20 new hair lengths[/b] down to the belt and a [b]chin beard down to the belt[/b] as well.

[size=4][b]Features[/b][/size]
[list]
[*][b]Turbo growth:[/b] one length step every 30 seconds (configurable).
[*][b]Far beyond the limit:[/b] hair length 29 instead of 9, chin beard 29 instead of 10, mustache 17, sideburns 14.
[*][b]Real hair physics:[/b] the long hair keeps its cloth simulation and swings when you walk and ride, with extra collision on the back so it stays on top of your clothes.
[*][b]Barber lock (optional):[/b] haircuts and shaves are undone at once - or turn it off and let it grow back.
[*][b]Hotkeys:[/b] F10 pause, End grow now, Delete test cut, F9 debug dump.
[*][b]English and German[/b] on-screen messages.
[/list]

[size=4][b]Requirements[/b][/size]
[list]
[*][url=http://www.dev-c.com/rdr2/scripthookrdr2/]ScriptHookRDR2[/url] (matching your game version)
[*][url=https://www.rdr2mods.com/downloads/rdr2/tools/76-lennys-mod-loader-rdr/]Lenny's Mod Loader[/url]
[/list]

[size=4][b]Installation[/b][/size]
[list=1]
[*]Copy [b]InfiniteHair.asi[/b] and [b]InfiniteHair.ini[/b] into your game folder (where RDR2.exe is).
[*]Copy the folder [b]lml\InfiniteHair[/b] into [b]<game folder>\lml\[/b].
[*]If you use the LML Mod Manager, make sure "Infinite Hair" is enabled.
[*]Start story mode. Use [b]End[/b] to grow a step right away.
[/list]
Vortex: install as usual, the archive mirrors the game folder.

[size=4][b]Good to know[/b][/size]
[list]
[*]Extra-long hair is made for Arthur's [b]"swept back"[/b] style without pomade. Other styles grow to the normal maximum.
[*]The game doesn't accept new model names, so the long lengths [b]replace unused barber options[/b]: hair styles 07/08, the beard styles forked / handlebar / dundreary and John's beard (in the epilogue John wears the long beard too).
[*]Beards have no physics in RDR2 - a very long beard follows the head rigidly.
[*]Conflicts with mods that replace those hair/beard models.
[/list]

[size=4][b]Uninstall[/b][/size]
Delete InfiniteHair.asi, InfiniteHair.ini, InfiniteHair.state, InfiniteHair.log and the folder lml\InfiniteHair.

[size=4][b]Story mode only[/b][/size]
Never start Red Dead Online with script mods installed.

[size=4][b]Source code[/b][/size]
Open source (MIT): [url=https://github.com/shrouddeath/RDR2-InfiniteHair]github.com/shrouddeath/RDR2-InfiniteHair[/url]

[size=4][b]Credits[/b][/size]
[list]
[*]Alexander Blade - ScriptHookRDR2
[*]LMS / Lenny - Lenny's Mod Loader
[*]dexyfex - CodeX
[*]femga - rdr3_discoveries (drawable and bone names)
[*]alloc8or - RDR3 Native DB
[*]Sollumz team - Sollumz_RDR (XML format reference)
[/list]
```

## 5. Permissions (your choice – suggestion)

- Upload to other sites: ask me first
- Modify / use in other mods: yes, with credit (the code is MIT on GitHub)
- Convert to other games: no
- Asset use: the hair/beard models are edited Rockstar assets – credit Rockstar Games
- Commercial use: no (Rockstar does not allow paid mods)

## 6. Images

Use your own in-game screenshots (Steam F12), for example:
hair 9 vs 29 from behind, the beard at 29 from the front, riding with long hair.
Ready-made renders: `docs/laenge-16-20-28-29.png`, `docs/bart-10-17-23-29-vorne.png`,
`docs/arthur-9-vs-29-seite.png`.

## 7. GitHub

Repository name used in the links above: `shrouddeath/RDR2-InfiniteHair`. If you pick a
different name, change the link in `dist/README_release.txt`, `lml/InfiniteHair/install.xml`,
`README.md` and this page, then rebuild the zip with `tools/make_release.py`.
