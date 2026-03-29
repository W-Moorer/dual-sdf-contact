# third_party

This directory is intentionally kept isolated from `src/`, but it is no longer always empty.

- `third_party/vcpkg/` is the default clone location used by `scripts/bootstrap_windows.ps1`.
- `third_party/_deps/openvdb/` may be populated by `scripts/bootstrap_wsl.sh` when Ubuntu package extraction is used for OpenVDB headers.
- Future source snapshots or vendored adapters should stay isolated here instead of leaking into `src/`.
