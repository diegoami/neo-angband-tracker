# Neo Angband beta.4 adoption

Task: https://github.com/diegoami/neo-angband-tracker/issues/641

Verified GitHub release tag and npm Core gitHead:
`a0f252de23eb672e2ccf12e0377a96088c66e24d` (`1.23.0-beta.4`).
Latest edge is `v1.23.0-edge.241`, a separate prerelease; select the numbered beta.
Official C remains at recorded pin `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6`.

Owned base `0c00e7761`; normal two-parent merge
`dc1093b3731f8e452fd10c2299577dddd77768ee`. The only previously unimported
release commit is the version commit. The 14-file merge diff contains no new
workflow or runtime behavior, aside from the two exported version literals.
No conflict resolution was needed. Existing stock-order and starting-kit
corrections, historical evidence, published topics and tags are preserved.
No product tag was imported or pushed.

Local validation under `flock /tmp/angband-validation.lock`, Node26.13.0,
pnpm12.3.4:

- Frozen install and `pnpm build`: pass.
- `node tools/version.mjs`: pass (14 aligned sites).
- Session identity check: pass.
- Fork workflow guard checker: pass, all11 inherited jobs restricted upstream.
- Focused Vitest: boot-save-gate, store/store, player/bind: 53 tests / 3 files
  pass, zero skips. The upstream boot-save correction `50bb7ce50` is already
  an ancestor of the owned base; this is verification of imported code, not
  a new patch or a claim of controller-specific GUI reproduction.
- Installer changed only executable modes on two tracked SDK bin scripts;
  original100644 modes restored before commit.

Full hosted CI, package/parity/MCP checks and production desktop smoke are
required before merge; live PR checks are authoritative. No local duplicate
full suite is launched. No consumer dependency or running lab is changed.
Historical beta.3 receipts retain their original measurements and boundaries.
