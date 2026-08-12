# Turkish data validation

## Scope and asset policy

The repository and CI artifact contain no Fallout or translation assets. The
unified `dsi-bench.nds` reads only files that the owner places on the SD card.
ORIGINAL and TURKISH are different roots and different log namespaces so a
localization failure cannot be reported as a base DSi-port failure.

The execution order is enforced by the benchmark:

1. open and validate ORIGINAL `master.dat`, `critter.dat`, `data/`, and the
   optional `fallout.cfg`;
2. load representative ORIGINAL map, scripts, messages, palette and art;
3. if the TURKISH root exists, build the complete file-difference manifest;
4. inspect original and patched PE executable hashes and `.text` sections when
   both executables were supplied;
5. only then load TURKISH data and run its glyph test.

`fallout.cfg` is optional. When present, `[system] master_dat`, `critter_dat`,
`master_patches`, and `language` are honored when they are safe paths inside
that dataset root. Otherwise the clean-install defaults are used.

## SD-card layout

For the base test, use only:

```text
sd:/
  fallout1/
    dsi-bench.nds
    original/
      master.dat
      critter.dat
      data/
      fallout.cfg        optional
      falloutw.exe       optional, audit metadata only
```

After ORIGINAL has passed on its own, retain it unchanged and add:

```text
sd:/fallout1/turkish/
  master.dat
  critter.dat
  data/
  fallout.cfg            optional
  falloutw.exe           optional, audit metadata only
```

The optional executables are never executed. They are read only to determine
whether the patch changed the original program as well as assets. Supplying
both allows the log to distinguish an identical executable, a resource-only
change, and a changed `.text` section. A changed `.text` section proves that
Fallout CE needs patch-specific executable behavior analysis; hashes alone do
not justify guessing what that behavior is. The returned non-commercial log
is the input to that follow-up audit.

## Difference evidence

Before any TURKISH semantic load, `bench.log` records:

- logical files added, removed, or modified inside both DAT archives;
- recursively added, removed, or modified loose files, including `data/`,
  config, fonts, art, text, external loaders, and executables;
- whole-archive FNV-1a hashes;
- original/patched executable and PE `.text` hashes when supplied.

No asset bytes are copied to the log. Logical paths, change classifications,
whole-archive hashes, and executable hashes are sufficient to locate changes
without redistributing commercial content.

## Turkish glyph validation

Fallout's GNW `.fon` fonts and interface `.aaf` fonts are byte-indexed. The
probe checks the Windows-1254 slots in both formats for all required characters
and rejects missing, empty, malformed, or out-of-range glyph bitmaps:

```text
ç Ç ğ Ğ ı İ ö Ö ş Ş ü Ü
E7 C7 F0 D0 FD DD F6 D6 FE DE FC DC
```

Structural presence is not proof of the correct drawing. The benchmark
therefore renders the actual user-supplied `.fon` (white) and `.aaf` (green)
bitmaps on screen and asks the hardware tester to attest with `A` (correct) or
`X` (wrong). Structural and visual results remain under the `TURKISH_GLYPH`
log section.

## Interpretation

- `ASSET.ORIGINAL.STATUS=PASS` establishes only the real-data loading lower
  bound; it does not claim a running gameplay state.
- `ASSET.TURKISH.STATUS=FAIL` with ORIGINAL passing is a localization-specific
  failure.
- `EXECUTABLE_DIFF.TEXT_SECTION_CHANGED=YES` means the patch changed code and
  requires a local, patch-specific semantic audit before CE compatibility can
  be claimed.
- Emulator timing is never valid for GO/NO-GO.
