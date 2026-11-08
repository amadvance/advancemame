# AdvanceMAME Game Information Database (`games.json`)

## Purpose and mandatory maintenance rule

`games.json` is the repository's structured, researched catalog covering **all games and ROM sets enabled in AdvanceMAME (`mame/mamedriv.c`)**. Every active `DRIVER(...)` entry in `mame/mamedriv.c` must appear in `shortnames` of exactly one record in `games.json`.

**For standard arcade games supported by official MAME, metadata and grouping are generated automatically** by `support/build_games_json.py` (from `advmame -listxml` and `support/genre.ini`) and enriched by `support/enrich_games.py` (from `support/external/`). In general, no manual editing is required for standard MAME sets.

**Modify `support/curated.json` only when necessary:**

1. **Standalone hacks, bootlegs, or ports as root records:** when a clone/hack set must remain at the top level as its own distinct game family rather than being folded into the original game's clone list (e.g. `gngprincess`, `sf2t`, `batugnsp`, `cotton2e`). List all shortnames belonging to the family in `shortnames: [...]`.
2. **Curated descriptions:** when a substantive, user-facing narrative `description` is written for a game, homebrew, translation, or notable hack.
3. **Dedicated websites:** when specific project URLs, official homepages, or GitHub repositories must be linked in `websites`.
4. **Specific hack attributes:** when documenting `hack_author`, `hack_year`, or specific entries in `modifications`.

In `support/curated.json`, you can freely omit any fields with `null` values or empty arrays, even if they are required in `games.json`; the build script will automatically fill missing technical fields from `-listxml`.

To regenerate `games.json` use the support scripts:
```bash
python3 support/build_games_json.py && python3 support/enrich_games.py
```

`games.json` and `support/curated.json` are the persistent catalog artifacts tracked in Git. Do not create separate research reports or scratch data files.

## Sources and authority

Use these resources while researching or updating a record:

### Local repository resources

- AdvanceMAME: `amadvance/advancemame` (current repository)
- Driver registration list: `mame/mamedriv.c`
- Driver implementations and header comments: `mame/drivers/*.c`, `mame/machine/*.c`, `mame/vidhrdw/*.c`, including `GAME`, `GAMEB`, and related registration macros
- Genre classification file: `support/genre.ini`
- Curated game metadata and standalone hack/variant definitions: `support/curated.json`
- Offline external reference databases in `support/external/` (documented in `support/external/README`):
  - `support/external/history.xml`: Arcade-History (`HISTORY.XML`) with game overviews, `- TECHNICAL -`, `- TRIVIA -`, `- SERIES -`, `- STAFF -`, `- PORTS -`, `- TIPS AND TRICKS -`, `- SCORING -`, and `- UPDATES -` (synthesize and rewrite descriptions in your own words rather than copying entries verbatim)
  - `support/external/launchbox_arcade.xml` and `support/external/launchbox_mame.xml`: LaunchBox Arcade database and MAME ROM mapping table (`<Overview>`, `<WikipediaURL>`, `<VideoURL>`, region/language/version metadata)
  - `support/external/tcrf-current.xml`: The Cutting Room Floor (`tcrf.net`) full MediaWiki XML dump covering unused content, debug modes, prototypes, and regional/revision differences
  - `support/external/scraped_metadata.json`: incrementally cached metadata fetched by `support/scrape_external.py` from Wikidata, English Wikipedia, and StrategyWiki
  - `support/external/mameinfo.dat`, `support/external/command.dat`, `support/external/gameinit.dat`, `support/external/renameSET.dat`, `support/external/catver.ini`, `support/external/series.ini`, and `support/external/languages.ini`
- Official MAME: `mamedev/mame` (local clone in `resource/mame/`), where useful for comparisons, commit history, and original games
- HBMAME: `Robbbert/hbmame` (local clone in `resource/hbmame/`), especially useful for arcade hacks, translations, and homebrew releases

### External databases and reference sites by category

- **MAME and arcade databases:**
  - Arcade-History (`https://www.arcade-history.com`)
  - Arcade Database / ADB (`http://adb.arcadeitalia.net`) for technical data, controls, and historical MAME shortname renames
  - Classic MAME community data files such as `mameinfo.dat` (driver history and version changes) and `command.dat` (fighting game move lists and inputs)
