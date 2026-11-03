#!/usr/bin/env python3
"""
Enrich ../games.json with verified metadata and reference websites from external/.

Assumes execution from the support/ directory:
- Target games catalog is located one level above at ../games.json.
- External reference databases are located in the child directory external/.

Behavior:
- Reads the target catalog (default: ../games.json) and offline databases in
  external/:
    * scraped_metadata.json (Wikidata, English Wikipedia, StrategyWiki)
    * launchbox_arcade.xml  (LaunchBox Arcade Game metadata)
    * launchbox_mame.xml    (LaunchBox MAME ROM mapping table: Region, Version, Language, Year)
    * tcrf-current.xml      (The Cutting Room Floor MediaWiki XML dump)
    * renameSET.dat         (Historical MAME shortname rename tracking)
    * series.ini            (Progetto-SNAPS game series classifications)
    * languages.ini         (Progetto-SNAPS in-game language classifications)
- Never overwrites existing non-empty fields in games.json and never adds duplicate entries.
- Does NOT touch or overwrite the curated "description" field.
- Automatically populates and enriches:
    1. websites:
       Adds verified, deduplicated reference URLs across all supported external domains:
         - English Wikipedia (https://en.wikipedia.org/wiki/...)
         - StrategyWiki (https://strategywiki.org/wiki/...)
         - The Cutting Room Floor / TCRF (https://tcrf.net/...)
         - The International Arcade Museum / KLOV (https://www.arcade-museum.com/game_detail.php?game_id=...)
         - Arcade Database / ADB (http://adb.arcadeitalia.net/dettaglio_mame.php?game_name=...&lang=en)
    2. Missing top-level scalar and array fields (only when currently null/empty in games.json):
         - year      (4-digit release year from LaunchBox MAME/Arcade)
         - developer (from LaunchBox Arcade/MAME)
         - publisher (from LaunchBox Arcade/MAME)
         - players   (max players integer from LaunchBox Arcade)
         - series    (franchise name from series.ini)
    3. Missing variant-level fields (only when currently null/missing on a variant):
         - region    (from launchbox_mame.xml <Region>)
         - language  (from languages.ini and launchbox_mame.xml <Language>)
         - version   (from launchbox_mame.xml <Version>)
    4. research_notes & confidence:
         - Removes unresolved research_notes entries ("Exact release year is not established.",
           "Original developer and publisher are not established.") once the corresponding
           field is populated, and updates confidence to "verified" when no unresolved notes remain.
- Preserves the exact canonical property ordering required by GAMES.md and writes atomically.

Command-line usage (run from support/):
    python3 enrich_games.py [options]

Options:
    -t, --target PATH         Target games.json file to read and update (default: ../games.json)
    -e, --external-dir PATH   Directory containing external databases (default: external)
    -g, --games SHORTNAME ... Only enrich specific game records matching the given shortnames
    -n, --dry-run             Preview enrichment statistics without modifying the target file
    --skip-tcrf-xml           Skip scanning the 812 MB tcrf-current.xml dump (use only Wikidata TCRF links)
    -v, --verbose             Print per-game enrichment details to stdout
"""

import argparse
import json
import os
import re
import sys
import urllib.parse
import xml.etree.ElementTree as ET


DEFAULT_TARGET = "../games.json"
DEFAULT_EXTERNAL_DIR = "external"

CANONICAL_GAME_KEYS = [
    "shortnames",
    "title",
    "year",
    "developer",
    "publisher",
    "category",
    "genre",
    "platform",
    "players",
    "release_status",
    "version",
    "parent_shortname",
    "original_game",
    "series",
    "variants",
    "hack_author",
    "hack_year",
    "modifications",
    "description",
    "trivia",
    "tips",
    "scoring",
    "websites",
    "sources",
    "confidence",
    "research_notes",
]

CANONICAL_VARIANT_KEYS = [
    "shortname",
    "parent_shortname",
    "title",
    "year",
    "developer",
    "publisher",
    "region",
    "language",
    "version",
    "differences",
    "websites",
]

