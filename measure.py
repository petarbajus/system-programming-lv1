"""Compile the copy programs and time them.

Windows rows use clock() (t2 - t1), which is what the lab asks to print.
wall_ms is elapsed time around the same copy, captured from stderr, and is
what the discussion uses when CPU time hides the disk wait.

Usage:
  python measure.py --cc path\\to\\clang.exe --sizes 1MB,1GB,5GB
"""

import argparse
import csv
import shutil
import statistics
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build"
DATA = ROOT / "data"
CSV_PATH = BUILD / "results.csv"

PROGRAMS = ["UW1", "UW2", "W1", "W2", "W3"]
BUFFERS = [256, 2048, 8192]
SIZES = {
    "1MB": 1 << 20,
    "1GB": 1 << 30,
    "5GB": 5 * (1 << 30),
}


def disk_free():
    return shutil.disk_usage(ROOT).free


def compile_one(cc, src, buf, dest):
    cmd = [
        cc,
        "-O2",
        "-std=c11",
        f"-DBUF_SIZE={buf}",
        "-o",
        str(dest),
        str(src),
    ]
    print("compile", " ".join(cmd), flush=True)
    r = subprocess.run(cmd, cwd=ROOT)
    if r.returncode != 0:
        raise SystemExit(f"compile failed: {src.name} BUF_SIZE={buf}")


def parse_run(proc):
    out = (proc.stdout or "").strip().splitlines()
    err = (proc.stderr or "").strip().splitlines()
    cpu = None
    wall = None
    for line in reversed(out):
        line = line.strip()
        if line.lstrip("-").isdigit():
            cpu = int(line)
            break
    for line in err:
        if line.startswith("wall_ms="):
            wall = int(line.split("=", 1)[1])
    return cpu, wall, out, err


def run_copy(exe, src, dst):
    if dst.exists():
        dst.unlink()
    proc = subprocess.run(
        [str(exe), str(src), str(dst)],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    cpu, wall, out, err = parse_run(proc)
    size = dst.stat().st_size if dst.exists() else -1
    ok = proc.returncode == 0 and size == src.stat().st_size and cpu is not None
    if dst.exists():
        dst.unlink()
    return {
        "ok": ok,
        "code": proc.returncode,
        "cpu_ms": cpu,
        "wall_ms": wall,
        "size": size,
        "stdout": " | ".join(out[-3:]),
        "stderr": " | ".join(err[-5:]),
    }


def median_run(rows):
    good = [r for r in rows if r["ok"]]
    if not good:
        return rows[-1]
    good.sort(key=lambda r: (r["cpu_ms"], r["wall_ms"] if r["wall_ms"] is not None else 0))
    return good[len(good) // 2]


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--cc", required=True)
    p.add_argument("--sizes", default="1MB,1GB,5GB")
    p.add_argument("--repeats-small", type=int, default=3)
    args = p.parse_args()

    cc = args.cc
    sizes = [s.strip() for s in args.sizes.split(",") if s.strip()]
    for s in sizes:
        if s not in SIZES:
            raise SystemExit(f"unknown size {s}")

    BUILD.mkdir(exist_ok=True)
    DATA.mkdir(exist_ok=True)

    gen = BUILD / "genfile.exe"
    compile_one(cc, ROOT / "genfile.c", 1024, gen)

    exes = {}
    for prog in PROGRAMS:
        for buf in BUFFERS:
            dest = BUILD / f"{prog}_{buf}.exe"
            compile_one(cc, ROOT / f"{prog}.c", buf, dest)
            exes[(prog, buf)] = dest

    new_file = not CSV_PATH.exists()
    with CSV_PATH.open("a", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(
            f,
            fieldnames=[
                "size", "program", "buf", "repeat", "ok", "cpu_ms", "wall_ms",
                "code", "bytes", "stdout", "stderr",
            ],
        )
        if new_file:
            w.writeheader()

        for size_name in sizes:
            nbytes = SIZES[size_name]
            need = nbytes * 2 + 2 * (1 << 30)
            free = disk_free()
            print(f"free={free} need>={need} for {size_name}", flush=True)
            if free < need:
                print(f"SKIP {size_name}: not enough free disk", flush=True)
                continue

            src = DATA / f"src_{size_name}.dat"
            dst = DATA / "dst.dat"
            print(f"generating {size_name}", flush=True)
            g = subprocess.run([str(gen), str(src), size_name], cwd=ROOT, capture_output=True, text=True)
            print(g.stderr.strip(), flush=True)
            if g.returncode != 0 or not src.exists() or src.stat().st_size != nbytes:
                print(f"generator failed for {size_name}: code={g.returncode} stderr={g.stderr}", flush=True)
                if src.exists():
                    src.unlink()
                continue

            repeats = args.repeats_small if nbytes <= (1 << 20) else 1
            print(f"warmup {size_name}", flush=True)
            warm = run_copy(exes[("W2", 8192)], src, dst)
            print(f"warmup ok={warm['ok']} wall={warm['wall_ms']} cpu={warm['cpu_ms']}", flush=True)

            for prog in PROGRAMS:
                for buf in BUFFERS:
                    trials = []
                    for rep in range(1, repeats + 1):
                        row = run_copy(exes[(prog, buf)], src, dst)
                        trials.append(row)
                        print(
                            f"{size_name} {prog} buf={buf} rep={rep} "
                            f"ok={row['ok']} cpu={row['cpu_ms']} wall={row['wall_ms']}",
                            flush=True,
                        )
                        w.writerow({
                            "size": size_name,
                            "program": prog,
                            "buf": buf,
                            "repeat": rep,
                            "ok": int(row["ok"]),
                            "cpu_ms": row["cpu_ms"],
                            "wall_ms": row["wall_ms"],
                            "code": row["code"],
                            "bytes": row["size"],
                            "stdout": row["stdout"],
                            "stderr": row["stderr"],
                        })
                        f.flush()
                    chosen = median_run(trials)
                    print(
                        f"CHOSEN {size_name} {prog} buf={buf} "
                        f"cpu={chosen['cpu_ms']} wall={chosen['wall_ms']} ok={chosen['ok']}",
                        flush=True,
                    )

            src.unlink(missing_ok=True)
            if dst.exists():
                dst.unlink()
            print(f"removed {size_name} source", flush=True)

    print("done", CSV_PATH, flush=True)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)
