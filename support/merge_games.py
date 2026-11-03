#!/usr/bin/env python3
#
# Merge game metadata from an external JSON file into AdvanceMAME's games.json.
#
# Behavior:
# - Matches records between the input file and the target games.json by
#   shortname intersection:
#   * If none of the incoming record's shortnames exist in games.json, the
#     entire record is appended as a new entry.
#   * If the incoming record matches the primary shortname of an existing
#     record, missing (null/empty) top-level fields are populated, list items
#     (modifications, websites, sources, research_notes) are appended without
#     duplicates, and variant entries are merged.
#   * If the incoming record targets a secondary shortname that is grouped
#     inside a multi-shortname record in games.json, its variant-specific
#     attributes are merged into the corresponding entry in "variants", while
#     missing record-level fields are populated on the enclosing record.
# - Existing non-empty scalar values in games.json are never overwritten.
#   Whenever an incoming record has a different non-empty value for an already
#   populated field, a message is printed to stdout in the format:
#     CONFLICT [<shortname>] <field>: current=<val> | incoming=<val>
# - Preserves the canonical key order and deterministic ASCII sorting defined
#   in GAMES.md, updating the target file atomically via a temporary file.
#
# Usage (run from support/):
#   python3 merge_games.py <input_file> [-t <target_file>] [-n]
#
# Arguments:
#   input_file            Path to the input JSON file to merge (must follow
#                         the games.json schema).
#   -t, --target <path>   Path to the destination JSON database to update
#                         (default: ../games.json).
#   -n, --dry-run         Analyze and print all conflicts and the merge summary
#                         to stdout without modifying the target file.
#   -h, --help            Show the help message and exit.
#

import argparse
import json
import os
import sys
import tempfile