SERIES_CLEANUP = {
    "The King of Fighters '94": "The King of Fighters",
    "Garou Densetsu": "Fatal Fury",
    "Samurai Spirits": "Samurai Shodown",
    "Bakumatsu Rouman": "The Last Blade",
    "Gouketsuji Ichizoku": "Power Instinct",
    "Sengoku Densyo": "Sengoku",
    "Mushihime Sama": "Mushihime-Sama",
    "R*Type": "R-Type",
    "Star Gladiator - Episode : I Final Crusade": "Star Gladiator",
    "Rival Schools - United by Fate": "Rival Schools",
    "Vapor Trail - Hyper Offence Formation": "Kuhga / Vapor Trail",
    "Alex Kidd In Miracle World": "Alex Kidd",
    "Prehistoric Isle in 1930": "Prehistoric Isle",
    "WWF Superstars": "WWF Arcade Wrestling",
    "Robotron: 2084": "Robotron",
    "Euro Champ '92": "Football Champ",
    "Gunlock": "RayForce",
    "Rayforce": "RayForce",
    "BaRaDuKe": "Baraduke",
}


def norm_title(text):
    """Normalize a game title for comparison."""
    if not text:
        return ""
    s = re.sub(r"\([^)]*\)", " ", str(text))
    s = re.sub(r"\[[^\]]*\]", " ", s)
    s = s.lower().replace("&", " and ")
    return re.sub(r"[^a-z0-9]+", "", s)


def clean_base_title(text):
    """Strip parenthetical suffixes and normalize whitespace."""
    if not text:
        return ""
    s = re.sub(r"\s*\([^)]*\)", "", str(text)).strip()
    return re.sub(r"\s+", " ", s)


def order_game_dict(rec):
    """Return a new dict with keys ordered according to CANONICAL_GAME_KEYS."""
    out = {}
    for k in CANONICAL_GAME_KEYS:
        if k in rec:
            out[k] = rec[k]
    for k, v in rec.items():
        if k not in out:
            out[k] = v
    return out


def order_variant_dict(var):
    """Return a new variant dict with keys ordered according to CANONICAL_VARIANT_KEYS."""
    out = {}
    for k in CANONICAL_VARIANT_KEYS:
        if k in var:
            out[k] = var[k]
    for k, v in var.items():
        if k not in out:
            out[k] = v
    return out


