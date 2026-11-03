#!/usr/bin/env python3
"""
Scrape and cache external arcade game metadata into external/scraped_metadata.json.

Assumes execution from the support/ directory:
- Target games catalog is located one level above at ../games.json.
- External reference files are located in the child directory external/.

Behavior:
- Reads the target game list from ../games.json (default).
- Enriches search hints using local files in external/ (launchbox_arcade.xml,
  launchbox_mame.xml, renameSET.dat) if present, plus bulk Wikidata SPARQL mappings
  (MAME ROM P2451, TCRF P8607, StrategyWiki P9075, KLOV P2858, English Wikipedia sitelinks).
- Queries external APIs incrementally per game for missing sources:
    * wikidata     (Wikidata QID, MAME ID, Wikipedia URL, StrategyWiki URL, TCRF URL, KLOV ID)
    * wikipedia    (English Wikipedia URL, lead summary, gameplay section)
    * strategywiki (StrategyWiki URL, lead overview, gameplay subpage/section)
- Writes progress to stdout for every processed game.
- Resumable: any source already marked with status "found" or "not_found" in
  external/scraped_metadata.json is skipped on subsequent runs (unless --refresh
  is passed). Sources marked "error" or missing are automatically retried.
- Safe Ctrl+C (SIGINT / SIGTERM) handling: catches interrupts, saves the current state
  atomically to external/scraped_metadata.json via a temporary file, and exits
  cleanly without corrupting the JSON file. Also checkpoints atomically after every game.

Command-line usage (run from support/):
    python3 scrape_external.py [options]

Options:
    -i, --input PATH       Input games.json path (default: ../games.json)
    -o, --output PATH      Output scraped JSON path (default: external/scraped_metadata.json)
    -g, --games KEY ...    Only process specific game shortnames (e.g., -g gng raiden2)
    -l, --limit N          Stop after updating at most N games in this run
    --sources SRC ...      Only query specific sources: wikidata, wikipedia, strategywiki
    --refresh              Re-fetch even if a source is already marked "found" or "not_found"
    --retry-not-found      Re-attempt sources previously marked "not_found"
    -d, --delay SECONDS    Pause in seconds between every HTTP request (default: 1.0)
"""

import argparse
import html
import json
import os
import re
import signal
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET


USER_AGENT = "AdvanceMAME-Metadata/1.0 (https://www.advancemame.it; metadata research)"
ALL_SOURCES = ("wikidata", "wikipedia", "strategywiki")

REQUEST_DELAY = 1.0
_LAST_HTTP_TS = 0.0
STOP_REQUESTED = False


def norm_title(text):
    """Normalize a game title for fuzzy comparison."""
    if not text:
        return ""
    s = re.sub(r"\([^)]*\)", " ", str(text))
    s = s.lower()
    s = s.replace("&", " and ")
    return re.sub(r"[^a-z0-9]+", "", s)


def clean_base_title(text):
    """Strip parenthetical suffixes and normalize whitespace."""
    if not text:
        return ""
    s = re.sub(r"\s*\([^)]*\)", "", str(text)).strip()
    return re.sub(r"\s+", " ", s)