TOP_KEYS = [
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

VARIANT_KEYS = [
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

MODIFICATION_KEYS = ["type", "description"]
WEBSITE_KEYS = ["type", "url"]
SOURCE_KEYS = ["type", "url", "path"]

SCALAR_TOP_FIELDS = [
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
    "hack_author",
    "hack_year",
    "description",
    "confidence",
]

SCALAR_VARIANT_FIELDS = [
    "parent_shortname",
    "title",
    "year",
    "developer",
    "publisher",
    "region",
    "language",
    "version",
]

LIST_TOP_FIELDS = [
    "modifications",
    "trivia",
    "tips",
    "scoring",
    "websites",
    "sources",
    "research_notes",
]

LIST_VARIANT_FIELDS = [
    "differences",
    "websites",
]


def order_dict(obj, key_order):
    """Return a new dict with keys ordered according to key_order."""
    ordered = {}
    for k in key_order:
        if k in obj:
            ordered[k] = obj[k]
    for k, v in obj.items():
        if k not in ordered:
            ordered[k] = v
    return ordered


def normalize_record(rec):
    """Ensure canonical key ordering and deterministic array sorting on a game record."""
    r = dict(rec)
    shortnames = list(r.get("shortnames", []))
    if len(shortnames) > 1:
        primary = shortnames[0]
        rest = sorted(s for s in shortnames[1:] if s != primary)
        shortnames = [primary] + rest
    r["shortnames"] = shortnames

    if "variants" in r and r["variants"]:
        sn_index = {s: i for i, s in enumerate(shortnames)}
        norm_vars = []
        for v in r["variants"]:
            v_copy = dict(v)
            if "websites" in v_copy:
                v_copy["websites"] = [order_dict(w, WEBSITE_KEYS) for w in v_copy["websites"]]
            if len(v_copy) > 1:
                norm_vars.append(order_dict(v_copy, VARIANT_KEYS))
        norm_vars.sort(key=lambda x: sn_index.get(x["shortname"], len(shortnames)))
        if norm_vars:
            r["variants"] = norm_vars
        else:
            r.pop("variants", None)
    else:
        r.pop("variants", None)

    if "modifications" in r:
        r["modifications"] = [order_dict(m, MODIFICATION_KEYS) for m in r["modifications"]]
    if "websites" in r:
        r["websites"] = [order_dict(w, WEBSITE_KEYS) for w in r["websites"]]
    if "sources" in r:
        r["sources"] = [order_dict(s, SOURCE_KEYS) for s in r["sources"]]

    return order_dict(r, TOP_KEYS)


def is_empty(val):
    return val is None or val == "" or val == []


def get_or_create_variant(target_rec, shortname):
    variants = target_rec.setdefault("variants", [])
    for v in variants:
        if v.get("shortname") == shortname:
            return v
    new_v = {"shortname": shortname}
    variants.append(new_v)
    return new_v


def merge_list_items(target_list, incoming_list, stats):
    added = 0
    for item in incoming_list:
        if item not in target_list:
            target_list.append(item)
            added += 1
    stats["list_items_added"] += added


def merge_variant_obj(target_rec, target_var, inc_var, stats):
    sname = target_var["shortname"]
    primary = target_rec["shortnames"][0]
    ctx = f"{primary}:{sname}" if sname != primary else primary

    for field in SCALAR_VARIANT_FIELDS:
        if field not in inc_var or is_empty(inc_var[field]):
            continue
        inc_val = inc_var[field]
        cur_val = target_var.get(field)
        # Also check if target_rec already has the exact same value at record level
        rec_val = target_rec.get(field)

        if is_empty(cur_val):
            if sname == primary and rec_val == inc_val:
                continue
            if cur_val is None and field not in target_var and rec_val == inc_val and field in ("year", "developer", "publisher", "version"):
                continue
            target_var[field] = inc_val
            stats["fields_added"] += 1
        elif cur_val != inc_val:
            print(
                f"CONFLICT [{ctx}] variants.{field}: "
                f"current={json.dumps(cur_val, ensure_ascii=False)} | "
                f"incoming={json.dumps(inc_val, ensure_ascii=False)}"
            )
            stats["conflicts"] += 1

    for lfield in LIST_VARIANT_FIELDS:
        if lfield in inc_var and inc_var[lfield]:
            t_list = target_var.setdefault(lfield, [])
            merge_list_items(t_list, inc_var[lfield], stats)


def merge_into_record(target_rec, inc_rec, matched_shortnames, stats):
    primary = target_rec["shortnames"][0]
    inc_primary = inc_rec["shortnames"][0]
    is_primary_match = (inc_primary == primary) or (primary in inc_rec.get("shortnames", []))

    # Add any new shortnames from incoming record that aren't already in target_rec or other records
    for s in inc_rec.get("shortnames", []):
        if s not in target_rec["shortnames"] and s not in stats["all_target_shortnames"]:
            target_rec["shortnames"].append(s)
            stats["all_target_shortnames"].add(s)
            stats["shortnames_added"] += 1

    if is_primary_match:
        # Merge top-level scalar fields
        for field in SCALAR_TOP_FIELDS:
            if field not in inc_rec or is_empty(inc_rec[field]):
                continue
            inc_val = inc_rec[field]
            cur_val = target_rec.get(field)
            if is_empty(cur_val):
                target_rec[field] = inc_val
                stats["fields_added"] += 1
            elif cur_val != inc_val:
                print(
                    f"CONFLICT [{primary}] {field}: "
                    f"current={json.dumps(cur_val, ensure_ascii=False)} | "
                    f"incoming={json.dumps(inc_val, ensure_ascii=False)}"
                )
                stats["conflicts"] += 1

        # Merge top-level list fields
        for lfield in LIST_TOP_FIELDS:
            if lfield in inc_rec and inc_rec[lfield]:
                t_list = target_rec.setdefault(lfield, [])
                merge_list_items(t_list, inc_rec[lfield], stats)
    else:
        # Incoming record represents a secondary variant shortname inside a grouped target record
        target_var = get_or_create_variant(target_rec, inc_primary)
        synth_var = {"shortname": inc_primary}
        for field in SCALAR_VARIANT_FIELDS:
            if field in inc_rec and not is_empty(inc_rec[field]):
                synth_var[field] = inc_rec[field]
        if "websites" in inc_rec and inc_rec["websites"]:
            synth_var["websites"] = inc_rec["websites"]
        merge_variant_obj(target_rec, target_var, synth_var, stats)

        # Top-level fields that only exist at record level
        for field in ("series", "hack_author", "hack_year", "description"):
            if field in inc_rec and not is_empty(inc_rec[field]):
                inc_val = inc_rec[field]
                cur_val = target_rec.get(field)
                if is_empty(cur_val):
                    target_rec[field] = inc_val
                    stats["fields_added"] += 1
                elif cur_val != inc_val:
                    print(
                        f"CONFLICT [{primary}<-{inc_primary}] {field}: "
                        f"current={json.dumps(cur_val, ensure_ascii=False)} | "
                        f"incoming={json.dumps(inc_val, ensure_ascii=False)}"
                    )
                    stats["conflicts"] += 1

        for lfield in ("modifications", "trivia", "tips", "scoring", "sources", "research_notes"):
            if lfield in inc_rec and inc_rec[lfield]:
                t_list = target_rec.setdefault(lfield, [])
                merge_list_items(t_list, inc_rec[lfield], stats)

    # Merge explicit variants from incoming record
    for inc_var in inc_rec.get("variants", []):
        v_sn = inc_var.get("shortname")
        if not v_sn or v_sn not in target_rec["shortnames"]:
            continue
        target_var = get_or_create_variant(target_rec, v_sn)
        merge_variant_obj(target_rec, target_var, inc_var, stats)


def main():
    parser = argparse.ArgumentParser(
        description="Merge game information from an input JSON file into games.json without overwriting existing values."
    )
    parser.add_argument(
        "input_file",
        help="Path to the input JSON file (same format as games.json)",
    )
    parser.add_argument(
        "-t",
        "--target",
        default="../games.json",
        help="Path to the target games.json file (default: ../games.json)",
    )
    parser.add_argument(
        "-n",
        "--dry-run",
        action="store_true",
        help="Report conflicts and summary without modifying the target file",
    )
    args = parser.parse_args()

    if not os.path.isfile(args.input_file):
        print(f"Error: input file '{args.input_file}' not found.", file=sys.stderr)
        sys.exit(1)

    if not os.path.isfile(args.target):
        print(f"Error: target file '{args.target}' not found.", file=sys.stderr)
        sys.exit(1)

    with open(args.target, "r", encoding="utf-8") as f:
        target_db = json.load(f)

    with open(args.input_file, "r", encoding="utf-8") as f:
        inc_db = json.load(f)

    if not isinstance(target_db, dict) or "games" not in target_db:
        print(f"Error: invalid format in '{args.target}'.", file=sys.stderr)
        sys.exit(1)

    if not isinstance(inc_db, dict) or "games" not in inc_db:
        print(f"Error: invalid format in '{args.input_file}'.", file=sys.stderr)
        sys.exit(1)

    target_games = target_db["games"]
    inc_games = inc_db["games"]

    # Map each shortname to its record in target_games
    sn_to_rec = {}
    all_target_shortnames = set()
    for rec in target_games:
        for s in rec.get("shortnames", []):
            sn_to_rec[s] = rec
            all_target_shortnames.add(s)

    stats = {
        "records_added": 0,
        "shortnames_added": 0,
        "fields_added": 0,
        "list_items_added": 0,
        "conflicts": 0,
        "all_target_shortnames": all_target_shortnames,
    }

    for inc_rec in inc_games:
        inc_sns = inc_rec.get("shortnames", [])
        if not inc_sns:
            continue

        # Find matching target records by shortname intersection
        matched_recs = []
        matched_sns_by_id = {}
        for s in inc_sns:
            if s in sn_to_rec:
                t_rec = sn_to_rec[s]
                rec_id = id(t_rec)
                if rec_id not in matched_sns_by_id:
                    matched_recs.append(t_rec)
                    matched_sns_by_id[rec_id] = []
                matched_sns_by_id[rec_id].append(s)

        if not matched_recs:
            # Brand new game record
            new_rec = normalize_record(inc_rec)
            target_games.append(new_rec)
            for s in new_rec["shortnames"]:
                sn_to_rec[s] = new_rec
                all_target_shortnames.add(s)
            stats["records_added"] += 1
        else:
            for t_rec in matched_recs:
                m_sns = matched_sns_by_id[id(t_rec)]
                merge_into_record(t_rec, inc_rec, m_sns, stats)
                for s in t_rec["shortnames"]:
                    sn_to_rec[s] = t_rec

    # Normalize and sort all target records
    normalized_games = [normalize_record(r) for r in target_games]
    normalized_games.sort(key=lambda r: r["shortnames"][0])
    target_db = {
        "schema_version": target_db.get("schema_version", 1),
        "games": normalized_games,
    }

    print(
        f"Summary: records_added={stats['records_added']}, "
        f"shortnames_added={stats['shortnames_added']}, "
        f"fields_added={stats['fields_added']}, "
        f"list_items_added={stats['list_items_added']}, "
        f"conflicts={stats['conflicts']}"
    )

    if not args.dry_run:
        target_dir = os.path.dirname(os.path.abspath(args.target))
        fd, tmp_path = tempfile.mkstemp(dir=target_dir, prefix=".games_merge_", suffix=".json")
        try:
            with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as tmp_f:
                json.dump(target_db, tmp_f, indent=2, ensure_ascii=False)
                tmp_f.write("\n")
            os.replace(tmp_path, args.target)
        except Exception:
            if os.path.exists(tmp_path):
                os.unlink(tmp_path)
            raise


if __name__ == "__main__":
    main()