def atomic_save_json(path, data):
    """Atomically write data as formatted JSON to path."""
    tmp_path = f"{path}.tmp.{os.getpid()}"
    with open(tmp_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")
    os.replace(tmp_path, path)


def load_rename_map(external_dir):
    """Load renameSET.dat mapping old MAME shortnames to modern MAME shortnames."""
    path = os.path.join(external_dir, "renameSET.dat")
    rename_map = {}
    if not os.path.isfile(path):
        return rename_map
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.match(r"^([A-Za-z0-9_]+)\s+>\s+([A-Za-z0-9_]+)\s+\(mID:", line)
            if m:
                rename_map.setdefault(m.group(1), m.group(2))
    return rename_map


def resolve_modern_name(name, rename_map):
    """Resolve a shortname through renameSET.dat chain."""
    curr = name
    visited = set()
    while curr in rename_map and curr not in visited:
        visited.add(curr)
        curr = rename_map[curr]
    return curr


def load_ini_map(path):
    """Load an AntoPISA INI file ([Section] -> shortname -> Section)."""
    mapping = {}
    if not os.path.isfile(path):
        return mapping
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        cur_sec = None
        for line in f:
            l = line.strip()
            if not l or l.startswith(";"):
                continue
            if l.startswith("[") and l.endswith("]"):
                sec = l[1:-1].strip()
                cur_sec = None if sec.upper() in ("FOLDER_SETTINGS", "ROOT_FOLDER") else sec
            elif cur_sec:
                mapping[l] = cur_sec
    return mapping



SERIES_CLEANUP = {
    "The King of Fighters '94": "The King of Fighters",
    "Garou Densetsu": "Fatal Fury",
    "Samurai Spirits": "Samurai Shodown",
    "Bakumatsu Rouman": "The Last Blade",
    "Gouketsuji Ichizoku": "Power Instinct",
    "Sengoku Densyo": "Sengoku",
    "Mushihime Sama": "Mushihime-Sama",
    "R*Type": "R-Type",
    "Star Gladiator - Episode : I Final Crusade": "Star Gladiator",
    "Rival Schools - United by Fate": "Rival Schools",
    "Vapor Trail - Hyper Offence Formation": "Kuhga / Vapor Trail",
    "Alex Kidd In Miracle World": "Alex Kidd",
    "Prehistoric Isle in 1930": "Prehistoric Isle",
    "WWF Superstars": "WWF Arcade Wrestling",
    "Robotron: 2084": "Robotron",
    "Euro Champ '92": "Football Champ",
    "Gunlock": "RayForce",
    "Rayforce": "RayForce",
    "BaRaDuKe": "Baraduke",
}



def load_launchbox_data(external_dir):
    """Load LaunchBox Arcade and MAME XML tables."""
    lb_arcade_path = os.path.join(external_dir, "launchbox_arcade.xml")
    lb_mame_path = os.path.join(external_dir, "launchbox_mame.xml")

    arcade_by_norm = {}
    if os.path.isfile(lb_arcade_path):
        for _, elem in ET.iterparse(lb_arcade_path, events=("end",)):
            if elem.tag == "Game":
                name = (elem.findtext("Name") or "").strip()
                n = norm_title(name)
                if n:
                    rel_date = (elem.findtext("ReleaseDate") or "").strip()
                    year_val = int(rel_date[:4]) if len(rel_date) >= 4 and rel_date[:4].isdigit() else None
                    max_p = (elem.findtext("MaxPlayers") or "").strip()
                    players_val = int(max_p) if max_p.isdigit() and int(max_p) > 0 else None
                    dev = (elem.findtext("Developer") or "").strip() or None
                    pub = (elem.findtext("Publisher") or "").strip() or None
                    dbid = (elem.findtext("DatabaseID") or "").strip() or None
                    wp_url = (elem.findtext("WikipediaURL") or "").strip() or None
                    arcade_by_norm.setdefault(n, []).append(
                        {
                            "name": name,
                            "year": year_val,
                            "players": players_val,
                            "developer": dev,
                            "publisher": pub,
                            "database_id": dbid,
                            "wikipedia_url": wp_url,
                        }
                    )
                elem.clear()

    mame_by_sn = {}
    if os.path.isfile(lb_mame_path):
        for _, elem in ET.iterparse(lb_mame_path, events=("end",)):
            if elem.tag == "MameFile":
                fn = (elem.findtext("FileName") or "").strip()
                if fn:
                    y_str = (elem.findtext("Year") or "").strip()
                    y_val = int(y_str) if len(y_str) == 4 and y_str.isdigit() else None
                    dev = (elem.findtext("Developer") or "").strip() or None
                    pub = (elem.findtext("Publisher") or "").strip() or None
                    if dev and dev.lower() in ("<unknown>", "unknown", "bootleg", "hack"):
                        dev = None
                    if pub and pub.lower() in ("<unknown>", "unknown", "bootleg", "hack"):
                        pub = None
                    mame_by_sn[fn] = {
                        "name": (elem.findtext("Name") or "").strip() or None,
                        "cloneof": (elem.findtext("CloneOf") or "").strip() or None,
                        "year": y_val,
                        "developer": dev,
                        "publisher": pub,
                        "region": (elem.findtext("Region") or "").strip() or None,
                        "version": (elem.findtext("Version") or "").strip() or None,
                        "language": (elem.findtext("Language") or "").strip() or None,
                    }
                elem.clear()

    return arcade_by_norm, mame_by_sn


def load_tcrf_arcade_urls(external_dir, skip_tcrf_xml=False):
    """Scan support/external/tcrf-current.xml for Arcade pages and map normalized title -> TCRF URL."""
    tcrf_map = {}
    if skip_tcrf_xml:
        return tcrf_map
    xml_path = os.path.join(external_dir, "tcrf-current.xml")
    if not os.path.isfile(xml_path):
        return tcrf_map

    print("Scanning support/external/tcrf-current.xml for Arcade pages...", flush=True)
    ns = "{http://www.mediawiki.org/xml/export-0.10/}"
    sys_re = re.compile(r"\|\s*systems?\s*=\s*[^\n|}]{0,80}\bArcade\b", re.IGNORECASE)

    for _, elem in ET.iterparse(xml_path, events=("end",)):
        if elem.tag == ns + "page":
            page_ns = elem.findtext(ns + "ns")
            if page_ns == "0":
                title = (elem.findtext(ns + "title") or "").strip()
                if title and "/" not in title:
                    rev = elem.find(ns + "revision")
                    txt = (rev.findtext(ns + "text") if rev is not None else "") or ""
                    is_arcade_title = "(Arcade" in title
                    if is_arcade_title or sys_re.search(txt):
                        slug = urllib.parse.quote(title.replace(" ", "_"), safe="/:()!,'$~*")
                        url = f"https://tcrf.net/{slug}"
                        n = norm_title(title)
                        if n:
                            # Prefer explicit "... (Arcade)" page over generic page when both exist
                            if n not in tcrf_map or is_arcade_title:
                                tcrf_map[n] = url
            elem.clear()

    print(f"Indexed {len(tcrf_map)} Arcade game URLs from tcrf-current.xml.", flush=True)
    return tcrf_map


def normalize_url_for_dedup(url):
    """Normalize a URL for deduplication comparisons."""
    if not url:
        return ""
    u = url.strip()
    u = re.sub(r"^http://(en\.wikipedia\.org|strategywiki\.org|tcrf\.net|www\.arcade-museum\.com|gamesdb\.launchbox-app\.com)", r"https://\1", u)
    return u.rstrip("/")


def add_website(websites_list, w_type, url):
    """Append a website object to websites_list if not already present. Returns True if added."""
    if not url or not url.startswith(("http://", "https://")):
        return False
    norm_new = normalize_url_for_dedup(url)
    for existing in websites_list:
        if normalize_url_for_dedup(existing.get("url", "")) == norm_new:
            return False
    websites_list.append({"type": w_type, "url": url})
    return True


def website_sort_key(site):
    """Deterministic sort key for websites list: priority domain rank, then alphabetical URL."""
    url = site.get("url", "").lower()
    w_type = site.get("type", "")

    if w_type in ("official", "developer", "project"):
        rank = 0
    elif "en.wikipedia.org" in url:
        rank = 1
    elif "wikipedia.org" in url:
        rank = 2
    elif "strategywiki.org" in url:
        rank = 3
    elif "tcrf.net" in url:
        rank = 4
    elif "arcade-museum.com" in url:
        rank = 5
    elif "adb.arcadeitalia.net" in url:
        rank = 6
    else:
        rank = 7
    return (rank, url)


def main():
    parser = argparse.ArgumentParser(
        description="Enrich games.json with verified metadata and reference websites from support/external/ without overwriting existing data."
    )
    parser.add_argument(
        "-t",
        "--target",
        default=DEFAULT_TARGET,
        help="Target games.json file path (default: ../games.json)",
    )
    parser.add_argument(
        "-e",
        "--external-dir",
        default=DEFAULT_EXTERNAL_DIR,
        help="Path to external databases directory (default: external)",
    )
    parser.add_argument(
        "-g",
        "--games",
        nargs="+",
        help="Only enrich specific game records matching the given shortnames",
    )
    parser.add_argument(
        "-n",
        "--dry-run",
        action="store_true",
        help="Preview enrichment counts without modifying games.json",
    )
    parser.add_argument(
        "--skip-tcrf-xml",
        action="store_true",
        help="Skip scanning tcrf-current.xml (use only Wikidata TCRF links)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Print per-game enrichment details to stdout",
    )
    args = parser.parse_args()

    if not os.path.isfile(args.target):
        print(f"Error: target file '{args.target}' not found.", file=sys.stderr)
        sys.exit(1)

    with open(args.target, "r", encoding="utf-8") as f:
        db = json.load(f)
    games = db.get("games", [])

    scraped_path = os.path.join(args.external_dir, "scraped_metadata.json")
    scraped_db = {}
    if os.path.isfile(scraped_path):
        with open(scraped_path, "r", encoding="utf-8") as f:
            scraped_db = json.load(f)
        print(f"Loaded {len(scraped_db)} entries from '{scraped_path}'.", flush=True)

    print("Loading renameSET.dat, series.ini, and languages.ini...", flush=True)
    rename_map = load_rename_map(args.external_dir)
    series_map = load_ini_map(os.path.join(args.external_dir, "series.ini"))
    lang_map = load_ini_map(os.path.join(args.external_dir, "languages.ini"))

    print("Loading LaunchBox Arcade and MAME XML databases...", flush=True)
    lb_arcade_by_norm, lb_mame_by_sn = load_launchbox_data(args.external_dir)

    tcrf_arcade_map = load_tcrf_arcade_urls(args.external_dir, skip_tcrf_xml=args.skip_tcrf_xml)

    filter_set = set(args.games) if args.games else None

    stats = {
        "records_modified": 0,
        "websites_added": 0,
        "websites_by_domain": {
            "wikipedia": 0,
            "strategywiki": 0,
            "tcrf": 0,
            "klov": 0,
            "adb": 0,
        },
        "year_filled": 0,
        "developer_filled": 0,
        "publisher_filled": 0,
        "players_filled": 0,
        "series_filled": 0,
        "variant_region_filled": 0,
        "variant_language_filled": 0,
        "variant_version_filled": 0,
        "research_notes_resolved": 0,
    }

    for idx, rec in enumerate(games):
        snames = rec.get("shortnames", [])
        if not snames:
            continue
        if filter_set and not (filter_set & set(snames)):
            continue

        primary = snames[0]
        orig_game = rec.get("original_game")
        parent_sn = rec.get("parent_shortname")
        title = rec.get("title") or ""

        # Build candidate shortnames list (AdvanceMAME + modern renamed + parent/original)
        cand_snames = []
        for s in list(snames) + ([parent_sn] if parent_sn else []) + ([orig_game] if orig_game else []):
            if s and s not in cand_snames:
                cand_snames.append(s)
            ms = resolve_modern_name(s, rename_map) if s else None
            if ms and ms not in cand_snames:
                cand_snames.append(ms)

        # Find matching LaunchBox MAME entry and LaunchBox Arcade entry
        lb_mame_hit = next((lb_mame_by_sn[s] for s in cand_snames if s in lb_mame_by_sn), None)
        lb_arcade_hit = None
        title_candidates = [title, clean_base_title(title)]
        if lb_mame_hit and lb_mame_hit.get("name"):
            title_candidates.extend([lb_mame_hit["name"], clean_base_title(lb_mame_hit["name"])])
        for tc in title_candidates:
            nt = norm_title(tc)
            if nt and nt in lb_arcade_by_norm:
                lb_arcade_hit = lb_arcade_by_norm[nt][0]
                break

        scraped_entry = scraped_db.get(primary, {})
        wd = scraped_entry.get("wikidata", {}) if scraped_entry.get("wikidata", {}).get("status") == "found" else {}
        wp = scraped_entry.get("wikipedia", {}) if scraped_entry.get("wikipedia", {}).get("status") == "found" else {}
        sw = scraped_entry.get("strategywiki", {}) if scraped_entry.get("strategywiki", {}).get("status") == "found" else {}

        modified_this_rec = False
        changes_log = []

        # 1. Fill missing top-level scalar fields (never overwrite non-null)
        if rec.get("year") is None:
            new_year = (lb_mame_hit.get("year") if lb_mame_hit else None) or (
                lb_arcade_hit.get("year") if lb_arcade_hit else None
            )
            if new_year is not None:
                rec["year"] = new_year
                stats["year_filled"] += 1
                modified_this_rec = True
                changes_log.append(f"year={new_year}")

        if rec.get("developer") is None:
            new_dev = (lb_arcade_hit.get("developer") if lb_arcade_hit else None) or (
                lb_mame_hit.get("developer") if lb_mame_hit else None
            )
            if new_dev:
                rec["developer"] = new_dev
                stats["developer_filled"] += 1
                modified_this_rec = True
                changes_log.append(f"developer={new_dev}")

        if rec.get("publisher") is None:
            new_pub = (lb_arcade_hit.get("publisher") if lb_arcade_hit else None) or (
                lb_mame_hit.get("publisher") if lb_mame_hit else None
            )
            if not new_pub and rec.get("developer"):
                new_pub = rec["developer"]
            if new_pub:
                rec["publisher"] = new_pub
                stats["publisher_filled"] += 1
                modified_this_rec = True
                changes_log.append(f"publisher={new_pub}")

        if rec.get("players") is None and lb_arcade_hit and lb_arcade_hit.get("players"):
            rec["players"] = lb_arcade_hit["players"]
            stats["players_filled"] += 1
            modified_this_rec = True
            changes_log.append(f"players={rec['players']}")

        if not rec.get("series"):
            ser_cand = None
            for cs in cand_snames:
                if cs in series_map:
                    raw_ser = series_map[cs]
                    ser_cand = SERIES_CLEANUP.get(raw_ser, raw_ser)
                    break
            if ser_cand:
                rec["series"] = ser_cand
                stats["series_filled"] += 1
                modified_this_rec = True
                changes_log.append(f"series={rec['series']}")

        # 2. Enrich existing variants (region, language, version) where missing
        if "variants" in rec and isinstance(rec["variants"], list):
            new_variants = []
            for v in rec["variants"]:
                v_sn = v.get("shortname")
                v_cands = [v_sn]
                if v_sn:
                    mv = resolve_modern_name(v_sn, rename_map)
                    if mv and mv not in v_cands:
                        v_cands.append(mv)
                v_lb = next((lb_mame_by_sn[s] for s in v_cands if s and s in lb_mame_by_sn), None)

                if not v.get("region") and v_lb and v_lb.get("region"):
                    v["region"] = v_lb["region"]
                    stats["variant_region_filled"] += 1
                    modified_this_rec = True

                if not v.get("language"):
                    v_lang = next((lang_map[s] for s in v_cands if s and s in lang_map), None)
                    if not v_lang and v_lb and v_lb.get("language"):
                        v_lang = v_lb["language"]
                    if v_lang:
                        v["language"] = v_lang
                        stats["variant_language_filled"] += 1
                        modified_this_rec = True

                if not v.get("version") and v_lb and v_lb.get("version"):
                    v["version"] = v_lb["version"]
                    stats["variant_version_filled"] += 1
                    modified_this_rec = True

                new_variants.append(order_variant_dict(v))
            rec["variants"] = new_variants

        # 3. Enrich websites across all verified external domains (deduplicated)
        websites = list(rec.get("websites") or [])

        # 3a. Wikipedia
        wp_url = wp.get("url") or wd.get("wikipedia_url") or (lb_arcade_hit.get("wikipedia_url") if lb_arcade_hit else None)
        if wp_url and add_website(websites, "reference", wp_url):
            stats["websites_added"] += 1
            stats["websites_by_domain"]["wikipedia"] += 1
            modified_this_rec = True
            changes_log.append("+wp")

        # 3b. StrategyWiki
        sw_url = sw.get("url") or wd.get("strategywiki_url")
        if sw_url and add_website(websites, "reference", sw_url):
            stats["websites_added"] += 1
            stats["websites_by_domain"]["strategywiki"] += 1
            modified_this_rec = True
            changes_log.append("+sw")

        # 3c. The Cutting Room Floor (TCRF)
        tcrf_url = wd.get("tcrf_url")
        if not tcrf_url and tcrf_arcade_map:
            for tc in title_candidates:
                nt = norm_title(tc)
                if nt and nt in tcrf_arcade_map:
                    tcrf_url = tcrf_arcade_map[nt]
                    break
        if tcrf_url and add_website(websites, "reference", tcrf_url):
            stats["websites_added"] += 1
            stats["websites_by_domain"]["tcrf"] += 1
            modified_this_rec = True
            changes_log.append("+tcrf")

        # 3d. KLOV / The International Arcade Museum
        klov_id = wd.get("klov_id")
        if klov_id:
            klov_url = f"https://www.arcade-museum.com/game_detail.php?game_id={klov_id}"
            if add_website(websites, "reference", klov_url):
                stats["websites_added"] += 1
                stats["websites_by_domain"]["klov"] += 1
                modified_this_rec = True
                changes_log.append("+klov")

        # 3e. Arcade Database (ADB)
        adb_sn = wd.get("mame_id")
        if not adb_sn:
            # If the primary shortname or its modern rename is a known MAME set in LaunchBox MAME, link ADB
            adb_sn = next((s for s in (primary, resolve_modern_name(primary, rename_map)) if s in lb_mame_by_sn), None)
        if adb_sn:
            adb_url = f"http://adb.arcadeitalia.net/dettaglio_mame.php?game_name={adb_sn}&lang=en"
            if add_website(websites, "reference", adb_url):
                stats["websites_added"] += 1
                stats["websites_by_domain"]["adb"] += 1
                modified_this_rec = True
                changes_log.append("+adb")

        if websites:
            orig_order = [w.get("url") for w in (rec.get("websites") or [])]
            websites.sort(key=website_sort_key)
            if [w.get("url") for w in websites] != orig_order:
                modified_this_rec = True
            rec["websites"] = websites
        else:
            rec["websites"] = []

        # 4. Update research_notes if previously missing year or developer/publisher are now resolved
        r_notes = list(rec.get("research_notes") or [])
        new_notes = []
        for note in r_notes:
            if note == "Exact release year is not established." and rec.get("year") is not None:
                stats["research_notes_resolved"] += 1
                modified_this_rec = True
                continue
            if (
                note == "Original developer and publisher are not established."
                and (rec.get("developer") is not None or rec.get("publisher") is not None)
            ):
                stats["research_notes_resolved"] += 1
                modified_this_rec = True
                continue
            new_notes.append(note)
        rec["research_notes"] = new_notes
        if not new_notes and rec.get("confidence") == "uncertain":
            rec["confidence"] = "verified"

        games[idx] = order_game_dict(rec)

        if modified_this_rec:
            stats["records_modified"] += 1
            if args.verbose:
                print(f"[{idx + 1}/{len(games)}] {primary}: {', '.join(changes_log)}")

    print("\nEnrichment Summary:")
    print(f"  Records modified:          {stats['records_modified']} / {len(games)}")
    print(f"  Total websites added:      {stats['websites_added']}")
    for dom, cnt in stats["websites_by_domain"].items():
        print(f"    - {dom:14s}:        {cnt}")
    print(f"  Missing year filled:       {stats['year_filled']}")
    print(f"  Missing developer filled:  {stats['developer_filled']}")
    print(f"  Missing publisher filled:  {stats['publisher_filled']}")
    print(f"  Missing players filled:    {stats['players_filled']}")
    print(f"  Missing series filled:     {stats['series_filled']}")
    print(f"  Variant region filled:     {stats['variant_region_filled']}")
    print(f"  Variant language filled:   {stats['variant_language_filled']}")
    print(f"  Variant version filled:    {stats['variant_version_filled']}")
    print(f"  Research notes resolved:   {stats['research_notes_resolved']}")

    if args.dry_run:
        print("\nDry run complete; target file was not modified.")
    else:
        atomic_save_json(args.target, db)
        print(f"\nSaved enriched database to '{args.target}'.")


if __name__ == "__main__":
    main()
