INFINITE HAIR 1.0.0 - Red Dead Redemption 2 (Story Mode)
by shrouddeath

Arthur's hair and beard keep growing in turbo speed - far beyond the game's
maximum. Hair grows down to the belt (length 29 instead of 9), the chin beard
too (29 instead of 10). Hair keeps its cloth physics.

STORY MODE ONLY. Never start Red Dead Online with script mods installed.

----------------------------------------------------------------------------
REQUIREMENTS
----------------------------------------------------------------------------
- ScriptHookRDR2 by Alexander Blade (version matching your game)
  http://www.dev-c.com/rdr2/scripthookrdr2/
- Lenny's Mod Loader (LML)
  https://www.rdr2mods.com/downloads/rdr2/tools/76-lennys-mod-loader-rdr/

----------------------------------------------------------------------------
INSTALLATION
----------------------------------------------------------------------------
Game folder = the folder that contains RDR2.exe

1. Copy InfiniteHair.asi and InfiniteHair.ini into the game folder.
2. Copy the folder lml\InfiniteHair into <game folder>\lml\
   (so you get <game folder>\lml\InfiniteHair\install.xml).
3. If you use the LML Mod Manager, make sure "Infinite Hair" is enabled.
4. Start the game in story mode. InfiniteHair.log appears in the game folder.

----------------------------------------------------------------------------
USAGE
----------------------------------------------------------------------------
Hair and beard grow one step every 30 seconds (GrowIntervalSeconds).
  F10     pause / resume
  End     grow one step right now
  Delete  test: cut everything to the shortest length
  F9      write all worn parts to InfiniteHair.log (for bug reports)
All settings are in InfiniteHair.ini (Language=de for German messages).
To start over, delete InfiniteHair.state while the game is closed.

----------------------------------------------------------------------------
GOOD TO KNOW
----------------------------------------------------------------------------
- Extra-long hair works for Arthur's "swept back" hairstyle without pomade.
  Other styles grow up to the game's normal maximum.
- The long lengths replace unused barber options: hair styles 07/08,
  the beard styles "forked" / "handlebar" / "dundreary" and John's beard
  (in the epilogue John wears the long beard too). Choosing them at the
  barber shows the long versions.
- LockBarber=1 (default) undoes haircuts and shaves at once. Set it to 0
  if you want to cut and let it grow back.
- Very long beards follow the head rigidly (the game has no beard physics),
  so looking down can push the beard into the chest.
- Conflicts: other mods that replace Arthur's hair styles 07/08, his
  beard styles 001 or John's beard models.

----------------------------------------------------------------------------
UNINSTALL
----------------------------------------------------------------------------
Delete InfiniteHair.asi, InfiniteHair.ini, InfiniteHair.state,
InfiniteHair.log and the folder lml\InfiniteHair.

----------------------------------------------------------------------------
DEUTSCH (KURZ)
----------------------------------------------------------------------------
Voraussetzungen: ScriptHookRDR2 und Lenny's Mod Loader (Links oben).
1. InfiniteHair.asi und InfiniteHair.ini in den Spielordner (wo RDR2.exe liegt).
2. Den Ordner lml\InfiniteHair nach <Spielordner>\lml\ kopieren.
3. In InfiniteHair.ini "Language=de" setzen fuer deutsche Meldungen.
Tasten: F10 Pause, Ende sofort wachsen, Entf Test-Kuerzen, F9 Log.
Nur Story-Modus - Red Dead Online nie mit Script-Mods starten.

----------------------------------------------------------------------------
CREDITS
----------------------------------------------------------------------------
- Alexander Blade - ScriptHookRDR2
- LMS / Lenny - Lenny's Mod Loader
- dexyfex - CodeX (model export/import)
- femga - rdr3_discoveries (drawable and bone names)
- alloc8or - RDR3 Native DB
- Sollumz team - Sollumz_RDR (XML format reference)
Source code: https://github.com/shrouddeath/RDR2-InfiniteHair
