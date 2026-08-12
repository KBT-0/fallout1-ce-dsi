# Phase 0C memory probe

The unified benchmark now parses user-supplied Fallout 1 DAT catalogs and
loads a simultaneous representative asset payload without bundling data. It
records platform free heap and largest allocatable block around:

- ORIGINAL archive catalogs;
- representative map, script list and bytecode, critter/item prototypes,
  localized message, palette, interface art and critter art;
- the corresponding TURKISH payload when supplied;
- payload unload.

These checkpoints are real-data lower bounds, not an engine-runtime working
set. Objects, parsed map structures, interpreter state, art-cache policy,
audio decoding, save/load spikes, and gameplay exploration remain unknown
until the engine reaches a representative map. The log deliberately uses
`REAL_ASSET_PAYLOAD_LOWER_BOUND` category names and retains separate
`RUNTIME_STATUS` fields to prevent the two measurements from being conflated.

The benchmark's STL/catalog allocations are visible in platform heap figures.
Buffers routed through `trackedAlloc` also appear in tracked counters. The
authoritative value is the simultaneous platform peak and smallest largest
allocatable block measured on a real DSi; emulator values are diagnostic only.
