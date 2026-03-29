from __future__ import annotations

import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from .run_example import project_root


FREEZE_SPEC: dict[str, Any] = {
    "freeze_name": "paper_experiment_freeze_v1",
    "recommended_backends": {
        "sdf": "openvdb",
        "reference": "fcl",
        "solver": "simple",
    },
    "recommended_voxel_sizes": {
        "coarse": 0.16,
        "medium": 0.08,
        "fine": 0.04,
    },
    "recommended_narrow_band_half_widths": {
        "tight": 2.0,
        "default": 4.0,
        "wide": 8.0,
    },
    "main_default_seed": 17,
    "suite_recommendations": {
        "main_text": "paper_minimal",
        "appendix": "paper_extended",
        "available": ["paper_minimal", "paper_extended"],
    },
}


def _timestamp_utc() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def _git_output(root: Path, args: list[str]) -> str | None:
    completed = subprocess.run(
        ["git", *args],
        cwd=root,
        capture_output=True,
        text=True,
        check=False,
    )
    if completed.returncode != 0:
        return None
    return completed.stdout.strip()


def git_metadata(root: Path) -> dict[str, Any]:
    commit_hash = _git_output(root, ["rev-parse", "HEAD"])
    short_hash = _git_output(root, ["rev-parse", "--short", "HEAD"])
    status_output = _git_output(root, ["status", "--porcelain"])
    return {
        "available": commit_hash is not None,
        "commit_hash": commit_hash or "",
        "commit_short": short_hash or "",
        "dirty": bool(status_output),
        "status_summary": status_output.splitlines()[:32] if status_output else [],
    }


def build_freeze_manifest(
    *,
    suite_name: str,
    run_name: str,
    benchmark_name: str | None = None,
    benchmark_dirs: list[Path] | None = None,
) -> dict[str, Any]:
    root = project_root()
    manifest: dict[str, Any] = {
        **FREEZE_SPEC,
        "generated_at_utc": _timestamp_utc(),
        "project_root": str(root),
        "suite_name": suite_name,
        "run_name": run_name,
        "git": git_metadata(root),
    }
    if benchmark_name is not None:
        manifest["benchmark_name"] = benchmark_name
    if benchmark_dirs:
        manifest["benchmark_dirs"] = [str(path) for path in benchmark_dirs]
    return manifest


def _read_json(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {}
    with path.open("r", encoding="utf-8") as handle:
        payload = json.load(handle)
    if isinstance(payload, dict):
        return payload
    return {"payload": payload}


def _write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def annotate_benchmark_output(benchmark_dir: Path, *, suite_name: str, run_name: str, benchmark_name: str) -> dict[str, Any]:
    manifest = build_freeze_manifest(suite_name=suite_name, run_name=run_name, benchmark_name=benchmark_name)
    for file_name in ("config_resolved.json", "environment.json"):
        path = benchmark_dir / file_name
        payload = _read_json(path)
        payload["experiment_freeze"] = manifest
        _write_json(path, payload)
    _write_json(benchmark_dir / "freeze_manifest.json", manifest)
    return manifest


def annotate_suite_output(suite_root: Path, *, suite_name: str, run_name: str, benchmark_dirs: list[Path]) -> dict[str, Any]:
    manifest = build_freeze_manifest(
        suite_name=suite_name,
        run_name=run_name,
        benchmark_dirs=benchmark_dirs,
    )
    _write_json(suite_root / "freeze_manifest.json", manifest)
    _write_json(project_root() / "outputs" / "latest_freeze_manifest.json", manifest)
    return manifest
