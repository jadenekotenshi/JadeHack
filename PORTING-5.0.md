# Porting NetHack 3.6.x / 5.0 fixes into JadeHack

Source of truth: NetHack 5.0.0 tag `NetHack-5.0.0_Released`, files
`doc/fixes3-6-0.txt` ... `doc/fixes3-6-7.txt` and `doc/fixes5-0-0.txt`.
The 5.0 notes only cover changes since 3.6.7; a 3.4.3-based tree also needs
the 3.6.x notes.

This file tracks the **crash / memory-safety batch** (217 entries matched by
keyword). Status codes: VERIFIED = code read in this tree; LIKELY = description
matches code that exists here, not yet read; N/A = code absent or disabled here.

## Verified present in this tree: PORTED (macOS build OK; escapes() ASan-tested)

| Fix (source file) | Where here | Notes |
|---|---|---|
| pline() buffer overflow on long messages (3.6.0, 3.6.5) | `src/pline.c` `Vsprintf(pbuf, ...)` into `char pbuf[BUFSZ]`; 3 unbounded `Vsprintf` calls | Use `vsnprintf` where available, else bound the format. Solaris 10 has `vsnprintf`. |
| Option escape handling overrun (3.6.0) | `src/options.c` `escapes()` | Writes into `tp` with no bound; `\\` at end of string reads past NUL (`cp[1]`). |
| `docall` prompt overflow with long type names (3.6.1) | `src/do_name.c` `Sprintf(qbuf, "Call %s:", an(xname(...)))` into `QBUFSZ` | Truncate with `strncpy`/`Snprintf` style bound. |

## Batch 2: PORTED (macOS build OK)

| Fix | Where here |
|---|---|
| Prayer: `Luck` dropping below -1 mid-prayer gave `rn1(0,...)` divide by zero (3.6.0) | `src/pray.c` `pleased()` now uses `max(Luck, -1)` |
| Bullwhip at map edge targeted outside the map (3.6.0) | `src/apply.c` `use_whip()` checks `isok(rx, ry)` |
| Eating a generic egg indexed `mons[-1]` (5.0, pr#547) | `src/eat.c` `fpostfx()` requires `corpsenm >= LOW_PM` |
| Applying a wielded cream pie left a dangling `uwep` (3.6.0) | `src/apply.c` `use_cream_pie()` calls `setnotworn()` |

Checked and found N/A here: levitation boots over a sink (3.6 sink change),
`gold_detect` NULL check (no NULL check exists to misorder), `#untrap`
divide by zero (5.0 itself says it can't happen).

## Batch 3: PORTED (macOS build OK)

| Fix | Where here |
|---|---|
| `O` command: "Set <option> to what?" answer overflowed `buf` (3.6.3) | `src/options.c` `doset()` uses `Snprintf` |
| `windowcolors` option value copied unbounded into `BUFSZ` (3.6.x config overflow family) | `src/options.c` `wc_set_window_colors()` bounded `strncpy` |
| Autopickup exception pattern plus added quotes could overrun `apebuf` (3.6.x) | `src/options.c` `apebuf` has room for the quotes |

Checked and found N/A or already handled: acid potion into water (this tree
already tracks `obj_destroyed`), `topten` long-name loop (SporkHack has its own
workaround), `windowtype` copy (uses bounded `nmcpy`), config file line reading
(`fgets` is bounded).

Open: vault guard `fakecorr overflow`. The 5.0 and 3.4.3 path code differs in
a few places (pool check, engraving cleanup) but I could not identify which
change fixes the overflow, so nothing was ported.

Still unread: burning-oil "object lost" panics, aklys `thrownobj`, helm of
opposite alignment `uarmh`, multishot freed monster, shopkeeper revival,
rolling boulder.

## N/A because SporkHack changed or removed the code

| Fix | Reason |
|---|---|
| All `cutworm` fixes (3.6.0, 3.6.2, 3.6.3) | `cutworm()` in `src/worm.c` starts with an unconditional `return;`, so long worms can't be cut. |
| `morguemon` NON_PM dereference (3.6.0) | No `morguemon` function with the 3.6 `ndemon` logic in this tree. |

## LIKELY applicable (read code before porting)

Prayer and Luck divide by zero; bullwhip at map edge; applying a wielded cream
pie; thrown potion hitting iron bars; potion of acid dropped into water (freed
memory); "object lost" panics from burning potions of oil, levitation or water
walking boots, polymorph and wielded potions; levitation boots on a sink; shop
and shopkeeper fixes (corpse revival, stolen object outside never-entered
shop, split bills); vault guard "fakecorr overflow"; aklys `thrownobj` stale
pointer; helm of opposite alignment `uarmh` null; multishot volley freed
monster; inventory-slot overflow cases; Plane of Water bmask; `#untrap` divide
by zero; egg `fpostfx` index; farlook `xdir`/`ydir` bounds; `gold_detect` null
check; rolling boulder trap segfault; long-name `topten` loop; config-file long
line and `O` option overflows; `data.base` lookup edge cases.

## N/A: code absent in 3.4.3 or not applicable to a Unix tty/curses build

sortloot, perm_invent, status highlights, sysconf and PANICTRACE, Lua and themed
rooms, `sanity_check`, EXTRA_SANITY_CHECKS, Qt, X11, win32, Amiga and DOS ports,
3.6+ features (zombies, `#terrain`, `#therecmdmenu`, repeat-after-prefix, split
stack `w`). Curses entries need a separate look because `win/curses` exists here
but is a different port than 5.0's.

## Next steps

1. Port the three VERIFIED fixes, rebuild on macOS, smoke test.
2. Read code for each LIKELY item, move it to VERIFIED or N/A, port in batches.
3. (Done: the three VERIFIED fixes below.)
4. Only then triage the non-crash bug fixes in the same notes files.
