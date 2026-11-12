#!/usr/bin/env python3
"""Identify AdvanceMAME games that reference ROM files absent from MAME.

This script runs `advmame -listxml` and `mame -listxml` from PATH, indexes all
non-nodump ROMs by `(CRC, size)` present in any MAME machine, and analyzes
every `<game>` entry in AdvanceMAME.

To distinguish games that exist in MAME (e.g. with redumped or reorganized
ROMs, even if renamed) from games that do not exist in MAME at all, the script
removes from each game (in both AdvanceMAME and MAME) all ROMs inherited from
its `romof` chain (parent/BIOS ancestors), leaving only the game's own
specific non-nodump ROMs:
- If at least one specific ROM is present in MAME and at least one is missing,
  the game is considered present in MAME with changed/missing ROMs.
- If ALL specific ROMs are absent from MAME, or if the game's shortname is
  listed in `./curated.json` in the current working directory, the game is
  assumed to be NOT present in MAME.

For games present in MAME with changed/missing ROMs, the script also searches
MAME machines whose specific ROMs (after `romof` removal) contain ALL of the
AdvanceMAME game's `[PRESENT]` specific ROMs. If there is a unique matching
MAME machine, it is shown as a potential match along with the specific ROMs
present in that MAME machine but not in the AdvanceMAME game (`[MAME ONLY]`).

By default, only games present in MAME with changed/missing specific ROMs are
listed. Use the CLI options to change this behavior:
  --include-not-in-mame  Also include games assumed NOT present in MAME.
  --only-not-in-mame     List ONLY games assumed NOT present in MAME.

Matching games are printed to stdout grouped by `sourcefile`, showing:
- short name
- full name (description)
- year
- parent/clone status (`parent` or `clone of <parent>`)
- `GAME_NOT_WORKING` flag (derived from `<driver status="preliminary">`)
- `NOT_IN_MAME (<reason>)` flag (when `--include-not-in-mame` or `--only-not-in-mame`
  is used), indicating `curated.json` or `all_roms_missing`
- category read from `./category.ini` in the current working directory
- all specific non-nodump ROMs of the game, marked as `[MISSING]` or `[PRESENT]`
- if uniquely matched in MAME, the potential MAME machine and its `[MAME ONLY]`
  specific ROMs
"""

import argparse
from collections import defaultdict
from dataclasses import dataclass
import json
import os
import shutil
import signal
import subprocess
import sys
from typing import Optional
import xml.etree.ElementTree as ET


@dataclass(frozen=True)
class RomEntry:
	name: str
	size: str
	crc: str
	status: str

	def key(self) -> tuple[str, ...]:
		if self.crc:
			return ("crc", self.crc, self.size)
		return ("name", self.name, self.size)


@dataclass
class GameEntry:
	name: str
	description: str
	year: str
	sourcefile: str
	cloneof: str
	romof: str
	not_working: bool
	roms: list[RomEntry]


@dataclass
class MameMatch:
	machine: GameEntry
	extra_roms: list[RomEntry]


@dataclass
class GameReport:
	game: GameEntry
	category: str
	not_in_mame: bool
	not_in_mame_reason: str
	specific_roms: list[tuple[RomEntry, bool]]
	mame_match: Optional[MameMatch]


def load_categories(path: str) -> dict[str, str]:
	"""Load game categories from a category.ini file in the current directory."""
	categories: dict[str, str] = {}
	if not os.path.isfile(path):
		print(
			f"Warning: '{path}' not found in current directory; categories will be empty.",
			file=sys.stderr,
		)
		return categories

	in_category_section = False
	with open(path, "r", encoding="utf-8", errors="replace") as f:
		for raw_line in f:
			line = raw_line.strip()
			if not line or line.startswith(";") or line.startswith("#"):
				continue
			if line.startswith("[") and line.endswith("]"):
				in_category_section = line[1:-1].strip().lower() == "category"
				continue
			if in_category_section and "=" in line:
				key, value = line.split("=", 1)
				categories[key.strip()] = value.strip()
	return categories


