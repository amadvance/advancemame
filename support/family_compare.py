#!/usr/bin/env python3
"""
family_compare.py - Compare game families between AdvanceMAME and MAME.

This tool executes 'advmame -listxml' and 'mame -listxml' found in PATH,
streaming their XML outputs directly via pipes in O(1) memory to compare game
families across emulator generations (MAME 0.106 baseline vs modern MAME).
MAME machines are automatically filtered using 'support/external/catver.ini' and
arcade heuristics (excluding isdevice, isbios, ismechanical, non-runnables, and
preliminary status drivers) to focus strictly on working arcade games.

It identifies sets by their "Key ROM" (largest dumped ROM by size, breaking ties
with the lowest hexadecimal CRC) to accurately track:
  - Parent promotions / inversions (former clone promoted to parent).
  - Parent renames (shortname changed, verified by matching Key ROM).
  - Parent ROM set modifications and version/region swaps.
  - Clone renames, newly added clones, and removed/transferred clones.
  - Driver source files for both emulators (per-family and consolidated summary).

COMMAND-LINE OPTIONS:
---------------------
  -h, --help
      Show help message and exit.

  --game <name>
      Filter and inspect only the family containing the specified game shortname
      (e.g. '--game ddragon', '--game puckman'). Works whether <name> is a parent
      or a clone.

  --all
      Show all matching game families, including those that are 100% identical
      (by default, only families with detected differences are displayed).

  --common-only
      Show only families present in both emulators (suppressing AdvanceMAME-only
      and MAME-only families).

  --only-advmame
      Show only families present in AdvanceMAME and absent in MAME.

  --only-mame
      Show only families present in MAME and absent in AdvanceMAME.

  --parent-diff-only
      Show only families where the parent set differs between AdvanceMAME and
      MAME (name change, clone promotion/inversion, or modified ROM set).

  --rom-diff-only
      Show only families where the parent ROM set has changed.

  --full-romset
      Compare the entire ROMSET (all ROM hashes, added/removed/re-dumped files)
      instead of only comparing the Key ROM (the default behavior).

  --summary-only
      Suppress detailed per-family output and display only the final statistical
      summary.

USAGE EXAMPLES:
---------------
  # Standard comparison (all differences, AdvanceMAME-only, MAME-only + summary):
  ./family_compare.py

  # Inspect a specific game family:
  ./family_compare.py --game ddragon

  # Show only families exclusive to AdvanceMAME:
  ./family_compare.py --only-advmame

  # Show only families exclusive to MAME:
  ./family_compare.py --only-mame

  # Check differences across the entire ROMSET (not just Key ROM):
  ./family_compare.py --full-romset

  # Show only parent differences:
  ./family_compare.py --parent-diff-only

  # Quick summary of all differences across the entire sets:
  ./family_compare.py --summary-only
"""

import sys
import os
import shutil
import subprocess
import time
import argparse
import xml.etree.ElementTree as ET
from collections import defaultdict


def find_executables():
    """Verify presence of advmame and mame in PATH."""
    advmame_path = shutil.which("advmame")
    mame_path = shutil.which("mame")

    if not advmame_path:
        sys.stderr.write("ERROR: Could not find 'advmame' in PATH.\n")
        sys.exit(1)
    if not mame_path:
        sys.stderr.write("ERROR: Could not find 'mame' in PATH.\n")
        sys.exit(1)

    return advmame_path, mame_path


class GameInfo:
    __slots__ = (
        "name",
        "sourcefile",
        "description",
        "category",
        "cloneof",
        "romof",
        "key_rom",
        "rom_count",
        "total_rom_size",
        "rom_hashes",
    )

    def __init__(self, name, sourcefile, description, category, cloneof, romof, key_rom, rom_count, total_rom_size, rom_hashes):
        self.name = name
        self.sourcefile = sourcefile
        self.description = description
        self.category = category
        self.cloneof = cloneof
        self.romof = romof
        self.key_rom = key_rom  # (size, crc_lower) or None
        self.rom_count = rom_count
        self.total_rom_size = total_rom_size
        self.rom_hashes = rom_hashes  # set of (size, crc_lower)


NON_ARCADE_CATEGORIES = {
    "Slot Machine",
    "System",
    "Electromechanical",
    "Computer",
    "Computer Graphic Workstation",
    "Gambling",
    "Handheld",
    "Tabletop",
    "Game Console",
    "Board Game",
    "Quiz",
    "Musical Instrument",
    "Musical Instrument Accessory",
    "Medal Game",
    "Utilities",
    "Calculator",
    "Redemption Game",
    "Telephone",
    "Whac-A-Mole",
    "Music Player",
    "Printer",
    "Player",
    "Touchscreen",
    "Card Games",
    "Tablet",
    "Watch",
    "Casino",
    "Medical Equipment",
    "Non Arcade",
    "Digital Camera",
    "Robot",
    "Road Indicator",
    "Radio",
    "Luna Park",
    "Digital Simulator",
    "TV Bundle",
    "TTL * Quiz",
    "Misc.",
}


