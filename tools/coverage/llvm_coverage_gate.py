#!/usr/bin/env python3

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def run(command, env=None, capture=False):
    result = subprocess.run(
        command,
        env=env,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.STDOUT if capture else None,
        check=False,
    )
    if result.returncode != 0:
        if capture and result.stdout:
            sys.stderr.write(result.stdout)
        raise SystemExit(result.returncode)
    return result.stdout if capture else ""


def collect_sources(source_dirs):
    suffixes = {".cpp", ".h", ".hpp"}
    sources = []
    for source_dir in source_dirs:
        root = Path(source_dir).resolve()
        if root.is_file() and root.suffix in suffixes:
            sources.append(root)
            continue
        for path in root.rglob("*"):
            if path.is_file() and path.suffix in suffixes:
                sources.append(path.resolve())
    return sorted(set(sources))


def percent(summary, name):
    entry = summary.get(name, {})
    if entry.get("count", 0) == 0:
        return 100.0
    return float(entry.get("percent", 0.0))


def main():
    parser = argparse.ArgumentParser(description="Run LLVM coverage and enforce thresholds.")
    parser.add_argument("--llvm-cov", required=True)
    parser.add_argument("--llvm-profdata", required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--line-threshold", type=float, required=True)
    parser.add_argument("--branch-threshold", type=float, required=True)
    parser.add_argument("--test", action="append", required=True)
    parser.add_argument("--object", action="append", required=True)
    parser.add_argument("--source-dir", action="append", required=True)
    args = parser.parse_args()

    output_dir = Path(args.output_dir).resolve()
    profile_dir = output_dir / "profiles"
    if output_dir.exists():
        shutil.rmtree(output_dir)
    profile_dir.mkdir(parents=True)

    env = os.environ.copy()
    env["LLVM_PROFILE_FILE"] = str(profile_dir / "%m-%p.profraw")
    for test in args.test:
        run([test], env=env)

    profiles = sorted(profile_dir.glob("*.profraw"))
    if not profiles:
        raise SystemExit("No LLVM profile data was produced")

    profdata = output_dir / "coverage.profdata"
    run([args.llvm_profdata, "merge", "-sparse", *map(str, profiles), "-o", str(profdata)])

    objects = [str(Path(obj).resolve()) for obj in args.object]
    sources = collect_sources(args.source_dir)
    if not sources:
        raise SystemExit("No source files matched coverage source filters")

    export_command = [
        args.llvm_cov,
        "export",
        "--format=text",
        "--summary-only",
        f"--instr-profile={profdata}",
        objects[0],
    ]
    for obj in objects[1:]:
        export_command.append(f"--object={obj}")
    for source in sources:
        export_command.extend(["--sources", str(source)])

    coverage_json = run(export_command, capture=True)
    (output_dir / "coverage-summary.json").write_text(coverage_json, encoding="utf-8")
    data = json.loads(coverage_json)
    totals = data["data"][0]["totals"]
    line_percent = percent(totals, "lines")
    branch_percent = percent(totals, "branches")

    summary = (
        f"Line coverage: {line_percent:.2f}% (required {args.line_threshold:.2f}%)\n"
        f"Branch coverage: {branch_percent:.2f}% (required {args.branch_threshold:.2f}%)\n"
    )
    (output_dir / "coverage-summary.txt").write_text(summary, encoding="utf-8")
    sys.stdout.write(summary)

    if line_percent < args.line_threshold or branch_percent < args.branch_threshold:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