def load_curated_shortnames(path: str) -> set[str]:
	"""Load shortnames from curated.json in the current directory."""
	shortnames: set[str] = set()
	if not os.path.isfile(path):
		print(
			f"Warning: '{path}' not found in current directory; curated list will be empty.",
			file=sys.stderr,
		)
		return shortnames

	with open(path, "r", encoding="utf-8") as f:
		data = json.load(f)

	for game in data.get("games", []):
		for shortname in game.get("shortnames", []):
			if isinstance(shortname, str) and shortname:
				shortnames.add(shortname)
	return shortnames


def load_listxml_games(bin_path: str) -> dict[str, GameEntry]:
	"""Stream `<bin> -listxml` and return all `<game>`/`<machine>` entries indexed by name."""
	games: dict[str, GameEntry] = {}

	proc = subprocess.Popen(
		[bin_path, "-listxml"],
		stdout=subprocess.PIPE,
		stderr=subprocess.PIPE,
	)
	assert proc.stdout is not None

	try:
		for _event, elem in ET.iterparse(proc.stdout, events=("end",)):
			if elem.tag in ("game", "machine"):
				name = elem.get("name", "")
				desc_elem = elem.find("description")
				year_elem = elem.find("year")
				driver_elem = elem.find("driver")

				roms: list[RomEntry] = []
				for rom in elem.findall("rom"):
					if rom.get("status") == "nodump":
						continue
					roms.append(
						RomEntry(
							name=rom.get("name", ""),
							size=rom.get("size", ""),
							crc=(rom.get("crc") or "").lower(),
							status=rom.get("status", "good"),
						)
					)

				games[name] = GameEntry(
					name=name,
					description=desc_elem.text if desc_elem is not None and desc_elem.text else "",
					year=year_elem.text if year_elem is not None and year_elem.text else "",
					sourcefile=elem.get("sourcefile") or "(unknown)",
					cloneof=elem.get("cloneof") or "",
					romof=elem.get("romof") or "",
					not_working=(
						driver_elem is not None
						and driver_elem.get("status") == "preliminary"
					),
					roms=roms,
				)
				elem.clear()
	finally:
		proc.stdout.close()
		stderr_out = proc.stderr.read().decode("utf-8", errors="replace") if proc.stderr else ""
		if proc.stderr:
			proc.stderr.close()
		ret = proc.wait()
		if ret != 0:
			raise RuntimeError(f"'{bin_path} -listxml' exited with code {ret}: {stderr_out.strip()}")

	return games


def collect_romof_keys(game_name: str, games: dict[str, GameEntry]) -> set[tuple[str, ...]]:
	"""Collect all ROM keys from a game's `romof` ancestor chain."""
	inherited: set[tuple[str, ...]] = set()
	visited: set[str] = {game_name}
	curr_name = games[game_name].romof

	while curr_name and curr_name not in visited:
		visited.add(curr_name)
		ancestor = games.get(curr_name)
		if ancestor is None:
			break
		for rom in ancestor.roms:
			inherited.add(rom.key())
		curr_name = ancestor.romof

	return inherited


def build_mame_index(
	mame_games: dict[str, GameEntry],
) -> tuple[
	set[tuple[str, str]],
	dict[str, list[RomEntry]],
	dict[tuple[str, str], set[str]],
]:
	"""Build MAME CRC+size set, per-machine specific ROMs (post-romof), and inverted index."""
	mame_crc_size: set[tuple[str, str]] = set()
	mame_specific_roms: dict[str, list[RomEntry]] = {}
	rom_to_machines: dict[tuple[str, str], set[str]] = defaultdict(set)

	for name, machine in mame_games.items():
		for rom in machine.roms:
			if rom.crc:
				mame_crc_size.add((rom.crc, rom.size))

		inherited_keys = collect_romof_keys(name, mame_games)
		specific = [rom for rom in machine.roms if rom.key() not in inherited_keys]
		mame_specific_roms[name] = specific
		for rom in specific:
			if rom.crc:
				rom_to_machines[(rom.crc, rom.size)].add(name)

	return mame_crc_size, mame_specific_roms, rom_to_machines


