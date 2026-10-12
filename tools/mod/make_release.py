"""Package the patch kit as a zip for players: double-click "Patch Vagrant Story.bat", no setup.

The kit's original game bytes are replaced by SHA-1 hashes (patch_image.py checks those), so the
zip holds only the mod's own bytes. An official embeddable Python (python.org, PSF licence,
free to redistribute) goes in files/python.

usage: make_release.py <kit dir> <python-3.x-embed-amd64.zip> <version> <out dir>
  e.g. make_release.py build/mod/kit python-3.13.5-embed-amd64.zip 2026-10-12 build/release
"""

import hashlib
import json
import os
import shutil
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
NAME = "Twos-A-Riot-Merlose"
SCRIPTS = ["patch_disc.py", "patch_image.py", "apply_ppf.py"]


def text(src, dst, version=None):
    """Copy a text file with Windows line endings (Notepad and cmd.exe want CRLF)."""
    data = open(src, encoding="utf-8").read().replace("\r\n", "\n")
    if version:
        data = data.replace("@VERSION@", version)
    open(dst, "w", encoding="utf-8", newline="\r\n").write(data)


def main():
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    kit_dir, python_zip, version, out_dir = sys.argv[1:]
    stage = os.path.join(out_dir, NAME)
    shutil.rmtree(stage, ignore_errors=True)
    files = os.path.join(stage, "files")
    os.makedirs(os.path.join(files, "kit"))

    for name in ("Patch Vagrant Story.bat", "README.txt", "Merlose options.ini"):
        text(os.path.join(HERE, "release", name), os.path.join(stage, name), version)
    for name in SCRIPTS:
        shutil.copy(os.path.join(HERE, name), files)

    kit = json.load(open(os.path.join(kit_dir, "patches.json")))
    groups = [kit["files"]] + list(kit.get("features", {}).values())
    for runs in (runs for files in groups for runs in files.values()):
        for run in runs:
            run["original_sha1"] = hashlib.sha1(bytes.fromhex(run.pop("original"))).hexdigest()
    with open(os.path.join(files, "kit", "patches.json"), "w") as f:
        json.dump(kit, f, indent=1)
    for entry in kit["added"]:
        shutil.copy(os.path.join(kit_dir, entry["file"]), os.path.join(files, "kit"))

    with zipfile.ZipFile(python_zip) as z:
        z.extractall(os.path.join(files, "python"))

    out_zip = os.path.join(out_dir, f"{NAME}-{version}.zip")
    with zipfile.ZipFile(out_zip, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for root, _, names in os.walk(stage):
            for name in sorted(names):
                path = os.path.join(root, name)
                z.write(path, os.path.relpath(path, out_dir))
    print(f"{out_zip}: {os.path.getsize(out_zip) >> 10} KB")


if __name__ == "__main__":
    main()