def atomic_save_json(path, data):
    """Atomically write data as formatted JSON to path."""
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    tmp_path = f"{path}.tmp.{os.getpid()}"
    with open(tmp_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")
    os.replace(tmp_path, path)


def throttle_http():
    """Enforce the configured pause (REQUEST_DELAY) between consecutive HTTP requests."""
    global _LAST_HTTP_TS
    if REQUEST_DELAY > 0 and _LAST_HTTP_TS > 0:
        elapsed = time.monotonic() - _LAST_HTTP_TS
        remaining = REQUEST_DELAY - elapsed
        if remaining > 0:
            # Sleep in small increments so Ctrl+C is responsive during the pause
            end_time = time.monotonic() + remaining
            while time.monotonic() < end_time and not STOP_REQUESTED:
                time.sleep(min(0.1, end_time - time.monotonic()))
    _LAST_HTTP_TS = time.monotonic()


def http_get_json(url, timeout=15, retries=2):
    """Perform an HTTP GET request with rate-limiting and parse JSON response."""
    last_err = None
    for attempt in range(retries + 1):
        if STOP_REQUESTED:
            raise InterruptedError("Interrupted by user signal")
        throttle_http()
        if STOP_REQUESTED:
            raise InterruptedError("Interrupted by user signal")
        try:
            req = urllib.request.Request(
                url,
                headers={
                    "User-Agent": USER_AGENT,
                    "Accept": "application/json",
                },
            )
            with urllib.request.urlopen(req, timeout=timeout) as resp:
                raw = resp.read().decode("utf-8", errors="replace")
                return json.loads(raw)
        except urllib.error.HTTPError as e:
            if e.code == 404:
                return None
            last_err = e
            if e.code == 429 and attempt < retries:
                time.sleep(2.0 * (attempt + 1))
                continue
            if attempt < retries:
                time.sleep(0.8 * (attempt + 1))
        except Exception as e:
            last_err = e
            if attempt < retries:
                time.sleep(0.8 * (attempt + 1))
    raise RuntimeError(f"HTTP request failed for {url}: {last_err}")


def strip_wikitext(wikitext):
    """Convert MediaWiki markup into readable plain text paragraphs."""
    if not wikitext:
        return ""
    text = wikitext
    # Remove HTML comments
    text = re.sub(r"<!--.*?-->", "", text, flags=re.DOTALL)
    # Remove <ref>...</ref>, self-closing <ref .../>, and <gallery>...</gallery>
    text = re.sub(r"<ref[^>]*/>", "", text, flags=re.IGNORECASE)
    text = re.sub(r"<ref[^>]*>.*?</ref>", "", text, flags=re.IGNORECASE | re.DOTALL)
    text = re.sub(r"<gallery[^>]*>.*?</gallery>", "", text, flags=re.IGNORECASE | re.DOTALL)
    # Iteratively strip nested templates {{...}}
    prev = None
    while prev != text:
        prev = text
        text = re.sub(r"\{\{[^{}]*\}\}", "", text)
    # Remove tables {| ... |}
    text = re.sub(r"\{\|.*?\|\}", "", text, flags=re.DOTALL)
    # Remove File/Image links [[File:...|...]]
    text = re.sub(r"\[\[(?:File|Image|Category):[^\]]*\]\]", "", text, flags=re.IGNORECASE)
    # Convert [[target|label]] -> label and [[target]] -> target
    text = re.sub(r"\[\[([^|\]]+)\|([^\]]+)\]\]", r"\2", text)
    text = re.sub(r"\[\[([^\]]+)\]\]", r"\1", text)
    # Convert external links [http://... label] -> label
    text = re.sub(r"\[https?://[^\s\]]+\s+([^\]]+)\]", r"\1", text)
    text = re.sub(r"\[https?://[^\]]+\]", "", text)
    # Strip bold/italic quotes
    text = re.sub(r"'{2,5}", "", text)
    # Strip remaining HTML tags
    text = re.sub(r"<[^>]+>", "", text)
    text = html.unescape(text)

    lines = []
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            lines.append("")
            continue
        if line.startswith("=="):
            continue
        line = re.sub(r"^[*:;#]+\s*", "- ", line)
        line = re.sub(r"\s+", " ", line).strip()
        lines.append(line)

    out = "\n".join(lines)
    out = re.sub(r"\n{3,}", "\n\n", out).strip()
    return out


def extract_wp_sections(full_extract):
    """Split a plain-text Wikipedia extract into (summary, gameplay)."""
    if not full_extract:
        return "", ""
    parts = re.split(r"\n(?===+\s*[^=]+\s*==+)", full_extract)
    summary = parts[0].strip()
    gameplay = ""
    for part in parts[1:]:
        m = re.match(r"^==+\s*([^=]+?)\s*==+\s*\n(.*)$", part, flags=re.DOTALL)
        if not m:
            continue
        heading = m.group(1).strip().lower()
        body = m.group(2).strip()
        if heading in ("gameplay", "overview", "synopsis", "plot and gameplay"):
            # Stop at next top-level == Heading == if any
            sub = re.split(r"\n==\s+[^=]+\s+==", body)[0].strip()
            if sub:
                gameplay = sub
                break
    return summary, gameplay


def is_likely_video_game_extract(text, year=None, manufacturer=None):
    """Check whether a Wikipedia extract describes a video/arcade game."""
    if not text:
        return False
    head = text[:1500].lower()
    if "may refer to:" in head or "most commonly refers to:" in head:
        return False
    vg_keywords = (
        "video game",
        "arcade game",
        "arcade",
        "shoot 'em up",
        "shooter",
        "platform game",
        "platformer",
        "fighting game",
        "beat 'em up",
        "puzzle game",
        "racing game",
        "sports game",
        "scrolling",
        "coin-operated",
        "mahjong",
        "pinball",
        "quiz game",
        "light gun",
        "developed by",
        "published by",
        "namco",
        "capcom",
        "konami",
        "taito",
        "sega",
        "irem",
        "data east",
        "snk",
        "atari",
        "midway",
        "williams",
        "nintendo",
        "jaleco",
        "kaneko",
        "toaplan",
        "psikyo",
        "cave",
        "raizing",
        "visco",
        "nichibutsu",
        "tecmo",
        "banpresto",
    )
    return any(k in head for k in vg_keywords)


def load_launchbox_hints(external_dir):
    """Load LaunchBox Arcade and MAME hints from support/external/ if available."""
    lb_arcade_path = os.path.join(external_dir, "launchbox_arcade.xml")
    lb_mame_path = os.path.join(external_dir, "launchbox_mame.xml")

    by_norm_name = {}
    if os.path.isfile(lb_arcade_path):
        try:
            for _, elem in ET.iterparse(lb_arcade_path, events=("end",)):
                if elem.tag == "Game":
                    name = (elem.findtext("Name") or "").strip()
                    n = norm_title(name)
                    if n:
                        entry = {
                            "name": name,
                            "overview": (elem.findtext("Overview") or "").strip(),
                            "wikipedia_url": (elem.findtext("WikipediaURL") or "").strip(),
                            "video_url": (elem.findtext("VideoURL") or "").strip(),
                            "database_id": (elem.findtext("DatabaseID") or "").strip(),
                            "developer": (elem.findtext("Developer") or "").strip(),
                            "publisher": (elem.findtext("Publisher") or "").strip(),
                            "release_date": (elem.findtext("ReleaseDate") or "").strip(),
                        }
                        by_norm_name.setdefault(n, []).append(entry)
                    elem.clear()
        except Exception as e:
            print(f"[WARN] Failed reading {lb_arcade_path}: {e}", file=sys.stderr)

    mame_sn_to_name = {}
    if os.path.isfile(lb_mame_path):
        try:
            for _, elem in ET.iterparse(lb_mame_path, events=("end",)):
                if elem.tag == "MameFile":
                    fn = (elem.findtext("FileName") or "").strip()
                    name = (elem.findtext("Name") or "").strip()
                    cloneof = (elem.findtext("CloneOf") or "").strip()
                    if fn and name:
                        mame_sn_to_name[fn] = (name, cloneof)
                    elem.clear()
        except Exception as e:
            print(f"[WARN] Failed reading {lb_mame_path}: {e}", file=sys.stderr)

    return by_norm_name, mame_sn_to_name


def fetch_bulk_wikidata_mame_map():
    """Fetch all Wikidata items with P2451 (MAME ROM) or P8607 (TCRF) in one SPARQL query."""
    query = """
SELECT ?item ?itemLabel ?mame ?tcrf ?strategywiki ?klov ?sitelink WHERE {
  ?item wdt:P2451 ?mame .
  OPTIONAL { ?item wdt:P8607 ?tcrf . }
  OPTIONAL { ?item wdt:P9075 ?strategywiki . }
  OPTIONAL { ?item wdt:P2858 ?klov . }
  OPTIONAL {
    ?sitelink schema:about ?item ;
              schema:isPartOf <https://en.wikipedia.org/> .
  }
  SERVICE wikibase:label { bd:serviceParam wikibase:language "en". }
}
"""
    url = "https://query.wikidata.org/sparql?format=json&query=" + urllib.parse.quote(query)
    by_mame = {}
    try:
        data = http_get_json(url, timeout=30, retries=2)
        if not data:
            return by_mame
        for row in data.get("results", {}).get("bindings", []):
            mame = row.get("mame", {}).get("value", "").strip()
            if not mame:
                continue
            uri = row.get("item", {}).get("value", "")
            qid = uri.rsplit("/", 1)[-1] if uri else ""
            label = row.get("itemLabel", {}).get("value", "")
            tcrf_id = row.get("tcrf", {}).get("value", "")
            sw_id = row.get("strategywiki", {}).get("value", "")
            klov_id = row.get("klov", {}).get("value", "")
            wp_url = row.get("sitelink", {}).get("value", "")

            rec = by_mame.setdefault(
                mame,
                {
                    "qid": qid,
                    "label": label,
                    "mame_id": mame,
                    "wikipedia_url": None,
                    "strategywiki_url": None,
                    "tcrf_url": None,
                    "klov_id": None,
                },
            )
            if wp_url and not rec["wikipedia_url"]:
                rec["wikipedia_url"] = wp_url
            if sw_id and not rec["strategywiki_url"]:
                rec["strategywiki_url"] = f"https://strategywiki.org/wiki/{sw_id.replace(' ', '_')}"
            if tcrf_id and not rec["tcrf_url"]:
                # Prefer Arcade or general TCRF pages over console-specific ones when multiple exist
                candidate_tcrf = f"https://tcrf.net/{tcrf_id.replace(' ', '_')}"
                if not rec["tcrf_url"] or "Arcade" in tcrf_id:
                    rec["tcrf_url"] = candidate_tcrf
            if klov_id and not rec["klov_id"]:
                rec["klov_id"] = klov_id
    except Exception as e:
        print(f"[WARN] Bulk Wikidata SPARQL query failed ({e}); falling back to per-game API.", file=sys.stderr)
    return by_mame


def query_wikidata_by_wp_title(wp_title):
    """Look up Wikidata entity claims (P8607 TCRF, P9075 StrategyWiki, P2858 KLOV) from an enwiki title."""
    params = urllib.parse.urlencode(
        {
            "action": "wbgetentities",
            "sites": "enwiki",
            "titles": wp_title,
            "props": "claims|sitelinks|labels",
            "languages": "en",
            "format": "json",
        }
    )
    url = f"https://www.wikidata.org/w/api.php?{params}"
    data = http_get_json(url)
    if not data or "entities" not in data:
        return None
    for qid, ent in data["entities"].items():
        if qid == "-1" or "missing" in ent:
            continue
        claims = ent.get("claims", {})

        def first_claim_val(prop):
            vals = []
            for c in claims.get(prop, []):
                dv = c.get("mainsnak", {}).get("datavalue", {})
                if dv.get("type") == "string":
                    vals.append(dv.get("value"))
            return vals

        tcrf_vals = first_claim_val("P8607")
        sw_vals = first_claim_val("P9075")
        klov_vals = first_claim_val("P2858")
        mame_vals = first_claim_val("P2451")

        tcrf_id = None
        for v in tcrf_vals:
            if "Arcade" in v:
                tcrf_id = v
                break
        if not tcrf_id and tcrf_vals:
            # Avoid linking a pure console page if it explicitly says (NES), (SNES), (PlayStation), etc.
            non_console = [
                v
                for v in tcrf_vals
                if not re.search(
                    r"\((?:NES|SNES|Genesis|Mega_Drive|PlayStation|Saturn|Game_Boy|MSX|Commodore|Atari|ZX|Amiga|PC|DOS)",
                    v,
                    re.I,
                )
            ]
            if non_console:
                tcrf_id = non_console[0]

        enwiki_title = ent.get("sitelinks", {}).get("enwiki", {}).get("title")
        wp_url = (
            f"https://en.wikipedia.org/wiki/{ urllib.parse.quote(enwiki_title.replace(' ', '_'), safe='/:()!,$~*') }"
            if enwiki_title
            else None
        )
        label = ent.get("labels", {}).get("en", {}).get("value", enwiki_title or "")
        return {
            "status": "found",
            "qid": qid,
            "label": label,
            "mame_id": mame_vals[0] if mame_vals else None,
            "wikipedia_url": wp_url,
            "strategywiki_url": f"https://strategywiki.org/wiki/{sw_vals[0].replace(' ', '_')}" if sw_vals else None,
            "tcrf_url": f"https://tcrf.net/{tcrf_id.replace(' ', '_')}" if tcrf_id else None,
            "klov_id": klov_vals[0] if klov_vals else None,
        }
    return None


def fetch_wikipedia_by_title(title):
    """Fetch Wikipedia page extract and URL given an exact or redirected title."""
    params = urllib.parse.urlencode(
        {
            "action": "query",
            "prop": "extracts|info|pageprops",
            "inprop": "url",
            "explaintext": "1",
            "redirects": "1",
            "titles": title,
            "format": "json",
        }
    )
    url = f"https://en.wikipedia.org/w/api.php?{params}"
    data = http_get_json(url)
    if not data or "query" not in data:
        return None
    pages = data["query"].get("pages", {})
    for pid, page in pages.items():
        if pid == "-1" or "missing" in page:
            continue
        if "disambiguation" in page.get("pageprops", {}):
            continue
        extract = (page.get("extract") or "").strip()
        if not extract:
            continue
        summary, gameplay = extract_wp_sections(extract)
        return {
            "title": page.get("title", title),
            "url": page.get("fullurl", f"https://en.wikipedia.org/wiki/{title.replace(' ', '_')}"),
            "summary": summary,
            "gameplay": gameplay,
            "full_extract": extract,
            "wikibase_item": page.get("pageprops", {}).get("wikibase_item"),
        }
    return None


def search_wikipedia_game(game, candidate_titles):
    """Search English Wikipedia for an arcade game and return its structured data."""
    year = game.get("year")
    manufacturer = game.get("manufacturer")

    # 1. Try direct candidate titles with (video game) / (arcade game) and exact title
    for base in candidate_titles:
        if not base:
            continue
        for variant in (f"{base} (video game)", f"{base} (arcade game)", base):
            res = fetch_wikipedia_by_title(variant)
            if res and is_likely_video_game_extract(res["full_extract"], year, manufacturer):
                resNorm = norm_title(res["title"])
                baseNorm = norm_title(base)
                # Verify title similarity or mention of base title in lead paragraph
                if (
                    resNorm == baseNorm
                    or baseNorm in resNorm
                    or base.lower() in res["summary"][:400].lower()
                ):
                    return {
                        "status": "found",
                        "title": res["title"],
                        "url": res["url"],
                        "summary": res["summary"],
                        "gameplay": res["gameplay"] or None,
                    }

    # 2. Fallback: Wikipedia search API with arcade/video game keywords
    primary_title = clean_base_title(game.get("title") or "")
    if primary_title:
        q = f'"{primary_title}" arcade video game'
        if year:
            q += f" {year}"
        params = urllib.parse.urlencode(
            {
                "action": "query",
                "list": "search",
                "srsearch": q,
                "srlimit": "3",
                "format": "json",
            }
        )
        sdata = http_get_json(f"https://en.wikipedia.org/w/api.php?{params}")
        if sdata and "query" in sdata:
            cand_norms = {norm_title(t) for t in candidate_titles if t}
            for hit in sdata["query"].get("search", []):
                htitle = hit.get("title", "")
                if norm_title(htitle) in cand_norms:
                    res = fetch_wikipedia_by_title(htitle)
                    if res and is_likely_video_game_extract(res["full_extract"], year, manufacturer):
                        return {
                            "status": "found",
                            "title": res["title"],
                            "url": res["url"],
                            "summary": res["summary"],
                            "gameplay": res["gameplay"] or None,
                        }

    return {"status": "not_found"}


def fetch_strategywiki_page(page_title):
    """Fetch a StrategyWiki page and its optional /Gameplay subpage."""
    params = urllib.parse.urlencode(
        {
            "action": "parse",
            "page": page_title,
            "prop": "wikitext",
            "redirects": "1",
            "format": "json",
        }
    )
    url = f"https://strategywiki.org/w/api.php?{params}"
    data = http_get_json(url)
    if not data or "error" in data or "parse" not in data:
        return None
    parse = data["parse"]
    resolved_title = parse.get("title", page_title)
    if ":" in resolved_title and not resolved_title.lower().startswith(page_title.lower()[:4]):
        # Avoid Category: or Template: redirects
        return None
    wikitext = parse.get("wikitext", {}).get("*", "")
    if not wikitext or "{{disambig" in wikitext.lower():
        return None

    overview = strip_wikitext(wikitext)
    if not overview:
        return None

    # Check if there is a /Gameplay subpage
    gameplay = None
    gp_params = urllib.parse.urlencode(
        {
            "action": "parse",
            "page": f"{resolved_title}/Gameplay",
            "prop": "wikitext",
            "redirects": "1",
            "format": "json",
        }
    )
    gp_data = http_get_json(f"https://strategywiki.org/w/api.php?{gp_params}")
    if gp_data and "parse" in gp_data:
        gp_wt = gp_data["parse"].get("wikitext", {}).get("*", "")
        if gp_wt:
            gp_clean = strip_wikitext(gp_wt)
            if gp_clean and gp_clean != overview:
                gameplay = gp_clean

    page_slug = urllib.parse.quote(resolved_title.replace(" ", "_"), safe="/:()!,'$~*")
    return {
        "status": "found",
        "title": resolved_title,
        "url": f"https://strategywiki.org/wiki/{page_slug}",
        "overview": overview,
        "gameplay": gameplay,
    }


def search_strategywiki_game(candidate_titles, sw_url_hint=None):
    """Find a game on StrategyWiki using Wikidata hint or candidate titles."""
    tried = set()
    candidates = []
    if sw_url_hint and "/wiki/" in sw_url_hint:
        slug = urllib.parse.unquote(sw_url_hint.split("/wiki/", 1)[1]).replace("_", " ")
        candidates.append(slug)
    for t in candidate_titles:
        if t:
            candidates.append(t)

    for cand in candidates:
        cand_clean = cand.strip()
        if not cand_clean or cand_clean.lower() in tried:
            continue
        tried.add(cand_clean.lower())
        res = fetch_strategywiki_page(cand_clean)
        if res:
            return res
    return {"status": "not_found"}


def needs_fetch(entry, source, refresh=False, retry_not_found=False):
    """Determine if a specific source needs to be fetched for this game entry."""
    if refresh:
        return True
    src_obj = entry.get(source)
    if not isinstance(src_obj, dict):
        return True
    status = src_obj.get("status")
    if status == "found":
        return False
    if status == "not_found":
        return bool(retry_not_found)
    return True


DEFAULT_INPUT = "../games.json"
DEFAULT_OUTPUT = "external/scraped_metadata.json"


def main():
    parser = argparse.ArgumentParser(
        description="Incrementally scrape and cache external metadata for ../games.json into external/scraped_metadata.json."
    )
    parser.add_argument(
        "-i",
        "--input",
        default=DEFAULT_INPUT,
        help="Input games.json file path (default: ../games.json)",
    )
    parser.add_argument(
        "-o",
        "--output",
        default=DEFAULT_OUTPUT,
        help="Output scraped metadata JSON path (default: external/scraped_metadata.json)",
    )
    parser.add_argument(
        "-g",
        "--games",
        nargs="+",
        help="Process only the specified game shortnames (matches any shortname in a group)",
    )
    parser.add_argument(
        "-l",
        "--limit",
        type=int,
        default=0,
        help="Stop after updating at most N games (0 = no limit)",
    )
    parser.add_argument(
        "--sources",
        nargs="+",
        choices=ALL_SOURCES,
        default=list(ALL_SOURCES),
        help="Sources to fetch (default: wikidata wikipedia strategywiki)",
    )
    parser.add_argument(
        "--refresh",
        action="store_true",
        help="Re-fetch sources even if already marked found or not_found",
    )
    parser.add_argument(
        "--retry-not-found",
        action="store_true",
        help="Retry sources previously marked not_found",
    )
    parser.add_argument(
        "-d",
        "--delay",
        type=float,
        default=1.0,
        help="Pause in seconds between every HTTP query (default: 1.0)",
    )
    args = parser.parse_args()

    global REQUEST_DELAY, STOP_REQUESTED
    REQUEST_DELAY = max(0.0, args.delay)

    if not os.path.isfile(args.input):
        print(f"Error: input file '{args.input}' not found.", file=sys.stderr)
        sys.exit(1)

    with open(args.input, "r", encoding="utf-8") as f:
        games_doc = json.load(f)
    games_list = games_doc.get("games", [])

    # Load existing scraped_metadata.json if present
    scraped_db = {}
    if os.path.isfile(args.output):
        try:
            with open(args.output, "r", encoding="utf-8") as f:
                loaded = json.load(f)
                if isinstance(loaded, dict):
                    scraped_db = loaded
            print(f"Loaded existing cache '{args.output}' with {len(scraped_db)} game entries.", flush=True)
        except Exception as e:
            print(f"Error reading existing '{args.output}': {e}", file=sys.stderr)
            sys.exit(1)

    # Set up graceful interrupt handling so Ctrl+C never corrupts the output file
    def handle_sigint(signum, frame):
        global STOP_REQUESTED
        STOP_REQUESTED = True
        print("\n[INTERRUPT] Signal received, saving progress safely and exiting...", flush=True)

    signal.signal(signal.SIGINT, handle_sigint)
    signal.signal(signal.SIGTERM, handle_sigint)

    external_dir = os.path.dirname(os.path.abspath(args.output))
    print("Loading local hints from support/external/ (LaunchBox XML)...", flush=True)
    lb_by_norm, lb_mame_sn = load_launchbox_hints(external_dir)

    print("Fetching bulk MAME/TCRF/StrategyWiki/Wikipedia index from Wikidata SPARQL...", flush=True)
    wd_mame_map = fetch_bulk_wikidata_mame_map() if not STOP_REQUESTED else {}
    print(f"Indexed {len(wd_mame_map)} MAME ROM mappings from Wikidata.", flush=True)

    filter_set = set(args.games) if args.games else None
    target_games = []
    for g in games_list:
        snames = g.get("shortnames", [])
        if not snames:
            continue
        all_sn = set(snames)
        if g.get("original_game"):
            all_sn.add(g["original_game"])
        if filter_set and not (filter_set & all_sn):
            continue
        target_games.append(g)

    total_targets = len(target_games)
    updated_count = 0
    skipped_count = 0

    print(
        f"Starting scan of {total_targets} games (sources: {', '.join(args.sources)}, delay: {REQUEST_DELAY}s)...",
        flush=True,
    )

    for idx, game in enumerate(target_games, start=1):
        if STOP_REQUESTED:
            break

        snames = game["shortnames"]
        key = snames[0]
        title = game.get("title")
        orig_game = game.get("original_game")
        year = game.get("year")
        manufacturer = game.get("manufacturer")

        lookup_snames = list(snames)
        if orig_game and orig_game not in lookup_snames:
            lookup_snames.append(orig_game)

        entry = json.loads(json.dumps(scraped_db.get(key, {})))
        entry.pop("launchbox", None)
        entry["shortnames"] = snames
        entry["title"] = title
        entry["original_game"] = orig_game
        entry["year"] = year
        entry["manufacturer"] = manufacturer

        sources_to_run = [
            s
            for s in args.sources
            if needs_fetch(entry, s, refresh=args.refresh, retry_not_found=args.retry_not_found)
        ]

        if not sources_to_run:
            scraped_db[key] = entry
            skipped_count += 1
            print(f"[{idx}/{total_targets}] {key} ({title}) -> SKIP (already cached)", flush=True)
            continue

        # Look up local LaunchBox hint in-memory (not persisted to scraped_metadata.json)
        lb_match = None
        for sn in lookup_snames:
            if sn in lb_mame_sn:
                m_name, m_clone = lb_mame_sn[sn]
                for cand_n in (norm_title(m_name), norm_title(clean_base_title(m_name))):
                    if cand_n and cand_n in lb_by_norm:
                        lb_match = lb_by_norm[cand_n][0]
                        break
            if lb_match:
                break
        if not lb_match:
            for cand_t in (title, clean_base_title(title)):
                cn = norm_title(cand_t)
                if cn and cn in lb_by_norm:
                    lb_match = lb_by_norm[cn][0]
                    break

        # Collect candidate titles for searching (note: orig_game is a MAME shortname, not a title)
        candidate_titles = []
        lb_mame_title = None
        for sn in lookup_snames:
            if sn in lb_mame_sn:
                lb_mame_title = lb_mame_sn[sn][0]
                break
        for t in (
            clean_base_title(title),
            title,
            lb_match["name"] if lb_match else None,
            clean_base_title(lb_mame_title),
        ):
            if t and t not in candidate_titles:
                candidate_titles.append(t)

        status_parts = []

        # 1. WIKIDATA (via bulk MAME map first, or resolved after Wikipedia)
        if "wikidata" in sources_to_run and not STOP_REQUESTED:
            wd_hit = None
            for sn in lookup_snames:
                if sn in wd_mame_map:
                    wd_hit = wd_mame_map[sn]
                    break
            if wd_hit:
                entry["wikidata"] = {
                    "status": "found",
                    "qid": wd_hit["qid"],
                    "label": wd_hit["label"],
                    "mame_id": wd_hit["mame_id"],
                    "wikipedia_url": wd_hit["wikipedia_url"],
                    "strategywiki_url": wd_hit["strategywiki_url"],
                    "tcrf_url": wd_hit["tcrf_url"],
                    "klov_id": wd_hit["klov_id"],
                }
            else:
                entry["wikidata"] = {"status": "not_found"}

        # 2. WIKIPEDIA
        if "wikipedia" in sources_to_run and not STOP_REQUESTED:
            try:
                wp_hint_url = (
                    entry.get("wikidata", {}).get("wikipedia_url")
                    or (lb_match["wikipedia_url"] if lb_match else None)
                )
                wp_res = None
                if wp_hint_url and "/wiki/" in wp_hint_url:
                    hint_title = urllib.parse.unquote(wp_hint_url.split("/wiki/", 1)[1]).replace("_", " ")
                    fetched = fetch_wikipedia_by_title(hint_title)
                    if fetched:
                        wp_res = {
                            "status": "found",
                            "title": fetched["title"],
                            "url": fetched["url"],
                            "summary": fetched["summary"],
                            "gameplay": fetched["gameplay"] or None,
                        }
                if not wp_res and not STOP_REQUESTED:
                    wp_res = search_wikipedia_game(game, candidate_titles)
                if wp_res:
                    entry["wikipedia"] = wp_res
            except InterruptedError:
                break
            except Exception as e:
                entry["wikipedia"] = {"status": "error", "error": str(e)}

        # Enrich Wikidata if it wasn't in the MAME P2451 map but Wikipedia found an article
        if (
            "wikidata" in sources_to_run
            and entry.get("wikidata", {}).get("status") != "found"
            and entry.get("wikipedia", {}).get("status") == "found"
            and not STOP_REQUESTED
        ):
            try:
                wp_title = entry["wikipedia"].get("title")
                if wp_title:
                    wd_from_wp = query_wikidata_by_wp_title(wp_title)
                    if wd_from_wp:
                        entry["wikidata"] = wd_from_wp
            except InterruptedError:
                break
            except Exception as e:
                entry["wikidata"] = {"status": "error", "error": str(e)}

        # 3. STRATEGYWIKI
        if "strategywiki" in sources_to_run and not STOP_REQUESTED:
            try:
                sw_hint = entry.get("wikidata", {}).get("strategywiki_url")
                entry["strategywiki"] = search_strategywiki_game(candidate_titles, sw_url_hint=sw_hint)
            except InterruptedError:
                break
            except Exception as e:
                entry["strategywiki"] = {"status": "error", "error": str(e)}

        if STOP_REQUESTED:
            break

        for s in ALL_SOURCES:
            st = entry.get(s, {}).get("status", "-")
            if s == "wikidata" and st == "found" and entry["wikidata"].get("tcrf_url"):
                st = "found+tcrf"
            status_parts.append(f"{s}={st}")

        scraped_db[key] = entry
        atomic_save_json(args.output, scraped_db)
        updated_count += 1

        print(
            f"[{idx}/{total_targets}] {key} ({title}) -> {', '.join(status_parts)}",
            flush=True,
        )

        if args.limit > 0 and updated_count >= args.limit:
            print(f"Reached limit of {args.limit} updated games.", flush=True)
            break

    # Final atomic save
    atomic_save_json(args.output, scraped_db)
    print(
        f"\nDone. Updated: {updated_count}, Skipped (cached): {skipped_count}, Total in '{args.output}': {len(scraped_db)}.",
        flush=True,
    )


if __name__ == "__main__":
    main()
