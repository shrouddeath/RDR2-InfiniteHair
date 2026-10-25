INFINITE HAIR 1.0.0 - Red Dead Redemption 2 (Story Mode)
by shrouddeath

Arthur's hair and beard grow just like in the vanilla game - they simply don't
stop at the game's maximum anymore. Hair grows down to the belt (length 29
instead of 9), the chin beard too (29 instead of 10). Hair keeps its cloth
physics. A hotkey lets you grow it yourself whenever you like.

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
Hair and beard grow naturally over in-game time, as in the vanilla game
(sleeping, waiting and travelling make time pass). Where the game stops -
at its maximum, or a beard without hair tonic - the mod keeps growing one
step every 48 in-game hours (HoursPerStep) up to the new maximum.
  End     grow one step right now (grow it yourself)
  F10     pause / resume
  Delete  cut everything to the shortest length
  F9      write all worn parts to InfiniteHair.log (for bug reports)
Barber and camp shaving work normally. GrowthMode=turbo makes it grow every
30 real seconds, LockBarber=1 undoes haircuts.
All settings are in InfiniteHair.ini.
To start over, delete InfiniteHair.state while the game is closed.

----------------------------------------------------------------------------
GOOD TO KNOW
----------------------------------------------------------------------------
- The extra-long hair is Arthur's "swept back" style without pomade. Any
  other style (also after a short barber cut) grows to its normal maximum
  and then continues as long swept back hair.
- The long lengths replace unused barber options: hair styles 07/08,
  the beard styles "forked" / "handlebar" / "dundreary" and John's beard
  (in the epilogue John wears the long beard too). Choosing them at the
  barber shows the long versions.
- When the game re-applies its own (shorter) length - e.g. changing outfits
  or mounting a horse with a hat - the mod restores the long hair at once.
- The mod's length is not stored per save game: loading an older save keeps
  the current length. Delete InfiniteHair.state to start over.
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
CREDITS
----------------------------------------------------------------------------
- Alexander Blade - ScriptHookRDR2
- LMS / Lenny - Lenny's Mod Loader
- dexyfex - CodeX (model export/import)
- femga - rdr3_discoveries (drawable and bone names)
- alloc8or - RDR3 Native DB
- Sollumz team - Sollumz_RDR (XML format reference)
Source code: https://github.com/shrouddeath/RDR2-InfiniteHair
