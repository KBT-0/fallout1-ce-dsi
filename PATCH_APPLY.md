# Apply this patch to the MrHuu Fallout 1 CE 3DS fork

Copy/merge the package into the **repository root** so these paths exist:

```text
Makefile.dsi
.github/workflows/dsi-size.yml
scripts/ci-dsi-size.sh
src/platform/dsi/runtime/include/SDL.h
src/platform/dsi/runtime/dsi_sdl.cc
src/platform/dsi/feasibility/README.md
```

Do not place the package one directory above the repo.

## First CI run

Commit and push, then run:

```text
DSi Phase 0 size feasibility
```

The first build is allowed to fail.

Download and return the entire artifact named like:

```text
dsi-phase0-size-<commit-sha>
```

Especially preserve:

```text
build/ci/environment.txt
build/ci/build.log
build/ci/build-exit-code.txt
build/ci/size.txt
build/ci/sdl-symbols.txt
build/ci/sdl-call-sites.txt
build/ci/maps/**
build/ci/*.elf
```

## Local Docker equivalent

From repository root:

```bash
docker pull devkitpro/devkitarm:20260610

docker run --rm \
  -v "$PWD":/work \
  -w /work \
  devkitpro/devkitarm:20260610 \
  bash scripts/ci-dsi-size.sh
```

The script attempts to install `nds-dev` if libnds/ndstool are absent and `nds-zlib` if the DS zlib portlib is missing.
