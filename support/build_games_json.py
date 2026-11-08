#!/usr/bin/env python3
"""
Build the baseline AdvanceMAME game catalog (../games.json) from AdvanceMAME -listxml and internal files.

Assumes execution from the support/ directory:
- Target games catalog is written one level above at ../games.json.
- Genre classifications are read from genre.ini in the current directory.
- Curated game metadata and descriptions are read from curated.json in the current directory.
- Game descriptions are read from description.ini in the current directory.
- AdvanceMAME -listxml output is generated dynamically using the "advmame" binary in PATH
  (or via --binary / --listxml).

Behavior:
- Parses all active games and ROM sets from AdvanceMAME (-listxml).
- Groups clones, regional variants, revisions, and standard bootlegs under their parent set,
  while keeping distinct hacks, homebrew releases, and special editions in dedicated records.
- Populates baseline structured fields (shortnames, title, year, developer, publisher, category,
  genre, platform, players, release_status, version, parent_shortname, original_game, variants,
  sources, confidence, research_notes).
- Applies special curated metadata (descriptions, hacks, homebrew, translations) from curated.json.
- Applies narrative game descriptions from description.ini (curated.json takes precedence).
- Leaves external metadata (trivia, tips, scoring, series, languages) for enrich_games.py.

Command-line usage (run from support/):
    python3 build_games_json.py [options]

Options:
    -o, --output PATH         Output games.json file path (default: ../games.json)
    -c, --curated PATH        Path to curated.json file (default: curated.json)
    -d, --description-ini PATH Path to description.ini file (default: description.ini)
    -g, --genre-ini PATH      Path to genre.ini file (default: genre.ini)
    -x, --listxml PATH        Optional path to cached -listxml file (default: None, generated via binary)
    -b, --binary PATH         AdvanceMAME binary command or path (default: advmame)
"""

import argparse
import json
import os
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict

parser = argparse.ArgumentParser(
    description="Build ../games.json covering all AdvanceMAME games from -listxml and internal support files."
)
parser.add_argument(
    "-o",
    "--output",
    default="../games.json",
    help="Output games.json file path (default: ../games.json)",
)
parser.add_argument(
    "-c",
    "--curated",
    default="curated.json",
    help="Path to curated.json file (default: curated.json)",
)
parser.add_argument(
    "-d",
    "--description-ini",
    default="description.ini",
    help="Path to description.ini file (default: description.ini)",
)
parser.add_argument(
    "-g",
    "--genre-ini",
    default="genre.ini",
    help="Path to genre.ini file (default: genre.ini)",
)
parser.add_argument(
    "-x",
    "--listxml",
    default=None,
    help="Optional path to cached AdvanceMAME -listxml XML file (default: None, generated on the fly via <binary> -listxml)",
)
parser.add_argument(
    "-b",
    "--binary",
    default="advmame",
    help="AdvanceMAME binary command or path (default: advmame)",
)
args = parser.parse_args()

# 1. Load official MAME shortnames from ../resource/mame/src/mame/mame.lst (if present)
mame_lst_path = "../resource/mame/src/mame/mame.lst"
mame_set = set()
if os.path.isfile(mame_lst_path):
    with open(mame_lst_path, "r", encoding="utf-8", errors="ignore") as f:
        mame_lst = f.read()
    mame_lst = re.sub(r"//.*", "", mame_lst)
    mame_lst = re.sub(r"/\*.*?\*/", "", mame_lst, flags=re.S)
    mame_set = set(w.strip() for w in mame_lst.splitlines() if w.strip() and not w.strip().startswith("@"))

# 2. Load genre.ini
genres = {}
with open(args.genre_ini, "r", encoding="latin-1") as f:
    for line in f:
        line = line.strip()
        if "=" in line and not line.startswith("["):
            k, v = line.split("=", 1)
            genres[k.strip()] = v.strip()

# 3. Parse AdvanceMAME -listxml (always generated from advmame in PATH unless cached path given)
listxml_path = args.listxml
if not listxml_path:
    listxml_path = "/tmp/advancemame/listxml.xml"
    os.makedirs(os.path.dirname(os.path.abspath(listxml_path)), exist_ok=True)
    print(f"Generating -listxml using '{args.binary}'...", flush=True)
    with open(listxml_path, "wb") as out_xml:
        subprocess.run([args.binary, "-listxml"], stdout=out_xml, check=True)