def is_rom_in_mame(
	rom: RomEntry,
	mame_crc_size: set[tuple[str, str]],
) -> bool:
	"""Check whether a ROM (CRC, size) exists in any MAME machine."""
	if rom.crc:
		return (rom.crc, rom.size) in mame_crc_size
	return False


def find_unique_mame_match(
	specific_roms: list[tuple[RomEntry, bool]],
	mame_games: dict[str, GameEntry],
	mame_specific_roms: dict[str, list[RomEntry]],
	rom_to_machines: dict[tuple[str, str], set[str]],
) -> Optional[MameMatch]:
	"""Find a unique MAME machine whose specific ROMs contain all PRESENT specific ROMs."""
	present_keys = {
		(rom.crc, rom.size)
		for rom, in_mame in specific_roms
		if in_mame and rom.crc
	}
	if not present_keys:
		return None

	candidate_sets = [rom_to_machines.get(key, set()) for key in present_keys]
	if any(not s for s in candidate_sets):
		return None

	candidates = set.intersection(*candidate_sets)
	if len(candidates) != 1:
		return None

	match_name = next(iter(candidates))
	adv_keys = {rom.key() for rom, _ in specific_roms}
	extra_roms = [
		rom for rom in mame_specific_roms[match_name]
		if rom.key() not in adv_keys
	]
	return MameMatch(machine=mame_games[match_name], extra_roms=extra_roms)


def analyze_games(
	advmame_games: dict[str, GameEntry],
	mame_games: dict[str, GameEntry],
	mame_crc_size: set[tuple[str, str]],
	mame_specific_roms: dict[str, list[RomEntry]],
	rom_to_machines: dict[tuple[str, str], set[str]],
	categories: dict[str, str],
	curated_shortnames: set[str],
	include_not_in_mame: bool,
	only_not_in_mame: bool,
) -> dict[str, list[GameReport]]:
	"""Filter AdvanceMAME games by specific ROMs missing from MAME, grouped by sourcefile."""
	by_source: dict[str, list[GameReport]] = defaultdict(list)

	for name, game in advmame_games.items():
		inherited_keys = collect_romof_keys(name, advmame_games)
		specific_roms = [rom for rom in game.roms if rom.key() not in inherited_keys]
		if not specific_roms:
			continue

		rom_statuses = [
			(rom, is_rom_in_mame(rom, mame_crc_size))
			for rom in specific_roms
		]
		missing_count = sum(1 for _, in_mame in rom_statuses if not in_mame)
		if missing_count == 0:
			continue

		in_curated = name in curated_shortnames
		all_missing = missing_count == len(specific_roms)
		not_in_mame = all_missing or in_curated
		if in_curated:
			not_in_mame_reason = "curated.json"
		elif all_missing:
			not_in_mame_reason = "all_roms_missing"
		else:
			not_in_mame_reason = ""

		if only_not_in_mame and not not_in_mame:
			continue
		if not only_not_in_mame and not include_not_in_mame and not_in_mame:
			continue

		mame_match = None
		if not not_in_mame:
			mame_match = find_unique_mame_match(
				rom_statuses,
				mame_games,
				mame_specific_roms,
				rom_to_machines,
			)

		by_source[game.sourcefile].append(
			GameReport(
				game=game,
				category=categories.get(name, ""),
				not_in_mame=not_in_mame,
				not_in_mame_reason=not_in_mame_reason,
				specific_roms=rom_statuses,
				mame_match=mame_match,
			)
		)

	return by_source


