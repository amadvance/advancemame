#!/usr/bin/env python3
"""
family_compare.py - Compare game families between AdvanceMAME and MAME.

This tool executes 'advmame -listxml' and 'mame -listxml' found in PATH,
streaming their XML outputs directly via pipes in O(1) memory to compare game
families across emulator generations (MAME 0.106 baseline vs modern MAME).

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
  # Standard comparison (Key ROM differences only + final summary):
  ./family_compare.py

  # Inspect a specific game family:
  ./family_compare.py --game ddragon

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
        "cloneof",
        "romof",
        "key_rom",
        "rom_count",
        "total_rom_size",
        "rom_hashes",
    )

    def __init__(self, name, sourcefile, description, cloneof, romof, key_rom, rom_count, total_rom_size, rom_hashes):
        self.name = name
        self.sourcefile = sourcefile
        self.description = description
        self.cloneof = cloneof
        self.romof = romof
        self.key_rom = key_rom  # (size, crc_lower) or None
        self.rom_count = rom_count
        self.total_rom_size = total_rom_size
        self.rom_hashes = rom_hashes  # set of (size, crc_lower)


def parse_listxml_stream(cmd, is_advmame=False):
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
                # In modern MAME, discard hardware device nodes that are not games/machines
                if not is_advmame and elem.attrib.get("isdevice") == "yes":
                    root.clear()
                    continue

                name = elem.attrib.get("name")
                if not name:
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

    # 1. Parsing AdvanceMAME
    print("[1/4] Running 'advmame -listxml' in streaming...", flush=True)
    t0 = time.time()
    adv_games = parse_listxml_stream([advmame_bin, "-listxml"], is_advmame=True)
    t_adv = time.time() - t0
    print(f"      -> {len(adv_games):,} games parsed in {t_adv:.2f}s.\n", flush=True)

    # 2. Parsing MAME
    print("[2/4] Running 'mame -listxml' in streaming (may take around one minute)...", flush=True)
    t0 = time.time()
    mame_games = parse_listxml_stream([mame_bin, "-listxml"], is_advmame=False)
    t_mame = time.time() - t0
    print(f"      -> {len(mame_games):,} machines/games parsed in {t_mame:.2f}s.\n", flush=True)

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

    print("[4/4] Comparing families present in both emulators...\n", flush=True)

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

    processed_mame_parents = set()

    # Iterate over AdvanceMAME families
    for adv_p_name, adv_fam in sorted(adv_families.items()):
        adv_p_info = adv_games[adv_p_name]
        adv_members = {adv_p_name} | adv_fam["clones"]

        # Filter by specific game if requested:
        if args.game and args.game not in adv_members:
            continue

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
            # Family only in AdvanceMAME (or driver removed/separated)
            continue

        mame_fam = mame_families[mame_p_name]
        mame_p_info = mame_games[mame_p_name]
        mame_members = {mame_p_name} | mame_fam["clones"]

        total_matched_families += 1
        processed_mame_parents.add(mame_p_name)

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
        if args.summary_only:
            continue

        if args.parent_diff_only and (parent_diff_type is None and not rom_diff_detected):
            continue

        if args.rom_diff_only and not rom_diff_detected:
            continue

        if not args.all and not has_differences:
            continue

        # Print family details
        fam_header = f"=== Family: AdvanceMAME '{adv_p_name}' / MAME '{mame_p_name}' ==="
        print("=" * len(fam_header))
        print(fam_header)
        print("=" * len(fam_header))

        print(f"Parent AdvanceMAME : {adv_p_name} ({adv_p_info.description})")
        print(f"Parent MAME        : {mame_p_name} ({mame_p_info.description})")
        print(f"Source AdvanceMAME : {', '.join(adv_sources) if adv_sources else 'N/A'}")
        print(f"Source MAME        : {', '.join(mame_sources) if mame_sources else 'N/A'}")

        if parent_notes:
            print("  [*] Parent Differences:")
            for note in parent_notes:
                print(f"      - {note}")

        if rom_diff_notes:
            print("  [*] Parent ROM Differences:")
            for note in rom_diff_notes:
                print(f"      - {note}")

        if renamed_clones:
            print(f"  [*] Renamed Clones ({len(renamed_clones)}):")
            for a_c, m_c in renamed_clones:
                print(f"      - AdvanceMAME '{a_c}' -> MAME '{m_c}' (same Key ROM)")

        if removed_clones:
            print(f"  [-] Clones absent in MAME ({len(removed_clones)}):")
            for a_c in sorted(removed_clones):
                desc = adv_games[a_c].description
                m_other_fam = mame_g2f.get(a_c)
                if m_other_fam:
                    print(f"      - '{a_c}': moved to MAME family '{m_other_fam}' ({desc})")
                else:
                    print(f"      - '{a_c}': {desc}")

        if added_clones:
            print(f"  [+] New clones in MAME ({len(added_clones)}):")
            for m_c in sorted(added_clones):
                desc = mame_games[m_c].description
                print(f"      - '{m_c}': {desc}")

        if not has_differences:
            print("  [OK] Family 100% identical (same parent, same ROMs, same clones).")

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