elif not os.path.isfile(listxml_path):
    os.makedirs(os.path.dirname(os.path.abspath(listxml_path)), exist_ok=True)
    print(f"Generating -listxml using '{args.binary}' to '{listxml_path}'...", flush=True)
    with open(listxml_path, "wb") as out_xml:
        subprocess.run([args.binary, "-listxml"], stdout=out_xml, check=True)

tree = ET.parse(listxml_path)
root = tree.getroot()
adv_all = {}
bios_names = set()

for g in root.findall("game"):
    name = g.attrib["name"]
    runnable = g.attrib.get("runnable", "yes")
    if runnable == "no":
        bios_names.add(name)
    inp = g.find("input")
    players = int(inp.attrib["players"]) if inp is not None and "players" in inp.attrib else None
    drv = g.find("driver")
    drv_status = drv.attrib.get("status") if drv is not None else None
    adv_all[name] = {
        "name": name,
        "sourcefile": g.attrib.get("sourcefile") or "",
        "runnable": runnable,
        "cloneof": g.attrib.get("cloneof"),
        "romof": g.attrib.get("romof"),
        "description": (g.findtext("description") or "").strip(),
        "year": (g.findtext("year") or "").strip(),
        "manufacturer": (g.findtext("manufacturer") or "").strip(),
        "players": players,
        "driver_status": drv_status,
    }

target_names = sorted(adv_all.keys())

# 4. Load curated.json (schema: {"schema_version": 1, "games": [...]})
curated_by_primary = {}
curated_by_sname = {}
curated_group_map = {}
if os.path.isfile(args.curated):
    with open(args.curated, "r", encoding="utf-8") as f:
        cur_db = json.load(f)
    for cur_rec in cur_db.get("games", []):
        cur_snames = cur_rec.get("shortnames", [])
        if not cur_snames:
            continue
        primary_sn = cur_snames[0]
        curated_by_primary[primary_sn] = cur_rec
        grp_id = f"curated:{primary_sn}"
        for sn in cur_snames:
            curated_by_sname[sn] = cur_rec
            curated_group_map[sn] = grp_id
    print(f"Loaded {len(curated_by_primary)} curated records covering {len(curated_group_map)} shortnames from '{args.curated}'.", flush=True)

