#!/usr/bin/env python3
"""
Update category.ini and genre.ini for AdvanceMAME supported games.

This script:
1. Calls "advmame -listbare" to get the list of supported games in XML format.
2. Reads category.ini, catver.ini, and renameSET.dat from the current directory.
3. Adds all missing supported games (excluding bios and clones) to category.ini
   using values from catver.ini, resolving renamed games via renameSET.dat.
4. Outputs a message to stdout for any entry found in neither category.ini
   nor catver.ini (ignoring bios and clones).
5. Sorts category.ini entries alphabetically.
6. Generates genre.ini from category.ini matching the logic of UPDATE.sh.
"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET


def find_advmame(explicit_cmd=None):
    if explicit_cmd:
        return explicit_cmd
    env_cmd = os.environ.get("ADVMAME")
    if env_cmd:
        return env_cmd
    if shutil.which("advmame"):
        return "advmame"

    # Search in common build output directories
    script_dir = Path(__file__).resolve().parent
    candidates = [
        Path("obj/mame/linux/blend/advmame"),
        script_dir.parent / "obj/mame/linux/blend/advmame",
    ]
    for c in candidates:
        if c.is_file() and os.access(c, os.X_OK):
            return str(c)

    return "advmame"


def load_ini(file_path):
    mapping = {}
    if not file_path.is_file():
        return mapping
    with open(file_path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("[") or line.startswith("#") or line.startswith(";"):
                continue
            if "=" in line:
                key, val = line.split("=", 1)
                mapping[key.strip()] = val.strip()
    return mapping


def load_renames(file_path):
    renames = {}
    if not file_path.is_file():
        return renames
    with open(file_path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if ">" in line:
                parts = line.strip().split(">")
                old_name = parts[0].strip()
                new_part = parts[1].strip()
                if new_part:
                    new_name = new_part.split()[0]
                    if old_name not in renames:
                        renames[old_name] = []
                    if new_name not in renames[old_name]:
                        renames[old_name].append(new_name)
    return renames


def find_category(game, catver_map, renames):
    if game in catver_map:
        return catver_map[game], game
    # Breadth-first search through renameSET chains
    queue = list(renames.get(game, []))
    visited = set(queue)
    visited.add(game)
    while queue:
        curr = queue.pop(0)
        if curr in catver_map:
            return catver_map[curr], curr
        for next_target in renames.get(curr, []):
            if next_target not in visited:
                visited.add(next_target)
                queue.append(next_target)
    return None, None


def main():
    parser = argparse.ArgumentParser(
        description="Update category.ini and genre.ini from catver.ini for supported games."
    )
    parser.add_argument(
        "--category",
        type=Path,
        default=Path("category.ini"),
        help="Path to category.ini (default: category.ini in current directory)",
    )
    parser.add_argument(
        "--catver",
        type=Path,
        default=Path("catver.ini"),
        help="Path to catver.ini (default: catver.ini in current directory)",
    )
    parser.add_argument(
        "--genre",
        type=Path,
        default=Path("genre.ini"),
        help="Path to genre.ini output (default: genre.ini in current directory)",
    )
    parser.add_argument(
        "--rename",
        type=Path,
        default=Path("renameSET.dat"),
        help="Path to renameSET.dat (default: renameSET.dat in current directory)",
    )
    parser.add_argument(
        "--advmame",
        type=str,
        default=None,
        help="Path or command name for advmame (default: advmame)",
    )
    args = parser.parse_args()

    category_path = args.category
    catver_path = args.catver
    genre_path = args.genre
    rename_path = args.rename

    if not category_path.is_file():
        sys.exit(f"Error: '{category_path}' not found in current directory.")
    if not catver_path.is_file():
        sys.exit(f"Error: '{catver_path}' not found in current directory.")

    advmame_cmd = find_advmame(args.advmame)

    try:
        res = subprocess.run(
            [advmame_cmd, "-listbare"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except FileNotFoundError:
        sys.exit(f"Error: '{advmame_cmd}' executable not found.")
    except subprocess.CalledProcessError as e:
        sys.exit(f"Error executing '{advmame_cmd} -listbare': {e.stderr.strip()}")

    try:
        root = ET.fromstring(res.stdout)
    except ET.ParseError as e:
        sys.exit(f"Error parsing XML output from '{advmame_cmd} -listbare': {e}")

    supported_games = []
    seen = set()
    for game_elem in root.findall("game"):
        name = game_elem.get("name")
        if not name or name in seen:
            continue
        seen.add(name)

        is_clone = bool(game_elem.get("cloneof"))
        is_bios = (
            game_elem.get("isbios") == "yes"
            or game_elem.get("runnable") == "no"
            or game_elem.find("biosset") is not None
        )
        if is_clone or is_bios:
            continue

        supported_games.append(name)

    category_map = load_ini(category_path)
    catver_map = load_ini(catver_path)
    renames = load_renames(rename_path)

    added_count = 0
    missing_both = []

    for game in supported_games:
        if game not in category_map:
            category, target = find_category(game, catver_map, renames)
            if category:
                category_map[game] = category
                added_count += 1
                if target != game:
                    print(f"Adding '{game}' (renamed to '{target}'): {category}")
                else:
                    print(f"Adding '{game}': {category}")
            else:
                missing_both.append(game)
                print(f"Entry '{game}' not found in category.ini or catver.ini")

    # Sort entries alphabetically
    sorted_entries = sorted(f"{game}={cat}" for game, cat in category_map.items())

    # Write updated category.ini with CRLF line terminators
    with open(category_path, "w", encoding="utf-8", newline="") as f:
        f.write("[Category]\r\n")
        for entry in sorted_entries:
            f.write(f"{entry}\r\n")

    # Generate genre.ini matching UPDATE.sh logic
    with open(genre_path, "w", encoding="utf-8", newline="") as f:
        f.write("[Category]\r\n")
        for entry in sorted_entries:
            parts = entry.split("/", 1)
            part0 = parts[0].rstrip()
            is_mature = len(parts) > 1 and ("Mature" in parts[1])
            genre_line = part0 + (" * Mature *" if is_mature else "")
            f.write(f"{genre_line}\r\n")

    print(
        f"Updated {category_path}: {added_count} missing games added ({len(category_map)} total entries)."
    )
    print(f"Generated {genre_path}.")
    if missing_both:
        print(f"Total entries not found in category.ini or catver.ini: {len(missing_both)}")


if __name__ == "__main__":
    main()