def load_catver_dict(ini_path=None):
    """
    Load mapping {shortname: category} from catver.ini (or category.ini fallback).
    """
    candidates = []
    if ini_path:
        candidates.append(ini_path)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidates.extend([
        os.path.join(script_dir, "external", "catver.ini"),
        os.path.join(script_dir, "catver.ini"),
        os.path.join(script_dir, "category.ini"),
    ])

    found_path = None
    for c in candidates:
        if os.path.isfile(c):
            found_path = c
            break

    if not found_path:
        sys.stderr.write("WARNING: No catver.ini found. Categories and arcade filtering will be disabled.\n")
        return {}

    mapping = {}
    with open(found_path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith(";") or line.startswith("#") or line.startswith("["):
                continue
            if "=" in line:
                k, v = line.split("=", 1)
                mapping[k.strip()] = v.strip()

    # Also load support/category.ini as fallback for any custom sets not in catver.ini
    cat_fallback = os.path.join(script_dir, "category.ini")
    if os.path.isfile(cat_fallback) and cat_fallback != found_path:
        with open(cat_fallback, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith(";") or line.startswith("#") or line.startswith("["):
                    continue
                if "=" in line:
                    k, v = line.split("=", 1)
                    k = k.strip()
                    if k not in mapping:
                        mapping[k] = v.strip()

    return mapping


def parse_listxml_stream(cmd, is_advmame=False, catver=None):
    """
    Executes command via subprocess and parses XML stream with iterparse in O(1) memory.
    Returns dictionary: {shortname: GameInfo}.
    """
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    games = {}

    target_tag = "game" if is_advmame else "machine"

    try:
        context = ET.iterparse(proc.stdout, events=("start", "end"))
        context = iter(context)
        event, root = next(context)

        for event, elem in context:
            if event == "end" and elem.tag == target_tag:
                # In modern MAME, discard devices, BIOS, mechanical, non-runnables, and preliminary drivers
                if not is_advmame:
                    if elem.attrib.get("isdevice") == "yes":
                        root.clear()
                        continue
                    if elem.attrib.get("isbios") == "yes":
                        root.clear()
                        continue
                    if elem.attrib.get("ismechanical") == "yes":
                        root.clear()
                        continue
                    if elem.attrib.get("runnable") == "no":
                        root.clear()
                        continue

                    drv = elem.find("driver")
                    if drv is not None and drv.attrib.get("status") == "preliminary":
                        root.clear()
                        continue

                name = elem.attrib.get("name")
                if not name:
                    root.clear()
                    continue

                category = catver.get(name, "") if catver else ""
                if not is_advmame and category:
                    top_cat = category.split("/")[0].strip()
                    if top_cat in NON_ARCADE_CATEGORIES:
                        root.clear()
                        continue

                sourcefile = elem.attrib.get("sourcefile", "").strip()
                cloneof = elem.attrib.get("cloneof")
                romof = elem.attrib.get("romof")

                desc_elem = elem.find("description")
                description = desc_elem.text.strip() if (desc_elem is not None and desc_elem.text) else ""

                valid_roms = []
                total_size = 0
                rom_hashes = set()

                for rom in elem.findall("rom"):
                    status = rom.attrib.get("status", "good")
                    if status == "nodump":
                        continue

                    size_str = rom.attrib.get("size")
                    crc_str = rom.attrib.get("crc")
                    if not size_str or not crc_str:
                        continue

                    try:
                        size = int(size_str)
                    except ValueError:
                        continue

                    if size <= 0:
                        continue

                    crc = crc_str.strip().lower()
                    valid_roms.append((size, crc))
                    total_size += size
                    rom_hashes.add((size, crc))

                # Key ROM calculation:
                # Largest ROM (by size). In case of tie, lowest hexadecimal CRC.
                if valid_roms:
                    key_rom = min(valid_roms, key=lambda r: (-r[0], r[1]))
                else:
                    key_rom = None

                games[name] = GameInfo(
                    name=name,
                    sourcefile=sourcefile,
                    description=description,
                    category=category,
                    cloneof=cloneof,
                    romof=romof,
                    key_rom=key_rom,
                    rom_count=len(valid_roms),
                    total_rom_size=total_size,
                    rom_hashes=rom_hashes,
                )

                root.clear()

        proc.stdout.close()
        proc.wait()
    except Exception as e:
        proc.kill()
        raise e

    return games


def build_families(games):
    """
    Group games into families.
    Each game without cloneof (or pointing to itself) is a parent.
    Games with cloneof are resolved to their root parent.
    Returns:
      families: {parent_name: {'parent': parent_name, 'clones': set([clone_names...])}}
      game_to_family: {game_name: parent_name}
    """
    game_to_family = {}
    families = defaultdict(lambda: {"parent": None, "clones": set()})

    for name, info in games.items():
        curr = info
        visited = {name}
        # Resolve cloneof chains
        while curr.cloneof and curr.cloneof in games and curr.cloneof not in visited:
            visited.add(curr.cloneof)
            curr = games[curr.cloneof]

        parent_name = curr.name
        game_to_family[name] = parent_name
        families[parent_name]["parent"] = parent_name
        if name != parent_name:
            families[parent_name]["clones"].add(name)

    return dict(families), game_to_family


def main():
    parser = argparse.ArgumentParser(
        description="Compare game families between AdvanceMAME and MAME searching 'advmame' and 'mame' in PATH."
    )
    parser.add_argument(
        "--game",
        type=str,
        help="Filter and inspect only the family containing the specified game (shortname).",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Show all matching families, including 100%% identical ones.",
    )
    parser.add_argument(
        "--common-only",
        action="store_true",
        help="Show only families present in both emulators (suppressing AdvanceMAME-only and MAME-only).",
    )
    parser.add_argument(
        "--only-advmame",
        action="store_true",
        help="Show only families present in AdvanceMAME and absent in MAME.",
    )
    parser.add_argument(
        "--only-mame",
        action="store_true",
        help="Show only families present in MAME and absent in AdvanceMAME.",
    )
    parser.add_argument(
        "--parent-diff-only",
        action="store_true",
        help="Show only families where the parent differs (name, inversion, or ROM changed).",
    )
    parser.add_argument(
        "--rom-diff-only",
        action="store_true",
        help="Show only families where the parent ROM has changed.",
    )
    parser.add_argument(
        "--full-romset",
        action="store_true",
        help="Compare the entire ROMSET (all ROM hashes) instead of only the Key ROM.",
    )
    parser.add_argument(
        "--summary-only",
        action="store_true",
        help="Show only the final summary statistics.",
    )

    args = parser.parse_args()

    advmame_bin, mame_bin = find_executables()

    print("[*] Using binaries found in PATH:")
    print(f"    AdvanceMAME : {advmame_bin}")
    print(f"    MAME        : {mame_bin}\n")

    # Load game categories from catver.ini (with category.ini fallback)
    catver = load_catver_dict()
    if catver:
        print(f"[*] Loaded {len(catver):,} game categories from catver.ini.\n")

    # 1. Parsing AdvanceMAME
    print("[1/4] Running 'advmame -listxml' in streaming...", flush=True)
    t0 = time.time()
    adv_games = parse_listxml_stream([advmame_bin, "-listxml"], is_advmame=True, catver=catver)
    t_adv = time.time() - t0
    print(f"      -> {len(adv_games):,} games parsed in {t_adv:.2f}s.\n", flush=True)

    # 2. Parsing MAME
    print("[2/4] Running 'mame -listxml' in streaming (filtered by catver.ini and arcade heuristics)...", flush=True)
    t0 = time.time()
    mame_games = parse_listxml_stream([mame_bin, "-listxml"], is_advmame=False, catver=catver)
    t_mame = time.time() - t0
    print(f"      -> {len(mame_games):,} arcade machines parsed in {t_mame:.2f}s.\n", flush=True)

    # 3. Grouping families
    print("[3/4] Grouping and indexing game families...", flush=True)
    adv_families, adv_g2f = build_families(adv_games)
    mame_families, mame_g2f = build_families(mame_games)

    print(f"      AdvanceMAME : {len(adv_families):,} families.")
    print(f"      MAME        : {len(mame_families):,} families.\n")

    # Key ROM -> game index in MAME for identifying renamed sets and parents
    mame_keyrom_to_games = defaultdict(list)
    for gname, ginfo in mame_games.items():
        if ginfo.key_rom:
            mame_keyrom_to_games[ginfo.key_rom].append(gname)

    print("[4/4] Comparing game families between emulators...\n", flush=True)

    # Determine what categories to display
    if args.parent_diff_only or args.rom_diff_only or args.common_only:
        show_common = True
        show_adv_only = False
        show_mame_only = False
    elif args.only_advmame and not args.only_mame:
        show_common = False
        show_adv_only = True
        show_mame_only = False
    elif args.only_mame and not args.only_advmame:
        show_common = False
        show_adv_only = False
        show_mame_only = True
    elif args.only_advmame and args.only_mame:
        show_common = False
        show_adv_only = True
        show_mame_only = True
    else:
        # Default (or --all): show common, adv-only, and mame-only
        show_common = True
        show_adv_only = True
        show_mame_only = True

    # Summary statistics
    total_matched_families = 0
    identical_families = 0
    parent_inverted_count = 0
    parent_renamed_count = 0
    parent_rom_changed_count = 0
    clones_renamed_total = 0
    clones_added_total = 0
    clones_removed_total = 0

    # Source file mappings tracker: adv_source -> {mame_source: set of families}
    source_mappings = defaultdict(lambda: defaultdict(set))

    # Output grouping tracker: adv_source -> list of family data dicts
    families_by_source = defaultdict(list)

    processed_mame_parents = set()
    adv_only_families = []

    # Iterate over AdvanceMAME families
    for adv_p_name, adv_fam in sorted(adv_families.items()):
        adv_p_info = adv_games[adv_p_name]
        adv_members = {adv_p_name} | adv_fam["clones"]

        # Find corresponding family in MAME:
        # Method 1: Shortname intersection across members
        candidate_mame_parents = set()
        for member in adv_members:
            if member in mame_g2f:
                candidate_mame_parents.add(mame_g2f[member])

        mame_p_name = None
        if candidate_mame_parents:
            # If multiple candidates, prioritize matching parent or highest clone overlap
            if adv_p_name in candidate_mame_parents:
                mame_p_name = adv_p_name
            else:
                mame_p_name = max(
                    candidate_mame_parents,
                    key=lambda mp: len(adv_members & ({mp} | mame_families[mp]["clones"])),
                )
        else:
            # Method 2: Fallback via Key ROM (if parent and all clones were renamed)
            if adv_p_info.key_rom and adv_p_info.key_rom in mame_keyrom_to_games:
                mame_match_name = mame_keyrom_to_games[adv_p_info.key_rom][0]
                mame_p_name = mame_g2f[mame_match_name]

        if not mame_p_name:
            # Family only in AdvanceMAME
            adv_sources = sorted({adv_games[m].sourcefile for m in adv_members if adv_games[m].sourcefile})
            adv_only_data = {
                "adv_p_name": adv_p_name,
                "adv_p_info": adv_p_info,
                "adv_sources": adv_sources,
                "clones": sorted(adv_fam["clones"]),
                "is_adv_only": True,
            }
            adv_only_families.append(adv_only_data)

            if args.game and args.game not in adv_members:
                continue

            if show_adv_only and not args.summary_only:
                primary_adv_source = adv_p_info.sourcefile or "unknown"
                families_by_source[primary_adv_source].append(adv_only_data)
            continue

        mame_fam = mame_families[mame_p_name]
        mame_p_info = mame_games[mame_p_name]
        mame_members = {mame_p_name} | mame_fam["clones"]

        total_matched_families += 1
        processed_mame_parents.add(mame_p_name)

        if args.game and args.game not in adv_members and args.game not in mame_members:
            continue

        # Collect driver source files for both emulators
        adv_sources = sorted({adv_games[m].sourcefile for m in adv_members if adv_games[m].sourcefile})
        mame_sources = sorted({mame_games[m].sourcefile for m in mame_members if mame_games[m].sourcefile})

        for a_src in adv_sources:
            for m_src in mame_sources:
                source_mappings[a_src][m_src].add(adv_p_name)

        # Parent analysis
        parent_diff_type = None  # None, 'INVERTED', 'RENAMED', 'DIFFERENT'
        parent_notes = []

        is_same_parent_name = (adv_p_name == mame_p_name)

        if not is_same_parent_name:
            # Case 1: Inversion (AdvMAME parent is clone in MAME, and MAME parent was clone in AdvMAME)
            if adv_p_name in mame_fam["clones"] and mame_p_name in adv_fam["clones"]:
                parent_diff_type = "INVERTED"
                parent_inverted_count += 1
                parent_notes.append(
                    f"Inversion: in AdvanceMAME the parent was '{adv_p_name}' (now a clone in MAME), "
                    f"while former clone '{mame_p_name}' is the new parent in MAME."
                )
            elif adv_p_info.key_rom and mame_p_info.key_rom and adv_p_info.key_rom == mame_p_info.key_rom:
                parent_diff_type = "RENAMED"
                parent_renamed_count += 1
                parent_notes.append(
                    f"Parent renamed: '{adv_p_name}' -> '{mame_p_name}' (same Key ROM: size={adv_p_info.key_rom[0]}, crc={adv_p_info.key_rom[1]})."
                )
            elif adv_p_name in mame_fam["clones"]:
                parent_diff_type = "PROMOTED_CLONE"
                parent_inverted_count += 1
                parent_notes.append(
                    f"Clone promoted: '{mame_p_name}' became the parent in MAME (was a clone in AdvanceMAME). "
                    f"Old parent '{adv_p_name}' is now a clone."
                )
            else:
                parent_diff_type = "DIFFERENT"
                parent_notes.append(f"Different parent: in AdvanceMAME '{adv_p_name}', in MAME '{mame_p_name}'.")

        # Parent ROM comparison
        rom_diff_detected = False
        rom_diff_notes = []

        if is_same_parent_name:
            key_rom_changed = (adv_p_info.key_rom != mame_p_info.key_rom)
            hashes_changed = (adv_p_info.rom_hashes != mame_p_info.rom_hashes)

            if key_rom_changed:
                rom_diff_detected = True
                parent_rom_changed_count += 1

                adv_kr = f"size={adv_p_info.key_rom[0]}, crc={adv_p_info.key_rom[1]}" if adv_p_info.key_rom else "None"
                mame_kr = f"size={mame_p_info.key_rom[0]}, crc={mame_p_info.key_rom[1]}" if mame_p_info.key_rom else "None"
                rom_diff_notes.append(f"Key ROM changed: [AdvanceMAME: {adv_kr}] vs [MAME: {mame_kr}]")

                # Check if old parent ROMs were moved to a clone (e.g. ddragon -> ddragonj)
                for m_clone in mame_fam["clones"]:
                    if mame_games[m_clone].key_rom == adv_p_info.key_rom:
                        rom_diff_notes.append(f"-> Key ROM of AdvanceMAME parent found in MAME clone '{m_clone}'!")

                # Check if new parent ROMs originated from an AdvanceMAME clone (e.g. ddragonw -> ddragon)
                for a_clone in adv_fam["clones"]:
                    if adv_games[a_clone].key_rom == mame_p_info.key_rom:
                        rom_diff_notes.append(f"-> Key ROM of MAME parent found in AdvanceMAME former clone '{a_clone}'!")

            elif args.full_romset and hashes_changed:
                rom_diff_detected = True
                parent_rom_changed_count += 1

                added_roms = len(mame_p_info.rom_hashes - adv_p_info.rom_hashes)
                removed_roms = len(adv_p_info.rom_hashes - mame_p_info.rom_hashes)
                rom_diff_notes.append(f"Full ROMSET modified (+{added_roms} added / -{removed_roms} removed or re-dumped)")

            # Cross-check: find best matching clone to detect version/region swaps when a diff is detected
            if rom_diff_detected:
                best_m_clone = None
                best_m_score = 0
                for m_clone in mame_fam["clones"]:
                    m_c_info = mame_games[m_clone]
                    if adv_p_info.rom_hashes and m_c_info.rom_hashes:
                        shared = len(adv_p_info.rom_hashes & m_c_info.rom_hashes)
                        total = len(adv_p_info.rom_hashes | m_c_info.rom_hashes)
                        score = shared / total if total > 0 else 0
                        if score > best_m_score:
                            best_m_score = score
                            best_m_clone = (m_clone, m_c_info.description, score, adv_p_info.rom_hashes == m_c_info.rom_hashes)

                if best_m_clone and best_m_score >= 0.8:
                    c_name, c_desc, score, exact = best_m_clone
                    match_str = "exactly" if exact else f"at {score*100:.0f}%"
                    rom_diff_notes.append(
                        f"-> AdvanceMAME parent ({adv_p_name}) matches {match_str} MAME clone '{c_name}' ({c_desc})"
                    )

                best_a_clone = None
                best_a_score = 0
                for a_clone in adv_fam["clones"]:
                    a_c_info = adv_games[a_clone]
                    if mame_p_info.rom_hashes and a_c_info.rom_hashes:
                        shared = len(mame_p_info.rom_hashes & a_c_info.rom_hashes)
                        total = len(mame_p_info.rom_hashes | a_c_info.rom_hashes)
                        score = shared / total if total > 0 else 0
                        if score > best_a_score:
                            best_a_score = score
                            best_a_clone = (a_clone, a_c_info.description, score, mame_p_info.rom_hashes == a_c_info.rom_hashes)

                if best_a_clone and best_a_score >= 0.8:
                    c_name, c_desc, score, exact = best_a_clone
                    match_str = "exactly" if exact else f"at {score*100:.0f}%"
                    rom_diff_notes.append(
                        f"-> MAME parent ({mame_p_name}) matches {match_str} AdvanceMAME former clone '{c_name}' ({c_desc})"
                    )

        # Clones analysis
        # 1. Clones with identical name
        common_clone_names = adv_fam["clones"] & mame_fam["clones"]

        # 2. Cloni in AdvanceMAME with name missing in MAME clones
        adv_unmatched_clones = adv_fam["clones"] - common_clone_names
        if mame_p_name in adv_unmatched_clones:
            adv_unmatched_clones.remove(mame_p_name)

        # 3. Clones in MAME missing in AdvanceMAME clones
        mame_unmatched_clones = mame_fam["clones"] - common_clone_names
        if adv_p_name in mame_unmatched_clones:
            mame_unmatched_clones.remove(adv_p_name)

        # Detect Renamed Clones (same Key ROM between adv_unmatched and mame_unmatched)
        renamed_clones = []  # list of (adv_clone, mame_clone)
        adv_matched_by_key = set()
        mame_matched_by_key = set()

        for a_clone in sorted(adv_unmatched_clones):
            a_info = adv_games[a_clone]
            if not a_info.key_rom:
                continue
            for m_clone in sorted(mame_unmatched_clones):
                if m_clone in mame_matched_by_key:
                    continue
                m_info = mame_games[m_clone]
                if a_info.key_rom == m_info.key_rom:
                    renamed_clones.append((a_clone, m_clone))
                    adv_matched_by_key.add(a_clone)
                    mame_matched_by_key.add(m_clone)
                    break

        clones_renamed_total += len(renamed_clones)

        # Clones actually removed from this family
        removed_clones = adv_unmatched_clones - adv_matched_by_key
        clones_removed_total += len(removed_clones)

        # Clones actually new in this family
        added_clones = mame_unmatched_clones - mame_matched_by_key
        clones_added_total += len(added_clones)

        has_differences = (
            parent_diff_type is not None
            or rom_diff_detected
            or bool(renamed_clones)
            or bool(removed_clones)
            or bool(added_clones)
        )

        if not has_differences:
            identical_families += 1

        # Display filters
        if not show_common:
            continue

        if args.summary_only:
            continue

        if args.parent_diff_only and (parent_diff_type is None and not rom_diff_detected):
            continue

        if args.rom_diff_only and not rom_diff_detected:
            continue

        if not args.all and not has_differences:
            continue

        # Store family result for grouped output by AdvanceMAME driver source
        fam_data = {
            "adv_p_name": adv_p_name,
            "mame_p_name": mame_p_name,
            "adv_p_info": adv_p_info,
            "mame_p_info": mame_p_info,
            "adv_sources": adv_sources,
            "mame_sources": mame_sources,
            "parent_notes": parent_notes,
            "rom_diff_notes": rom_diff_notes,
            "renamed_clones": renamed_clones,
            "removed_clones": removed_clones,
            "added_clones": added_clones,
            "has_differences": has_differences,
            "is_adv_only": False,
        }

        # Primary source file of the AdvanceMAME parent
        primary_adv_source = adv_p_info.sourcefile or "unknown"
        families_by_source[primary_adv_source].append(fam_data)

    # Process MAME families not matched to any AdvanceMAME parent
    mame_only_by_source = defaultdict(list)
    mame_only_families = []
    mame_split_families = []

    for mame_p_name, mame_fam in sorted(mame_families.items()):
        if mame_p_name in processed_mame_parents:
            continue

        mame_members = {mame_p_name} | mame_fam["clones"]
        mame_p_info = mame_games[mame_p_name]
        mame_sources = sorted({mame_games[m].sourcefile for m in mame_members if mame_games[m].sourcefile})
        primary_mame_source = mame_p_info.sourcefile or "unknown"

        shared = [m for m in mame_members if m in adv_g2f]
        if shared:
            adv_orig_fams = sorted({adv_g2f[m] for m in shared})
            split_data = {
                "mame_p_name": mame_p_name,
                "mame_p_info": mame_p_info,
                "mame_sources": mame_sources,
                "clones": sorted(mame_fam["clones"]),
                "shared": shared,
                "adv_orig_fams": adv_orig_fams,
            }
            mame_split_families.append(split_data)
            if (not args.game or args.game in mame_members) and show_mame_only and not args.summary_only:
                mame_only_by_source[primary_mame_source].append(("split", split_data))
        else:
            only_data = {
                "mame_p_name": mame_p_name,
                "mame_p_info": mame_p_info,
                "mame_sources": mame_sources,
                "clones": sorted(mame_fam["clones"]),
            }
            mame_only_families.append(only_data)
            if (not args.game or args.game in mame_members) and show_mame_only and not args.summary_only:
                mame_only_by_source[primary_mame_source].append(("only", only_data))

    # Print family details grouped by AdvanceMAME source file
    for adv_src in sorted(families_by_source.keys()):
        src_header = f"##### AdvanceMAME Driver Source: {adv_src} #####"
        print("#" * len(src_header))
        print(src_header)
        print("#" * len(src_header) + "\n")

        for f in families_by_source[adv_src]:
            if f.get("is_adv_only"):
                fam_header = f"=== Family: AdvanceMAME '{f['adv_p_name']}' [ONLY IN ADVANCEMAME] ==="
                print("=" * len(fam_header))
                print(fam_header)
                print("=" * len(fam_header))

                adv_cat = f" [{f['adv_p_info'].category}]" if f['adv_p_info'].category else ""
                print(f"Parent AdvanceMAME : {f['adv_p_name']} ({f['adv_p_info'].description}){adv_cat}")
                print(f"Source AdvanceMAME : {', '.join(f['adv_sources']) if f['adv_sources'] else 'N/A'}")
                print(f"  [!] Present only in AdvanceMAME (absent in MAME).")
                if f["clones"]:
                    print(f"  [*] Clones in AdvanceMAME ({len(f['clones'])}):")
                    for a_c in f["clones"]:
                        a_cat = f" [{adv_games[a_c].category}]" if adv_games[a_c].category else ""
                        print(f"      - '{a_c}': {adv_games[a_c].description}{a_cat}")
                print()
                continue

            fam_header = f"=== Family: AdvanceMAME '{f['adv_p_name']}' / MAME '{f['mame_p_name']}' ==="
            print("=" * len(fam_header))
            print(fam_header)
            print("=" * len(fam_header))

            adv_cat = f" [{f['adv_p_info'].category}]" if f['adv_p_info'].category else ""
            mame_cat = f" [{f['mame_p_info'].category}]" if f['mame_p_info'].category else ""
            print(f"Parent AdvanceMAME : {f['adv_p_name']} ({f['adv_p_info'].description}){adv_cat}")
            print(f"Parent MAME        : {f['mame_p_name']} ({f['mame_p_info'].description}){mame_cat}")
            print(f"Source AdvanceMAME : {', '.join(f['adv_sources']) if f['adv_sources'] else 'N/A'}")
            print(f"Source MAME        : {', '.join(f['mame_sources']) if f['mame_sources'] else 'N/A'}")

            if f["parent_notes"]:
                print("  [*] Parent Differences:")
                for note in f["parent_notes"]:
                    print(f"      - {note}")

            if f["rom_diff_notes"]:
                print("  [*] Parent ROM Differences:")
                for note in f["rom_diff_notes"]:
                    print(f"      - {note}")

            if f["renamed_clones"]:
                print(f"  [*] Renamed Clones ({len(f['renamed_clones'])}):")
                for a_c, m_c in f["renamed_clones"]:
                    m_cat = f" [{mame_games[m_c].category}]" if mame_games[m_c].category else ""
                    print(f"      - AdvanceMAME '{a_c}' -> MAME '{m_c}' (same Key ROM){m_cat}")

            if f["removed_clones"]:
                print(f"  [-] Clones absent in MAME ({len(f['removed_clones'])}):")
                for a_c in sorted(f["removed_clones"]):
                    desc = adv_games[a_c].description
                    a_cat = f" [{adv_games[a_c].category}]" if adv_games[a_c].category else ""
                    m_other_fam = mame_g2f.get(a_c)
                    if m_other_fam:
                        print(f"      - '{a_c}': moved to MAME family '{m_other_fam}' ({desc}){a_cat}")
                    else:
                        print(f"      - '{a_c}': {desc}{a_cat}")

            if f["added_clones"]:
                print(f"  [+] New clones in MAME ({len(f['added_clones'])}):")
                for m_c in sorted(f["added_clones"]):
                    desc = mame_games[m_c].description
                    m_cat = f" [{mame_games[m_c].category}]" if mame_games[m_c].category else ""
                    print(f"      - '{m_c}': {desc}{m_cat}")

            if not f["has_differences"]:
                print("  [OK] Family 100% identical (same parent, same ROMs, same clones).")

            print()

    # Print families present only in MAME (grouped by MAME driver source)
    if show_mame_only and mame_only_by_source and not args.summary_only:
        mame_section_header = "##### FAMILIES PRESENT ONLY IN MAME (OR SPLIT FROM ADVANCEMAME) #####"
        print("#" * len(mame_section_header))
        print(mame_section_header)
        print("#" * len(mame_section_header) + "\n")

        for mame_src in sorted(mame_only_by_source.keys()):
            src_header = f"##### MAME Driver Source: {mame_src} #####"
            print("#" * len(src_header))
            print(src_header)
            print("#" * len(src_header) + "\n")

            for kind, item in mame_only_by_source[mame_src]:
                if kind == "split":
                    fam_header = f"=== Family: MAME '{item['mame_p_name']}' [SPLIT FROM ADVANCEMAME] ==="
                    print("=" * len(fam_header))
                    print(fam_header)
                    print("=" * len(fam_header))
                    mame_cat = f" [{item['mame_p_info'].category}]" if item['mame_p_info'].category else ""
                    print(f"Parent MAME        : {item['mame_p_name']} ({item['mame_p_info'].description}){mame_cat}")
                    print(f"Source MAME        : {', '.join(item['mame_sources']) if item['mame_sources'] else 'N/A'}")
                    print(f"  [*] Origin       : Split from AdvanceMAME family: {', '.join(item['adv_orig_fams'])}")
                    print(f"  [*] Shared Games : {', '.join(item['shared'])}")
                    if item["clones"]:
                        print(f"  [*] Clones in MAME ({len(item['clones'])}):")
                        for m_c in item["clones"]:
                            m_cat = f" [{mame_games[m_c].category}]" if mame_games[m_c].category else ""
                            print(f"      - '{m_c}': {mame_games[m_c].description}{m_cat}")
                    print()
                else:
                    fam_header = f"=== Family: MAME '{item['mame_p_name']}' [ONLY IN MAME] ==="
                    print("=" * len(fam_header))
                    print(fam_header)
                    print("=" * len(fam_header))
                    mame_cat = f" [{item['mame_p_info'].category}]" if item['mame_p_info'].category else ""
                    print(f"Parent MAME        : {item['mame_p_name']} ({item['mame_p_info'].description}){mame_cat}")
                    print(f"Source MAME        : {', '.join(item['mame_sources']) if item['mame_sources'] else 'N/A'}")
                    print(f"  [!] Present only in MAME (absent in AdvanceMAME).")
                    if item["clones"]:
                        print(f"  [*] Clones in MAME ({len(item['clones'])}):")
                        for m_c in item["clones"]:
                            m_cat = f" [{mame_games[m_c].category}]" if mame_games[m_c].category else ""
                            print(f"      - '{m_c}': {mame_games[m_c].description}{m_cat}")
                    print()

    # Print final summary
    print("=" * 65)
    print("FINAL SUMMARY OF GAME FAMILY COMPARISON")
    print("=" * 65)
    print(f"Families analyzed in AdvanceMAME : {len(adv_families):,}")
    print(f"Families analyzed in MAME        : {len(mame_families):,}")
    print(f"Matching families found          : {total_matched_families:,}")
    print(f"100% identical families          : {identical_families:,}")
    print(f"Families with differences        : {total_matched_families - identical_families:,}")
    print(f"Families only in AdvanceMAME     : {len(adv_only_families):,}")
    print(f"Families only in MAME            : {len(mame_only_families):,}")
    if mame_split_families:
        print(f"MAME families split from AdvMAME : {len(mame_split_families):,}")
    print("-" * 65)
    print(f"Inverted parents / promoted clones : {parent_inverted_count:,}")
    print(f"Renamed parents (same Key ROM)     : {parent_renamed_count:,}")
    rom_mode_label = "full ROMSET" if args.full_romset else "Key ROM only"
    print(f"Parents with modified ROMs ({rom_mode_label}) : {parent_rom_changed_count:,}")
    print(f"Total renamed clones               : {clones_renamed_total:,}")
    print(f"New clones supported in MAME       : {clones_added_total:,}")
    print(f"Removed or missing clones          : {clones_removed_total:,}")
    print("=" * 65)

    if source_mappings:
        print()
        print("=" * 65)
        print("DRIVER SOURCE FILE MAPPINGS (AdvanceMAME -> MAME)")
        print("=" * 65)
        for a_src in sorted(source_mappings.keys()):
            m_dict = source_mappings[a_src]
            print(f"AdvanceMAME: {a_src}")
            for m_src in sorted(m_dict.keys()):
                fams = sorted(m_dict[m_src])
                if len(fams) <= 8:
                    fam_str = ", ".join(fams)
                else:
                    fam_str = f"{', '.join(fams[:8])}, ... (+{len(fams)-8} more)"
                print(f"  -> MAME: {m_src} (families: {fam_str})")
            print()


if __name__ == "__main__":
    main()