# 5. Load description.ini
descriptions_ini = {}
if os.path.isfile(args.description_ini):
    with open(args.description_ini, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("[") or line.startswith(";") or line.startswith("#"):
                continue
            if "=" in line:
                k, v = line.split("=", 1)
                k = k.strip()
                v = v.strip().replace("\\n", "\n")
                if k and v:
                    descriptions_ini[k] = v
    print(f"Loaded {len(descriptions_ini)} descriptions from '{args.description_ini}'.", flush=True)

PLATFORM_MAP = {
    "neogeo.c": "Neo Geo MVS",
    "cps1.c": "CPS-1",
    "cps2.c": "CPS-2",
    "cps3.c": "CPS-3",
    "pgm.c": "PGM",
    "cv1k.c": "Cave CV1000",
    "cave.c": "Cave 68000",
    "stv.c": "Sega ST-V",
    "megaplay.c": "Sega Mega Play",
    "megatech.c": "Sega Mega-Tech",
    "system1.c": "Sega System 1",
    "system16.c": "Sega System 16",
    "segas16a.c": "Sega System 16A",
    "segas16b.c": "Sega System 16B",
    "segas18.c": "Sega System 18",
    "system24.c": "Sega System 24",
    "segas32.c": "Sega System 32",
    "segac2.c": "Sega System C-2",
    "segaorun.c": "Sega OutRun hardware",
    "segahang.c": "Sega Hang-On hardware",
    "segaxbd.c": "Sega X Board",
    "model1.c": "Sega Model 1",
    "model3.c": "Sega Model 3",
    "vicdual.c": "Sega VIC Dual",
    "zn.c": "Sony ZN-1 / ZN-2",
    "konamigv.c": "Konami GV System",
    "konamigx.c": "Konami GX",
    "djmain.c": "Konami DJ Main",
    "twin16.c": "Konami Twin 16",
    "namcos1.c": "Namco System 1",
    "namcos2.c": "Namco System 2",
    "namcos11.c": "Namco System 11",
    "namcos12.c": "Namco System 12",
    "namcos21.c": "Namco System 21",
    "namcos22.c": "Namco System 22",
    "namcos23.c": "Namco System 23",
    "namcos86.c": "Namco System 86",
    "namcona1.c": "Namco NA-1 / NA-2",
    "namconb1.c": "Namco NB-1 / NB-2",
    "namcofl.c": "Namco FL",
    "taito_b.c": "Taito B System",
    "taito_f2.c": "Taito F2 System",
    "taito_f3.c": "Taito F3 System",
    "taito_l.c": "Taito L System",
    "taito_x.c": "Taito X System",
    "taito_z.c": "Taito Z System",
    "taitojc.c": "Taito JC System",
    "taitosj.c": "Taito SJ System",
    "toaplan1.c": "Toaplan 1",
    "toaplan2.c": "Toaplan 2",
    "vsnes.c": "Nintendo Vs. System",
    "playch10.c": "Nintendo PlayChoice-10",
    "decocass.c": "DECO Cassette System",
    "deco_mlc.c": "Data East MLC",
    "deco32.c": "Data East 32-bit",
    "dec0.c": "Data East MEC-M1",
    "dec8.c": "Data East 8-bit",
    "m62.c": "Irem M62",
    "m72.c": "Irem M72",
    "m90.c": "Irem M90",
    "m92.c": "Irem M92",
    "atarisy1.c": "Atari System 1",
    "atarisy2.c": "Atari System 2",
    "atarig1.c": "Atari G1",
    "atarigt.c": "Atari GT",
    "atarigx2.c": "Atari GX2",
    "itech8.c": "Incredible Technologies 8-bit",
    "itech32.c": "Incredible Technologies 32-bit",
    "midtunit.c": "Midway T-Unit",
    "midyunit.c": "Midway Y-Unit",
    "midwunit.c": "Midway Wolf Unit",
    "midvunit.c": "Midway V-Unit",
    "midzeus.c": "Midway Zeus",
    "seattle.c": "Atari / Midway Seattle",
    "vegas.c": "Atari / Midway Vegas",
    "mcr.c": "Bally Midway MCR",
    "mcr68.c": "Bally Midway MCR-68",
    "seibuspi.c": "Seibu SPI",
    "psikyo.c": "Psikyo 68EC020",
    "psikyosh.c": "Psikyo SH-2",
    "psikyo4.c": "Psikyo PS4",
    "kaneko16.c": "Kaneko 16",
    "suprnova.c": "Kaneko Super Nova System",
    "megasys1.c": "Jaleco Mega System 1",
    "ms32.c": "Jaleco Mega System 32",
    "ssv.c": "Seta / Sammy / Visco SSV",
    "nmk16.c": "NMK 16-bit",
    "alpha68k.c": "Alpha Denshi 68000",
    "snk68.c": "SNK 68000",
    "mpu4.c": "Barcrest MPU4",
    "cvs.c": "Century CVS",
    "pacman.c": "Namco Pac-Man hardware",
    "galaxian.c": "Namco Galaxian hardware",
    "gng.c": "Capcom Ghosts'n Goblins hardware",
}

def get_platform(name):
    sf = adv_all[name]["sourcefile"]
    return PLATFORM_MAP.get(sf, "Arcade")

def get_genre(name):
    if name in genres:
        return genres[name]
    curr = adv_all[name]["cloneof"]
    visited = set()
    while curr and curr not in visited:
        visited.add(curr)
        if curr in genres:
            return genres[curr]
        curr = adv_all.get(curr, {}).get("cloneof")
    if name in bios_names:
        return "System"
    return "Arcade"

def parse_year(y_str):
    if not y_str:
        return None
    if re.fullmatch(r"\d{4}", y_str):
        return int(y_str)
    return None

def parse_manufacturer(mfg_str):
    if not mfg_str:
        return None, None
    s = mfg_str.strip()
    low = s.lower()
    if low in ("bootleg", "bootleg?", "<unknown>", "unknown", "hack"):
        return None, None
    if s.endswith("?") and s[:-1].strip().lower() in ("bootleg", "unknown"):
        return None, None
    if s.endswith("?"):
        s = s[:-1].strip()

    m = re.match(r"^\[([^\]]+)\]\s+([^(]+?)\s*\(([^)]+?)\s+licen[cs]e\)$", s, re.I)
    if m:
        return m.group(1).strip(), f"{m.group(2).strip()} / {m.group(3).strip()}"

    m = re.match(r"^\[([^\]]+)\]\s*\(([^)]+?)\s+licen[cs]e\)$", s, re.I)
    if m:
        return m.group(1).strip(), m.group(2).strip()

    m = re.match(r"^\[([^\]]+)\]\s+(.+)$", s)
    if m:
        return m.group(1).strip(), m.group(2).strip()

    m = re.match(r"^([^(]+?)\s*\(licensed from\s+([^)]+)\)$", s, re.I)
    if m:
        return m.group(2).strip(), m.group(1).strip()

    m = re.match(r"^([^(]+?)\s*\(([^)]+?)\s+licen[cs]e\)$", s, re.I)
    if m:
        return m.group(1).strip(), m.group(2).strip()

    m = re.match(r"^([^(]+?)\s*\(([^)]+)\)$", s)
    if m:
        return m.group(1).strip(), m.group(2).strip()

    return s, s

def get_root(name):
    curr = name
    visited = set()
    while curr in adv_all:
        visited.add(curr)
        p = adv_all[curr]["cloneof"]
        if not p or p in bios_names or p in visited:
            return p if (p and p not in bios_names) else curr
        curr = p
    return curr

KEEP_PARENS = {
    "4-Player",
    "Endurance Course Version",
    "Mega Play",
    "Sex Kit",
    "Single Monitor",
    "Team Challenge",
    "The Great Shot",
    "Unlimited Version",
}

def clean_base_title(desc, root_name=None):
    if desc == "<unknown> Meyco game":
        return "Unknown Meyco Game"
    s = desc.strip()
    if s.endswith(" Alt"):
        s = s[:-4].strip()
    # Extract trailing parentheticals and brackets
    trailing = []
    while True:
        m = re.search(r"\s*(\([^)]*\)|\[[^\]]*\])\s*$", s)
        if not m:
            break
        token = m.group(1)
        s = s[:m.start()].strip()
        if token.startswith("(") and token.endswith(")"):
            inner = token[1:-1].strip()
            if inner in KEEP_PARENS:
                trailing.append(f"({inner})")
    trailing.reverse()
    if trailing:
        s = s + " " + " ".join(trailing)
    # Strip trailing version suffix like " v8.41" or " V25.4X" only if there is a version field? No, keep "Cherry Master II" clean if version is extracted
    return s.strip()

def parse_desc_details(desc):
    """Extract region, language, version, and differences from parentheticals in description."""
    parens = re.findall(r"\(([^)]+)\)", desc)
    brackets = re.findall(r"\[([^\]]+)\]", desc)
    region = None
    language = None
    version = None
    diffs = []

    REGION_PATTERNS = [
        (r"\bWorld\??\b", "World"),
        (r"\bEuro(?:pe)?\b", "Europe"),
        (r"\bU\.?S\.?A?\.?\b|\bUS\b", "USA"),
        (r"\bJapan(?:ese)?\b", "Japan"),
        (r"\bAsia\b", "Asia"),
        (r"\bKorea(?:n)?\b", "Korea"),
        (r"\bHispanic\b", "Hispanic"),
        (r"\bBrazil\b", "Brazil"),
        (r"\bGerman(?:y)?\b", "Germany"),
        (r"\bItaly\b|\bItalian\b", "Italy"),
        (r"\bSpain\b|\bSpanish\b", "Spain"),
        (r"\bFrance\b|\bFrench\b", "France"),
        (r"\bUK\b", "UK"),
        (r"\bAustralia\b", "Australia"),
        (r"\bAustria\b", "Austria"),
        (r"\bBulgaria\b", "Bulgaria"),
        (r"\bArgentina\b", "Argentina"),
        (r"\bCroatia\b", "Croatia"),
        (r"\bChina\b|\bChinese\b", "China"),
        (r"\bTaiwan\b", "Taiwan"),
        (r"\bHong Kong\b", "Hong Kong"),
    ]

    LANG_PATTERNS = [
        (r"\bEnglish\b", "English"),
        (r"\bJapanese\b", "Japanese"),
        (r"\bGerman\b", "German"),
        (r"\bFrench\b", "French"),
        (r"\bSpanish\b", "Spanish"),
        (r"\bItalian\b", "Italian"),
    ]

    for p in parens:
        p_clean = p.strip()
        if p_clean in KEEP_PARENS:
            continue
        # Check region codes like JUE, JUET, JET, J, EA
        m_stv = re.match(r"^(JUE|JUET|JET|J|EA)\s+(\d{6}\s+V[\d.]+)$", p_clean)
        if m_stv:
            rcode, ver = m_stv.groups()
            rmap = {
                "J": "Japan",
                "JUE": "Japan / USA / Europe",
                "JUET": "Japan / USA / Europe / Taiwan",
                "JET": "Japan / Europe / Taiwan",
                "EA": "Europe / Asia",
            }
            region = rmap.get(rcode, rcode)
            version = ver
            continue

        # Check region
        found_regs = []
        for pat, rname in REGION_PATTERNS:
            if re.search(pat, p_clean, re.I):
                if rname not in found_regs:
                    found_regs.append(rname)
        if found_regs and region is None:
            region = " / ".join(found_regs)

        # Check language
        for pat, lname in LANG_PATTERNS:
            if re.search(pat, p_clean, re.I) and ("translation" in p_clean.lower() or "language" in p_clean.lower() or p_clean.lower() in ("english", "japanese", "german", "french", "spanish", "italian") or "english)" in desc.lower()):
                if language is None:
                    language = lname

        # Check version/revision
        m_ver = re.search(r"\b(?:(?:Ver|Version|Rev|Revision)(?:\.|\s)+|proto\s+v|v(?=\d))([A-Za-z0-9._-]+(?:\s+Ver\.?\s*[A-Za-z0-9._-]+)?)", p_clean, re.I)
        if m_ver and version is None:
            version = m_ver.group(0).strip()
        elif re.fullmatch(r"\d{4}[-/]\s*\d{1,2}[-/]\s*\d{1,2}(?:\s+MASTER\s+VER\.?)?", p_clean, re.I):
            if version is None:
                version = re.sub(r"\s+", " ", p_clean)
        elif re.search(r"\b\d{6}\b", p_clean) and version is None:
            m_d = re.search(r"\b(\d{6})\b", p_clean)
            version = m_d.group(1)

        # Specific meaningful differences
        low_p = p_clean.lower()
        if p_clean == "Enhanced":
            diffs.append("Enhanced Gameplay Edition with fluid crouching and smooth ladder climbing.")
        elif "not encrypted" in low_p or "decrypted" in low_p:
            diffs.append("Uses unencrypted ROMs.")
        elif "no cd" in low_p:
            diffs.append("Runs without CD-ROM drive (No CD).")
        elif "dirty dump" in low_p:
            diffs.append("Alternate ROM dump with pre-populated NVRAM/EEPROM data.")
        elif "joystick" in low_p:
            diffs.append("Configured for standard joystick controls.")
        elif "rotary" in low_p:
            diffs.append("Configured for rotary joystick controls.")
        elif "trackball" in low_p:
            diffs.append("Configured for trackball controls.")
        elif "cocktail" in low_p:
            diffs.append("Cocktail cabinet configuration.")
        elif "single monitor" in low_p:
            diffs.append("Single-monitor cabinet configuration.")
        elif "315-" in low_p or "fd1089" in low_p or "fd1094" in low_p or "mc-8123" in low_p or "8751" in low_p:
            diffs.append(f"Hardware configuration: {p_clean}.")

    return region, language, version, diffs

def get_group_key(name):
    if name in curated_group_map:
        return curated_group_map[name]
    if name in bios_names:
        return f"bios:{name}"
    r = get_root(name)
    return f"root:{r}"

groups = defaultdict(list)
for n in target_names:
    groups[get_group_key(n)].append(n)

# Special curated metadata for researched games, hacks, translations, homebrew, prototypes, and originals
def determine_category_and_status(primary, names, root_name):
    cur = curated_by_primary.get(primary) or next((curated_by_sname[s] for s in names if s in curated_by_sname), {})
    if "category" in cur:
        return cur["category"], cur.get("release_status", "released")
    if primary in bios_names:
        return "original", "released"

    g = adv_all[primary]
    d = g["description"].lower()
    mfg = g["manufacturer"].lower()
    sf = g["sourcefile"]

    # Check prototype
    if "proto" in d or "prototype" in d:
        return "prototype", "prototype"
    # Check homebrew
    if "homebrew" in d:
        return "homebrew", "released"
    # Check hack
    if "hack" in d or mfg == "hack" or "translation" in d:
        return "hack", "released"
    # Check bootleg
    if "bootleg" in d or "bootleg" in mfg or primary in ("sboblbob", "kikcubib", "vautour2", "topracra", "wc90b", "searthia"):
        return "bootleg", "released"
    # Check console-to-arcade port / Mega Play / Vs. / PlayChoice
    if sf in ("megaplay.c", "playch10.c"):
        return "port", "released"

    # If primary has no cloneof (or cloneof is a BIOS), it's a parent set in AdvanceMAME
    co = g["cloneof"]
    if not co or co in bios_names:
        return "original", "released"

    # Otherwise it's a clone or regional variant
    reg, _, _, _ = parse_desc_details(g["description"])
    parent_g = adv_all.get(co, {})
    parent_reg, _, _, _ = parse_desc_details(parent_g.get("description", ""))
    if reg and reg != parent_reg:
        return "regional_variant", "released"
    return "clone", "released"

def build_record(group_key, names):
    # Choose primary shortname:
    # 1. If a member has cloneof == None or cloneof in bios_names, or is the parent of other members in the group
    name_set = set(names)
    parents_in_group = [n for n in names if not adv_all[n]["cloneof"] or adv_all[n]["cloneof"] in bios_names or adv_all[n]["cloneof"] not in name_set]
    # Prefer the member that other members in the group point to as cloneof
    pointed_to = set(adv_all[n]["cloneof"] for n in names if adv_all[n]["cloneof"] in name_set)
    if pointed_to:
        # If gngprincess and makprincess are both in pointed_to, pick gngprincess
        if "gngprincess" in pointed_to:
            primary = "gngprincess"
        else:
            primary = sorted(pointed_to)[0]
    elif parents_in_group:
        primary = sorted(parents_in_group)[0]
    else:
        primary = sorted(names)[0]

    rest = sorted(n for n in names if n != primary)
    shortnames = [primary] + rest

    p_info = adv_all[primary]
    root_name = get_root(primary)
    root_info = adv_all.get(root_name, p_info)

    cat, rel_status = determine_category_and_status(primary, shortnames, root_name)

    # Title
    cur = curated_by_primary.get(primary) or next((curated_by_sname[s] for s in shortnames if s in curated_by_sname), {})
    if "title" in cur:
        title = cur["title"]
    else:
        title = clean_base_title(p_info["description"], root_name)

    # Year: use primary's 4-digit year, or fallback to root_info's 4-digit year if clone has empty year
    year = parse_year(p_info["year"])
    if year is None and root_name != primary:
        year = parse_year(root_info["year"])

    # Developer / Publisher
    if "developer" in cur or "publisher" in cur:
        dev = cur.get("developer")
        pub = cur.get("publisher")
    else:
        dev, pub = parse_manufacturer(p_info["manufacturer"])
        if dev is None and pub is None and cat in ("clone", "regional_variant", "prototype", "hack"):
            dev, pub = parse_manufacturer(root_info["manufacturer"])

    genre = get_genre(primary)
    platform = get_platform(primary)
    players = p_info["players"]
    if players is None and root_name != primary:
        players = root_info["players"]

    reg_p, lang_p, ver_p, diffs_p = parse_desc_details(p_info["description"])
    version = cur.get("version", ver_p if len(shortnames) == 1 else None)

    # Relationships
    parent_shortname = p_info["cloneof"] if p_info["cloneof"] else None
    if "original_game" in cur:
        original_game = cur["original_game"]
    elif cat == "original":
        original_game = None
    else:
        original_game = root_name if root_name != primary else parent_shortname

    # Build variants
    hack_author_val = None
    if cat == "hack":
        hack_author_val = cur.get("hack_author", dev if p_info["manufacturer"].lower() not in ("bootleg", "bootleg?", "hack", "<unknown>", "unknown", "") and dev != parse_manufacturer(root_info["manufacturer"])[0] else None)

    variants = []
    for sname in shortnames:
        s_info = adv_all[sname]
        s_reg, s_lang, s_ver, s_diffs = parse_desc_details(s_info["description"])
# Language fallback is populated in enrich_games.py
        s_year = parse_year(s_info["year"])
        s_dev, s_pub = parse_manufacturer(s_info["manufacturer"])
        if cat == "hack" and s_dev and hack_author_val and (s_dev in hack_author_val or hack_author_val in s_dev):
            s_dev, s_pub = dev, pub
        s_title = s_info["description"]
        s_parent = s_info["cloneof"] if s_info["cloneof"] else None

        v_obj = {"shortname": sname}
        if sname != primary and s_parent != parent_shortname:
            v_obj["parent_shortname"] = s_parent
        if len(shortnames) > 1 or s_title != title:
            v_obj["title"] = s_title
        if s_year is not None and s_year != year:
            v_obj["year"] = s_year
        if s_dev is not None and s_dev != dev:
            v_obj["developer"] = s_dev
        if s_pub is not None and s_pub != pub:
            v_obj["publisher"] = s_pub
        if s_reg is not None:
            v_obj["region"] = s_reg
        if s_lang is not None:
            v_obj["language"] = s_lang
        if s_ver is not None and (len(shortnames) > 1 or s_ver != version):
            v_obj["version"] = s_ver
        if s_diffs:
            v_obj["differences"] = s_diffs

        if len(v_obj) > 1:
            variants.append(v_obj)

    # Assemble record in exact canonical key order
    rec = {
        "shortnames": shortnames,
        "title": title,
        "year": year,
        "developer": dev,
        "publisher": pub,
        "category": cat,
        "genre": genre,
        "platform": platform,
        "players": players,
        "release_status": rel_status,
        "version": version,
        "parent_shortname": parent_shortname,
        "original_game": original_game,
    }

    series_val = cur.get("series")
    if series_val:
        rec["series"] = series_val

    if variants:
        rec["variants"] = variants

    if cat == "hack":
        rec["hack_author"] = cur.get("hack_author", dev if p_info["manufacturer"].lower() not in ("bootleg", "bootleg?", "hack", "<unknown>", "unknown", "") and dev != parse_manufacturer(root_info["manufacturer"])[0] else None)
        if "hack_year" in cur and cur["hack_year"] != year:
            rec["hack_year"] = cur["hack_year"]
        if "modifications" in cur:
            rec["modifications"] = cur["modifications"]
        else:
            # Specific factual modification from description
            d_low = p_info["description"].lower()
            if "joystick hack" in d_low:
                rec["modifications"] = [{"type": "gameplay", "description": "Adapts the rotary/positional control scheme for standard 8-way joystick input."}]
            else:
                rec["modifications"] = []

    cur_desc = cur.get("description")
    if cur_desc is not None and cur_desc != "":
        rec["description"] = cur_desc
    else:
        ini_desc = descriptions_ini.get(primary) or next((descriptions_ini[s] for s in shortnames if s in descriptions_ini), None)
        rec["description"] = ini_desc
    rec["trivia"] = cur.get("trivia", [])
    rec["tips"] = cur.get("tips", [])
    rec["scoring"] = cur.get("scoring", [])

    rec["websites"] = cur.get("websites", [])

    sources = []
    if p_info["sourcefile"]:
        sources.append({
            "type": "source_code",
            "url": None,
            "path": f"mame/drivers/{p_info['sourcefile']}"
        })
    rec["sources"] = sources

    research_notes = []
    if year is None and primary not in bios_names:
        research_notes.append("Exact release year is not established.")
    if dev is None and pub is None and cat not in ("bootleg", "hack"):
        research_notes.append("Original developer and publisher are not established.")
    if p_info["description"].startswith("<unknown>"):
        research_notes.append("Original game title is unknown.")

    rec["confidence"] = cur.get("confidence", "verified" if not research_notes else "uncertain")
    rec["research_notes"] = research_notes

    return rec

records = [build_record(k, v) for k, v in groups.items()]
records.sort(key=lambda r: r["shortnames"][0])

db = {
    "schema_version": 1,
    "games": records
}

tmp_out = f"{args.output}.tmp.{os.getpid()}"
with open(tmp_out, "w", encoding="utf-8", newline="\n") as f:
    json.dump(db, f, indent=2, ensure_ascii=False)
    f.write("\n")
os.replace(tmp_out, args.output)

print(f"Wrote {len(records)} records covering {sum(len(r['shortnames']) for r in records)} shortnames to '{args.output}'.")