- **Original arcade games, clones, and regional variants:**
  - English Wikipedia (`https://en.wikipedia.org`) and Wikidata (`https://www.wikidata.org`)
  - The International Arcade Museum / KLOV (`https://www.arcade-museum.com`)
  - System 16 - The Arcade Museum (`https://www.system16.com`) for arcade system hardware families and board specifications
  - MobyGames (`https://www.mobygames.com`) and StrategyWiki (`https://strategywiki.org`) for detailed gameplay mechanics
- **Prototypes and unreleased games:**
  - The Cutting Room Floor / TCRF (`https://tcrf.net`) for prototype differences, regional changes, and revision comparisons
  - Hidden Palace (`https://hiddenpalace.org`), Unseen64 (`https://www.unseen64.net`), and Gaming Alexandria (`https://www.gamingalexandria.com`) for prototype recovery and development history
- **Hacks and translations:**
  - ROMhack Plaza (`https://romhackplaza.org`) and ROMhacking.net (`https://www.romhacking.net`) for release notes, authors, and modification lists
  - Official project repositories (such as GitHub release pages and `README` documentation by hack and translation authors)
- **Homebrew games:**
  - Official developer pages, `itch.io` project pages, and Neo Geo community archives (such as NeoHomeBrew and Neo-Geo Wiki)
- **Bootlegs and unauthorized conversions:**
  - BootlegGames Wiki (`https://bootleggames.fandom.com`) for console-to-arcade conversions and bootleg hardware/software modifications

AdvanceMAME is authoritative for the shortnames actually registered in AdvanceMAME and their declared parent/clone relationships. Use external sources to verify gameplay, dates, authors, differences between versions, project history, and URLs. Source comments may be obsolete; do not treat every comment as independently verified historical fact.

**Do not store a dedicated C implementation/source-file property in each game record.** Source paths are useful during research and may appear as evidence entries in `sources` when relevant.

## Output conventions

Create or update `games.json` in the repository root.