def format_rom_details(rom: RomEntry) -> str:
	parts = [f"size={rom.size}"]
	if rom.crc:
		parts.append(f"crc={rom.crc}")
	if rom.status != "good":
		parts.append(f"status={rom.status}")
	return ", ".join(parts)


def print_report(by_source: dict[str, list[GameReport]]) -> None:
	"""Print the grouped report to stdout."""
	total_games = 0
	for sourcefile in sorted(by_source):
		reports = sorted(by_source[sourcefile], key=lambda r: r.game.name)
		total_games += len(reports)
		print(f"=== {sourcefile} ({len(reports)}) ===")
		for report in reports:
			game = report.game
			relation = f"clone of {game.cloneof}" if game.cloneof else "parent"
			flags = []
			if game.not_working:
				flags.append("GAME_NOT_WORKING")
			if report.not_in_mame:
				flags.append(f"NOT_IN_MAME ({report.not_in_mame_reason})")
			flag_str = f" | {' | '.join(flags)}" if flags else ""
			category = report.category if report.category else "(uncategorized)"
			year = game.year if game.year else "?"
			print(
				f"  {game.name} | {game.description} | {year} | {relation}{flag_str} | {category}"
			)
			for rom, in_mame in report.specific_roms:
				marker = "+ [PRESENT]" if in_mame else "- [MISSING]"
				print(f"      {marker} {rom.name} ({format_rom_details(rom)})")
			if report.mame_match is not None:
				m = report.mame_match.machine
				print(f"      -> MAME match: {m.name} | {m.description} ({m.sourcefile})")
				for rom in report.mame_match.extra_roms:
					print(f"         * [MAME ONLY] {rom.name} ({format_rom_details(rom)})")
		print()

	print(f"Total games listed: {total_games} (across {len(by_source)} source files)")


def parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(
		description="List AdvanceMAME games with ROMs missing from MAME."
	)
	group = parser.add_mutually_exclusive_group()
	group.add_argument(
		"--include-not-in-mame",
		action="store_true",
		help="Also include games assumed NOT present in MAME (all specific ROMs missing).",
	)
	group.add_argument(
		"--only-not-in-mame",
		action="store_true",
		help="List ONLY games assumed NOT present in MAME (all specific ROMs missing).",
	)
	return parser.parse_args()


def main() -> int:
	if hasattr(signal, "SIGPIPE"):
		signal.signal(signal.SIGPIPE, signal.SIG_DFL)

	args = parse_args()

	advmame_bin = shutil.which("advmame")
	mame_bin = shutil.which("mame")
	if not advmame_bin:
		print("Error: 'advmame' not found in PATH.", file=sys.stderr)
		return 1
	if not mame_bin:
		print("Error: 'mame' not found in PATH.", file=sys.stderr)
		return 1

	categories = load_categories("category.ini")
	curated_shortnames = load_curated_shortnames("curated.json")

	print("Loading MAME games...", file=sys.stderr)
	mame_games = load_listxml_games(mame_bin)
	mame_crc_size, mame_specific_roms, rom_to_machines = build_mame_index(mame_games)
	print(
		f"Indexed {len(mame_games)} MAME machines and {len(mame_crc_size)} CRC+size ROMs.",
		file=sys.stderr,
	)

	print("Loading AdvanceMAME games...", file=sys.stderr)
	advmame_games = load_listxml_games(advmame_bin)

	by_source = analyze_games(
		advmame_games=advmame_games,
		mame_games=mame_games,
		mame_crc_size=mame_crc_size,
		mame_specific_roms=mame_specific_roms,
		rom_to_machines=rom_to_machines,
		categories=categories,
		curated_shortnames=curated_shortnames,
		include_not_in_mame=args.include_not_in_mame,
		only_not_in_mame=args.only_not_in_mame,
	)

	print_report(by_source)
	return 0


if __name__ == "__main__":
	sys.exit(main())