- UTF-8, valid JSON, 2-space indentation, LF line endings
- Consistent property ordering following the exact key sequence shown in the [Example record](#example-record) for both top-level game objects and nested objects (`variants`, `modifications`, `websites`, `sources`)
- Deterministic ASCII lexicographical ordering of game records (sorted by primary identifier `shortnames[0]`), remaining `shortnames` (`shortnames[1..]`), and `variants` (matching their order in `shortnames`)
- No JSON comments, trailing commas, unnecessary computed indexes, or summary statistics
- All prose and human-readable metadata in English
- Prefer ASCII-compatible punctuation in prose; don't manually wrap paragraph strings
- Use `null` for unknown scalar values and `[]` for an empty required collection
- Omit optional inapplicable fields instead of filling them with placeholders
- Update the file atomically and preserve already verified information

The top-level structure is:

```json
{
  "schema_version": 1,
  "games": []
}
```

## Game record schema

Each element of `games` describes **one distinct game or a closely related group sharing a common description**.

### 1. Coverage: `shortnames` (required)

An array of AdvanceMAME shortnames covered by this record.

- Must contain at least one existing AdvanceMAME shortname.
- **The first element is the record's primary identifier.** Sort all remaining shortnames alphabetically (ASCII order).
- **Selecting the primary identifier:**
  1. If the group includes the parent set registered in AdvanceMAME, use that parent set as `shortnames[0]`.
  2. If all sets in the group are clones of a parent in a separate record (for example, a standalone hack or conversion given its own record), choose the primary or most complete release in the group, or the first in ASCII alphabetical order when no single set is clearly primary.
- Every active AdvanceMAME shortname in `mame/mamedriv.c` must occur in **exactly one** record's `shortnames` array.
- Do not create a redundant top-level `shortname` property.
- Do not include shortnames that are not registered in AdvanceMAME (`mame/mamedriv.c`); use relationship fields (`original_game`) if referencing an external shortname.
- Grouping does not require identical direct parents, but it requires enough shared gameplay, content, and history for one accurate description.

### 2. Game identity (required fields)

| Field | Type | Meaning |
|---|---|---|
| `title` | string | Full title common to the record |
| `year` | integer or null | Release year |
| `developer` | string or null | Developer |
| `publisher` | string or null | Publisher |

Use the primary/common values here; put meaningful differences in `variants`.

- **`year`:** If the driver macro contains an uncertain year with wildcards (such as `199?` or `198?`) and the exact year cannot be verified from reliable sources, set `year` to `null` and mention the approximate period in `description` (and/or `research_notes`).
- **`developer` and `publisher`:** MAME drivers combine these into a single manufacturer string (often formatted as `"Developer (Publisher license)"` or placeholders like `"<unknown>"` or `"bootleg"`). Split them accurately into `developer` and `publisher`. When the same company both developed and published the game, repeat the company name in both fields. Use `null` for unknown values; never copy placeholders such as `"<unknown>"`, `"bootleg"`, or `"hack"` into `developer` or `publisher`.

### 3. Classification (required fields)

| Field | Type | Meaning |
|---|---|---|
| `category` | string | Primary classification |
| `genre` | string or null | Gameplay genre |
| `platform` | string or null | Original hardware or platform |
| `players` | integer or null | Supported player count, if reliably known |
| `release_status` | string or null | Release status |
| `version` | string or null | Common version identifier |

Allowed `category` values:

- `original`
- `clone`
- `regional_variant`
- `hack`
- `bootleg`
- `homebrew`
- `prototype`
- `port`
- `demo`

Allowed non-null `release_status` values: `released`, `prototype`, `beta`, `demo`, `unreleased`, `unknown`. Prefer `null` when there is no evidence; use `unknown` only when it is explicitly useful to distinguish the status.

Use consistent names for genres and platforms. Do not infer player count or type from a ROM filename.

- **`genre` conventions:** Use the standard genre labels: `"Arcade"`, `"Ball & Paddle"`, `"Board Game"`, `"Climbing"`, `"Driving"`, `"Electromechanical"`, `"Fighter"`, `"Gambling"`, `"Game Console"`, `"Maze"`, `"Misc."`, `"MultiGame"`, `"Multiplay"`, `"Music Game"`, `"Platform"`, `"Puzzle"`, `"Quiz"`, `"Shooter"`, `"Slot Machine"`, `"Sports"`, `"System"`, `"Tabletop"`, `"Utilities"`, `"Whac-A-Mole"` (or their `"* Mature *"` variants such as `"Fighter * Mature *"`, `"Puzzle * Mature *"`, `"Tabletop * Mature *"`, etc., when applicable).
- **`platform` conventions:** Name the specific arcade system or hardware family when well established (for example, `"CPS-1"`, `"CPS-2"`, `"Neo Geo MVS"`, `"PGM"`, `"Sega System 16B"`, `"Namco Pac-Man hardware"`), and use `"Arcade"` for dedicated or discrete arcade hardware without a standard system name.

### 4. Relationships (required fields, except `series`)

| Field | Type | Meaning |
|---|---|---|
| `parent_shortname` | string or null | Direct parent declared in AdvanceMAME for the primary shortname |
| `original_game` | string or null | Original game's established shortname for a hack, bootleg, port, or derivative |
| `series` | string or null (optional) | Established franchise or game series name (for example, `"Street Fighter"`, `"Ghosts'n Goblins"`, `"DoDonPachi"`); omit when the game is not part of a multi-installment series |

- **`parent_shortname`:** If the primary shortname is registered as a parent set in AdvanceMAME (`clone_of` is `0` in the `GAME` macro), set `parent_shortname` to `null`.
- **`original_game`:** For a standalone original game (`category` is `"original"` and not derived from an earlier work), set `original_game` to `null` (do not repeat the record's own primary shortname). For a hack, bootleg, clone, port, or derivative placed in its own record, set `original_game` to the established shortname of the historically original game.
- **`series`:** Use a clean scalar franchise name (for example, extracted from `support/external/series.ini` or the `- SERIES -` section of `support/external/history.xml`). Omit `series` when the game is a standalone release not belonging to a series.
- **The immediate AdvanceMAME parent and the historically original game are not necessarily the same.** Do not conflate them. If a variant has a different direct parent, record that in the variant.

### 5. Per-shortname differences: `variants` (optional)

Use `variants` **only when at least one covered shortname needs distinct facts**. Every variant object must include `shortname`, which must also occur in the enclosing `shortnames` array, and entries in `variants` must follow the same order as `shortnames`. Other allowed variant properties (in canonical order) are:

- `parent_shortname` (string or null)
- `title` (string)
- `year` (integer or null)
- `developer` (string or null)
- `publisher` (string or null)
- `region` (string or null)
- `language` (string or null)
- `version` (string or null)
- `differences` (array of factual strings)
- `websites` (array of website objects, when specifically relevant to this variant)

Only add fields with meaningful variant-specific values. A primary variant needs an entry only if it has individual details to record. Avoid creating empty variants or repeating all game-level metadata. Do not infer differences solely from naming conventions. For different parent relationships, add explicit variant objects so the association remains accurate.

### 6. Hacks and modifications (optional)

For applicable hacks include:

- `hack_author` (string or null)
- `hack_year` (integer or null; omit when the same as the already recorded `year`)
- `modifications` (array of objects `{ "type": "...", "description": "..." }`)

Allowed modification types: `graphics`, `characters`, `levels`, `gameplay`, `difficulty`, `audio`, `translation`, `bugfix`, `technical`, `other`.

Describe **specific, verified changes** from the original. Use game-level `modifications` for changes shared across all covered shortnames; put variant-specific changes in that variant's `differences`. Omit hack-specific properties for games where they do not apply. For an unauthorized conversion or bootleg, discuss verifiable differences without automatically treating it as a hack.

### 7. Narrative: `description` (required, string or null)

A single, readable free-text string written **for the end user / player** explaining the game and what makes it distinctive, or `null` when no meaningful user-facing information is available yet. When present, it should stand on its own for someone who has not inspected the JSON fields. It may cover:

- Title, year, developer, publisher, genre, platform, and players
- Gameplay objectives, mechanics, controls, progression, enemies, and challenges
- Multiplayer features and distinctive presentation
- Original game, versions, ports, and notable regional or revision changes
- Precise hack/bootleg modifications and their historical context
- Release circumstances, cancelled projects, prototypes, and preservation history
- Interesting, well-supported background information

**What must NEVER appear in `description`:**

- **No emulator/driver meta-commentary:** Do not write about ROM shortnames, C source files, `GAME` macros, driver registrations, parent/clone declarations in code, or whether a shortname is present or absent in official MAME (for example, never write *"X is registered in AdvanceMAME as..."*, *"The source file establishes..."*, or *"This shortname does not appear in official MAME..."*).
- **No research disclaimers or uncertainty notes:** Never state in `description` that details are unknown, unverified, provisional, or need further research (for example, never write *"Exact differences require verification"* or *"Full game title is not yet established"*). Put all unresolved questions and editorial caveats exclusively in `research_notes`.
- **No placeholder or boilerplate text:** If nothing substantive is known about the game itself beyond its bare driver registration, set `description` to `null` rather than generating a filler sentence.

**Repeating structured facts naturally in `description` is allowed and desirable when it makes the explanation complete and readable.** Do not make the prose incomplete to avoid mentioning a developer, year, game relationship, or modification already represented in another field. Avoid unnecessary repetition *within* the prose, filler, marketing claims, and speculation. Use `\n\n` for paragraph breaks inside the JSON string. No manual wrapping.

Aim for approximately 100-200 words for substantial games when reliable sources support that much detail. A concise 1-3 sentence summary is fine for minor revisions or simple hacks; set `description` to `null` when no genuine user-facing description can be written.

For a grouped record, ensure its shared description applies accurately to every listed shortname. Explain any important variation in the narrative or in `variants`.

### 8. Trivia, tips, and scoring (optional)

When supported by reliable sources (such as `support/external/history.xml`, `support/external/tcrf-current.xml`, `support/external/command.dat`, or `support/external/gameinit.dat`), include any of the following optional player-facing fields immediately after `description` (omit them when empty or inapplicable):

- `trivia` (array of strings): Notable historical facts, release dates, alternate regional titles, soundtrack releases, or cultural references (for example, from the `- TRIVIA -` section of `support/external/history.xml`). Each element should be a self-contained factual statement or paragraph.
- `tips` (array of strings): Secrets, unlockable characters, warp codes, Easter eggs, or gameplay strategies (for example, from the `- TIPS AND TRICKS -` section of `support/external/history.xml`).
- `scoring` (array of strings): Point values, bonus rules, or multiplier mechanics (for example, from the `- SCORING -` section of `support/external/history.xml`).

### 9. `websites` (required, may be empty)

Array of objects with:

- `type`: one of `official`, `developer`, `project`, `release`, `reference`
- `url`: complete verified URL

Prefer game-specific official pages, developer/project sites, release pages, and reputable preservation references (such as Wikipedia, StrategyWiki, and The Cutting Room Floor). Do not guess URLs. If a website applies only to one variant, place it in that variant's `websites` instead.

### 10. `sources` (required, may be empty)

Array of evidence objects with:

- `type`: one of `source_code`, `official`, `documentation`, `archive`, `community`
- `url`: URL or null
- `path`: repository-relative path or null (for example, `mame/drivers/cps1.c` for AdvanceMAME, or `resource/mame/...` and `resource/hbmame/...` when citing local reference repositories)

Use this for references consulted and supporting record facts. Do not invent references. Include source paths here only when they document relevant information; **do not add a general `source_file` field** to the record.

### 11. Research quality (required)

- `confidence`: one of `verified`, `corroborated`, `uncertain`
- `research_notes`: array of unresolved factual questions (possibly empty)

`verified` requires authoritative evidence; `corroborated` requires multiple reasonably reliable sources; `uncertain` means important facts remain insufficiently established. An empty `research_notes` array does not certify omniscience. Never elevate confidence without performing the checks.

## Example record

This example is **synthetic** and illustrates structure only, not historical facts:

```json
{
  "schema_version": 1,
  "games": [
    {
      "shortnames": ["example", "examplea", "exampleb"],
      "title": "Example Game",
      "year": 1990,
      "developer": "Example Developer",
      "publisher": "Example Publisher",
      "category": "hack",
      "genre": "Platform",
      "platform": "Arcade",
      "players": 2,
      "release_status": "released",
      "version": null,
      "parent_shortname": "original",
      "original_game": "original",
      "series": "Example Series",
      "variants": [
        {
          "shortname": "examplea",
          "region": "Japan",
          "differences": ["Alternate level layout."]
        },
        {
          "shortname": "exampleb",
          "version": "1.1",
          "differences": ["Fixes a gameplay bug."]
        }
      ],
      "hack_author": "Example Author",
      "modifications": [
        {
          "type": "graphics",
          "description": "Replaces the main character sprites."
        }
      ],
      "description": "Example Game is a modified two-player arcade platform game based on Original Game. Players navigate stages, avoid hazards, and defeat enemies while collecting items.\n\nThe modification changes the visual presentation while retaining the fundamental mechanics of the original.",
      "trivia": [
        "Original Game was released in arcades in October 1990."
      ],
      "tips": [
        "Hold Start while selecting a character to use the alternate color palette."
      ],
      "scoring": [
        "Defeating an enemy: 100 points.",
        "Collecting a bonus item: 500 points."
      ],
      "websites": [],
      "sources": [],
      "confidence": "uncertain",
      "research_notes": []
    }
  ]
}
```

Do not copy invented example facts into real records.

## Research requirements by kind of game

**Original arcade games:** Investigate objectives, mechanics, controls, progression, player count, hardware, developer, release history, and distinguishing features.

**Hacks:** Identify the original game, original developer, hack author and release (when known), and *exactly what changes*. Distinguish graphical edits, level design, characters, gameplay, difficulty, audio, translations, fixes, and technical patches. Research similarly named revisions independently when their changes could differ.

**Bootlegs and unauthorized conversions:** Identify the original title and verified changes in hardware, presentation, software, protection, or gameplay. Do not presume modified gameplay. Distinguish console-to-arcade conversions from conventional arcade bootlegs.

**Homebrew:** Investigate developer/team, gameplay, hardware, release status, versions, project history, and an official page if available. Distinguish original work from a port, remake, or adaptation.

**Prototypes:** Investigate intended release, completion state, development background, known cancellation reasons, later discoveries, and differences from released versions. Do not invent reasons for cancellation.

**Clones and regional variants:** Capture verified region, language, software revisions, content, difficulty, hardware, and branding differences. If unknown, leave the difference undocumented rather than guessing.

## Grouping rules

1. One record describes one distinct game or a cohesive family of nearly equivalent versions (typically a parent set registered in `mame/mamedriv.c` together with its regional releases, revisions, and standard clones/bootlegs).
2. `shortnames` is the authoritative coverage list; its first item is the primary identifier.
3. `variants` records only actual per-shortname differences, and is omitted when unnecessary.
4. Group regional releases, minor revisions, alternate ROM sets, and closely related hack revisions when they can share an accurate narrative.
5. Split materially different hacks, games, ports, or projects when their gameplay, content, origin, or history requires a separate description.
6. Sharing a driver file, hardware platform, manufacturer, or franchise is **not sufficient** justification to group games.
7. Direct parent relationships are not required to match across a group, but must be represented accurately.
8. Every active AdvanceMAME shortname in `mame/mamedriv.c` must appear in exactly one record. Do not add shortnames that are not registered in `mame/mamedriv.c`.

## Research and editing workflow

For every new or changed AdvanceMAME game:

1. **Verify driver registration:** Ensure the shortname is registered in `mame/mamedriv.c` and builds cleanly.
2. **Determine if curation is necessary:**
   - **Standard MAME games / clones:** No curation needed. Running the generation scripts will automatically place clones under their parent, assign genres from `genre.ini`, and fetch trivia, tips, scoring, and website links from `support/external/`.
   - **Standalone hacks, bootlegs, or homebrew:** If a set has a `cloneof` tag in the driver but must be a standalone root family in `games.json`, add an entry to `support/curated.json` specifying all covered `shortnames`, `category: "hack"` (or `"bootleg"`/`"homebrew"`), and `original_game`.
   - **Curated descriptions or custom links:** If you have researched and written an informative gameplay narrative or want to attach specific project/official URLs, add them to the game's record in `support/curated.json`.
3. **Regenerate the catalog:**
   Run the build and enrichment scripts from `support/`:
   ```bash
   python3 build_games_json.py && python3 enrich_games.py
   ```
4. **Validate the result:**
   Check `git diff games.json` to verify that the affected game is correctly represented, no unrelated records are changed, and all active shortnames remain covered.

## Validation checklist

Before completing a change, check:

- The JSON parses and `schema_version` is `1`.
- All required fields and types are valid; optional fields use appropriate types if present.
- Every covered shortname exists in the current AdvanceMAME registration list (`mame/mamedriv.c`).
- Every active shortname in `mame/mamedriv.c` occurs exactly once across all records.
- No duplicate coverage, duplicate primary records, or stray unregistered shortnames exist.
- The first `shortnames` element is the primary identifier, and the rest are sorted.
- Game records are sorted by their primary identifier.
- Each variant references a shortname covered by the enclosing record, without duplicates.
- Variant parent changes and other known differences are represented accurately.
- Parent and original-game relationships are coherent and agree with known definitions.
- Hacks and modifications are specific, not speculative, and use allowed types.
- Categories, statuses, and confidence values use the defined vocabulary.
- `description` is either `null` or a non-empty, informative, self-contained user-facing text free of boilerplate, driver/MAME registration commentary, and research disclaimers.
- Natural overlap between the prose and structured metadata is **not** treated as an error.
- References are relevant; URLs are syntactically valid and are marked verified only if actually checked.
- Unknown values are `null`, empty arrays, or omitted optional fields as appropriate.
- No dedicated implementation source-file field has been introduced.

Report unresolved issues in the change summary; do **not** create a separate report file.

## Final editorial principles

- Be accurate rather than exhaustive when evidence is scarce; prefer `description: null` over boilerplate or meta-commentary.
- Prefer specific facts to generic praise or filler.
- Never invent dates, authors, credits, gameplay changes, historical anecdotes, or URLs.
- Use structured fields for machine-readable facts, `description` for a coherent player-facing account of the game, and `research_notes` for unresolved editorial questions.
- Preserve distinct variants and histories without producing redundant records.
- Keep `games.json` synchronized with additions and changes to AdvanceMAME games.
