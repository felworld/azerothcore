# AGENTS.md

AzerothCore is a C++ MMORPG server emulator for World of Warcraft 3.3.5a (WotLK), built with CMake, backed by MySQL.

## Felworld

This checkout is part of **Felworld**: replicating the feel of an MMORPG in a mostly single-player context, with "other players" emulated by AI (game-AI playerbots + LLM-driven behavior). The repos are public, sanitized as a tech demo. Five repos:

- **felworld/azerothcore** (this repo) — fork of mod-playerbots/azerothcore-wotlk (itself forked from azerothcore/azerothcore-wotlk); server core, containers, gameplay QOL. GPL v2.
- **felworld/mod-playerbots** (`modules/mod-playerbots`) — fork of mod-playerbots/mod-playerbots; QOL on top. GPL v2. Version-coupled with the core fork (see below).
- **felworld/mod-llm** (`modules/mod-llm`) — our own module (no upstream, MIT): agentic LLM-driven bots via a tool-call architecture (say/emote/remember/forget, party/guild/duel/follow actions, …) against the ac-vllm OpenAI-compatible endpoint.
- **felworld/mod-ah-bot-plus** (`modules/mod-ah-bot-plus`) — fork of NathanHandley/mod-ah-bot-plus (GPLv2+); deterministic auction house market maker (seller + buyer). Inert until `AuctionHouseBot.GUIDs` names dedicated non-playerbot character(s) created in-game.
- **felworld/configs** (`env/dist/etc`) — our playtested configs, no upstream.

Only containerized usage is supported (`docker compose`; Podman via `podman compose` works equally well) — upstream install-from-source instructions are obsolete here. Session modes via `.env.solo` / `.env.dumbbots` / `.env.llm` (vLLM profile).

### Upstream syncing

The felworld repos are not GitHub forks (GitHub `parent` is null) — find the sync point via `git merge-base` against the last non-Justin commit, not the fork API. `upstream` remotes are configured locally in the main repo and the submodules.

- **azerothcore** (this repo) ← `https://github.com/mod-playerbots/azerothcore-wotlk.git` branch **`Playerbot`** (the playerbots org's AC fork, which itself merges acore/master). NOT `azerothcore/azerothcore-wotlk`.
- **mod-playerbots** ← `https://github.com/mod-playerbots/mod-playerbots.git` branch `master`.
- **mod-ah-bot-plus** ← `https://github.com/NathanHandley/mod-ah-bot-plus.git` branch `master` (our default branch is `main`). Not version-coupled. When upstream's `conf/mod_ahbot.conf.dist` changes, regenerate `env/dist/etc/modules/mod_ahbot.conf` with `tools/regen-config.py`.
- **mod-llm** — entirely our code, no upstream to sync.

**Coupling:** mod-playerbots and the core `Playerbot` branch are version-locked — update them as a pair, then rebuild. Upstream commits like mod-playerbots #2470/#2492 ("must accompany core PR" / "Required update for core") match core-side changes. Never bump one without the other.

Sync = merge (not rebase) `upstream/<branch>` into `main`, then in the main repo `git add modules/<mod>` + commit the pointer bump. On README conflicts, resolve keep-ours (and keep upstream's translated READMEs deleted).

### Config tree (`env/dist/etc`)

The `felworld/configs` submodule is the deployed config tree — bind-mounted into the containers, which read **only the `.conf` files** (`.conf.dist` are in-repo reference, never read at runtime). Policy ("parallel pair"):

- Each `.conf.dist` is a **verbatim copy** of the pinned upstream template it tracks (`src/server/apps/worldserver/worldserver.conf.dist` for core, `modules/<mod>/conf/<mod>.conf.dist` for a module).
- Each `.conf` is **that template with our value-overrides re-applied** — structurally identical (same comments, order, whitespace, line count), differing only in overridden values, so `diff <name>.conf <name>.conf.dist` shows exactly our deviations from default.
- On an upstream bump: copy the new templates over the `.conf.dist` files verbatim, then regenerate each `.conf` with `tools/regen-config.py` (3-way: base = old `.dist`, ours = old `.conf`, theirs = new `.dist`). New upstream keys keep the template default. Verify identical line counts and a value-only diff; if upstream reformatted the template, the `.conf` gets reflowed to match — accepted cost.
- Commit in the submodule, then bump the pointer in the main repo.

### No backwards compatibility

Felworld is pre-production; existing DB data is disposable. When a schema or feature replaces an old one, drop/replace directly (e.g. `DROP TABLE IF EXISTS` in module base SQL) — no migration or seeding code, no support for deprecated config options.

### Documentation upkeep

READMEs are hub-and-spokes and are the onboarding surface for closed-beta invitees — keep them current:

- **Hub**: `.github/README.md` — project pitch, repo/fork table, containerized quickstart, session-modes table, "What we've changed" QOL summary, license note.
- **Spokes**: `modules/mod-playerbots/README.md`, `modules/mod-llm/README.md`, `env/dist/etc/README.md` — each: what the fork/module is, "Felworld changes" list, links to upstream + hub.
- **FEATURES.md** (uniform name on purpose) at the hub root, mod-playerbots, and mod-llm holds the detailed feature/behavior docs (config knobs + rationale); READMEs carry only one-liner summaries linking to it.

When landing a user-visible change — gameplay/QOL option, GM command, compose/session-mode change, config-convention change — update the matching docs in the same piece of work: full detail in that repo's FEATURES.md, a one-liner in the README. Style: to-the-point, closed-beta audience, server-side only (no client-setup docs), no public-repo fluff (badges, contributing, socials).

**Framing**: all docs and repo descriptions present Felworld as *a tech demo of AI "players" (LLM agents + classical game AI) populating and interacting in an MMO world* — not as "a private WoW server". Avoid "blizzlike"/"World of Warcraft" in our own prose (factual, nominative mentions like "3.3.5a client" are fine; quoted upstream text stays verbatim). The hub README's "License and disclaimer" section (`#license-and-disclaimer` anchor) is linked from every spoke — keep the anchor stable and new docs consistent with it. Licenses are verified from LICENSE files, not upstream READMEs' claims.

### LLM prompt/context design (mod-llm)

- **Commands + tools, not chat parsing**: mod-playerbots never interprets free-form chat — natural-language understanding lives in mod-llm. When bots should react to something players say (e.g. BG callouts like "inc" or "fc mid"), add an explicit playerbot command that performs the behavior deterministically, then expose that command to mod-llm as a tool so the LLM decides when to invoke it. Real players get the same command for free, and keyword heuristics stay out of C++.
- **Context heuristic**: anything a player would see **on their screen** (party/raid frames, target, zone, chat, own current activity, …) must be in the bot's context; anything a player would **remember over 1–5 minutes** is a strong candidate. Bots should act on the same information a human at the keyboard would have — missing on-screen facts causes nonsense like inviting someone already in the party. Check ContextSnapshot against this bar before reaching for tool-availability filtering or prompt rules. Where a server config defines player rules (e.g. `ListenRange.*` hear distances), mirror bot behavior to it.
- **No anti-examples in prompts**: never quote what not to say (e.g. `never say "Greetings, traveler"`) — on small local models a phrase given as a negative example can *increase* its generation probability, since the tokens are in context regardless of the negation. Steer register with positive few-shot exemplars and positive rules only.

## Agent rules

- **Do not configure or build unless explicitly asked.** Builds are slow (CMake + compile of a large C++ codebase) and rarely needed to make code changes.
- **Discuss design questions before implementing.** When the user poses a design question or proposal ("what do you think?", "an alternative idea…"), answer with a recommendation and trade-offs and wait for agreement — don't jump to implementation, builds, or commits. The commit-after-change rule below applies to agreed work, not to drafts produced mid-discussion.
- **Never edit SQL files outside `data/sql/updates/pending_db_*/`.** `data/sql/base/`, `data/sql/archive/`, and `data/sql/updates/db_*/` are immutable (do not modify).
- **Commit after every completed change, without being asked.** Don't wait for a running build/test to finish — commit as soon as the change is done; if the build later fails, fix forward with a follow-up commit. `modules/*` and `env/dist/etc` are git submodules with their own history: commit inside the submodule, then bump the pointer in the parent repo as a separate commit (both commit directly to `main`). Stage only the files you changed — never sweep in unrelated dirty files; if a pointer bump would pull in earlier submodule commits you didn't author, flag it and let the user decide. **Push only when asked.**
- **Close issues from commit messages.** When a commit resolves a GitHub issue, add one line per issue in the commit body: `Fixes <owner>/<repo>#<n>`. The keyword must directly precede each reference — prose like "fixes a family of bugs (#12, #13)" does not match GitHub's auto-close syntax. Use the full `owner/repo#n` form when committing in a submodule whose issues live in another repo; bare `#n` only works within the same repo.
- **Read GitHub issues/PRs with `gh`** (`gh issue view` / `gh pr view`, add `--comments` when needed), not WebFetch — the rendered-page fetch costs far more tokens for the same content.
- **Track in-flight work on the GitHub issue, not in notes or agent memory.** Post findings and status on the issue via `gh`; reserve memory for durable preferences, workflows, and environment facts.
- **Run long-running commands (builds, etc.) bare** — no `| tail` / `| head` pipes, which buffer the output so nothing is visible until the command finishes; the user follows the live output. If only the end matters, tail the output file after completion.

## Build

Only containerized builds are supported — do not run cmake/make natively:

```bash
docker compose build ac-worldserver                 # the main (slow) build
docker compose build ac-authserver ac-db-import     # also worth verifying after core merges
```

Compiler: **C++20** required (`CMAKE_CXX_STANDARD 20`). Full CMake flag set in `conf/dist/config.cmake`. Google Test unit tests live in `src/test/` (built with `-DBUILD_TESTING=ON`).

## Repository layout

- `src/common/` — networking (Asio), crypto, config, logging, shared utilities.
- `src/server/game/` — core gameplay; compiled into worldserver.
- `src/server/scripts/` — content scripts grouped by region (`EasternKingdoms/`, `Northrend/`, …), class (`Spells/spell_mage.cpp`, …), and domain (`Commands/`, `Pet/`, `OutdoorPvP/`, `World/`).
- `src/server/database/` — DB abstraction and schema updater.
- `src/server/shared/` — code shared by auth and world servers.
- `src/server/apps/{authserver,worldserver}/` — entry points (ports 3724 and 8085).
- `src/test/` — Google Test unit tests + mocks.
- `data/sql/` — `base/` (historical schema), `updates/db_*/` (merged), `updates/pending_db_*/` (in-flight, **edit here**), `custom/` (gitignored).
- `modules/` — external modules (each a subdir with its own `CMakeLists.txt`). Disable with `-DDISABLED_AC_MODULES="mod1;mod2"`. See `modules/how_to_make_a_module.md`.
- `apps/` — helper scripts; `apps/codestyle/` holds the lint scripts (see below).
- `conf/dist/` — distributed config templates; `conf/*.conf` is gitignored.
- `deps/` — vendored third-party dependencies.

## Adding SQL updates

1. `cd data/sql/updates/pending_db_world/` (or `pending_db_auth` / `pending_db_characters`).
2. `./create_sql.sh` generates an empty `rev_<timestamp>.sql` you write into.
3. Required SQL conventions (enforced by `apps/codestyle/codestyle-sql.py`):
   - Every `INSERT` must be preceded by a matching `DELETE` (idempotency).
   - 4-space indent (no tabs), trailing newline, no double semicolons, no multiple blank lines.
   - Tables must use the InnoDB engine.

The three databases:

- `acore_auth` — accounts, realm list, IP/account bans, session keys. Shared across all realms.
- `acore_characters` — per-character state: characters, inventory, in-progress quests, mail, guilds, arena teams, achievements. One per realm.
- `acore_world` — static game content: creature/gameobject/item/quest templates, spawn lists, loot tables, SmartAI scripts, gossip, conditions. Read-mostly; rebuilt from SQL.

## Code style

Run the linters before claiming a change is done:

```bash
python apps/codestyle/codestyle-cpp.py     # C++
python apps/codestyle/codestyle-sql.py     # SQL (compares to origin/master)
```

Hard rules (also enforced by CI with `-Werror`):

- 4-space indent for C++ (tabs forbidden); 2-space for JSON/YAML/sh/ts/js. UTF-8, LF, max 120 cols, trailing newline.
- Allman braces. No braces around single-line statements. `if (x)` — never `if(x)` or `if ( x )`.
- `auto const&` (not `const auto&`); `Type const*` (not `const Type*`).
- Use `{}` format specifiers (`fmt`-style), not `%u`/`%s`.
- Use the typed helpers, not raw flag access:
  - `IsPlayer()`, `IsCreature()`, `IsItem()`, … instead of `GetTypeId() == TYPEID_*`.
  - `GetNpcFlags()`, `HasNpcFlag()`, `SetNpcFlag()`, `RemoveNpcFlag()`, `ReplaceAllNpcFlags()` instead of `*Flag(UNIT_NPC_FLAGS, …)`.
  - `IsRefundable()`, `IsBOPTradable()`, `IsWrapped()` instead of `HasFlag(ITEM_FIELD_FLAGS, …)`.
  - `HasFlag(ItemFlag)` / `HasFlag2(ItemFlag2)` / `HasFlagCu(ItemFlagsCustom)` instead of bitwise `Flags & ITEM_FLAG…`.
  - `ObjectGuid::ToString().c_str()` instead of `ObjectGuid::GetCounter()`.

CI also runs `cppcheck`.

## Project conventions

- **Logging**: `LOG_INFO("category.sub", "msg with {}", arg)` (also `LOG_WARN`, `LOG_ERROR`, `LOG_DEBUG`, `LOG_TRACE`). Categories are hierarchical, dot-separated (e.g. `server.loading`, `entities.player`, `sql.dev`). No `printf`-style; no `sLog->`; no `TC_LOG_*`. Macro in `src/common/Logging/Log.h`.
- **Random**: use project helpers from `src/common/Utilities/Random.h` — `urand`, `irand`, `frand`, `rand32`, `rand_chance`, `roll_chance_f`, `roll_chance_i`. Do not use `std::rand` or `<random>` directly.
- **Strings**: `Acore::StringFormat(fmt, args...)` (wraps `fmt::format`, `{}` placeholders) — `src/common/Utilities/StringFormat.h`.
- **Config**: read options with `sConfigMgr->GetOption<T>("Name", default)`.
- **Namespace**: project-wide is `Acore::` (no `Trinity::` remnants — agents porting from upstream forks must rename).
- **Long-lived references**: do not store a raw `Player*` / `Creature*` / `Unit*` past the current call/tick — the object can be removed (logout, despawn, instance unload) and the pointer dangles. Store the `ObjectGuid` and resolve at use time via `ObjectAccessor::FindPlayer(guid)`, `ObjectAccessor::GetCreature(*from, guid)`, `Map::GetCreature(guid)`, etc.
- **DB queries**: use `PreparedStatement` (via `WorldDatabase` / `CharacterDatabase` / `LoginDatabase` and the prepared-statement enums) rather than raw query strings. Reads that don't need to block the world tick go through the async path: `_queryProcessor.AddCallback(db.AsyncQuery(stmt).WithPreparedCallback(...))` (or `WithCallback` for non-prepared). Multi-statement writes wrap in `SQLTransaction` + `Execute` / `AppendPreparedStatement`.
- **Timed actions in AI**: use `EventMap` (event id → delay; simple) or `TaskScheduler` (lambdas, repeats, cancellation). Both are members of `CreatureAI`; see any boss script under `src/server/scripts/` for examples — don't roll your own tick counters.

## Scripting registration

Scripts inherit from a `ScriptObject` subclass (`SpellScript`, `AuraScript`, `CreatureScript`, `InstanceMapScript`, `GameObjectScript`, `CommandScript`, …). Two registration styles coexist:

- **Spell / aura scripts**: use the `RegisterSpellScript(ClassName)` (or `RegisterSpellAndAuraScriptPair(...)`) macro inside `AddSC_<name>()`.
- **Creature scripts**: prefer `RegisterCreatureAI(ClassName)` for new code; legacy zones still use `new ClassName();`. Match the surrounding pattern.

Then declare and call `AddSC_<name>()` from the regional loader: `Spells/spells_script_loader.cpp`, `EasternKingdoms/eastern_kingdoms_script_loader.cpp`, etc.

**SmartAI** (data-driven creature behaviour) lives in the world DB's `smart_scripts` table — not in C++. Engine: `src/server/game/AI/SmartScripts/`. For new creature behaviour prefer SmartAI (added via the SQL update workflow); reach for `CreatureScript` only when SmartAI's event/action vocabulary isn't enough.

**Module hooks** (e.g. `OnPlayerLogin`, `OnWorldUpdate`, `OnSpellCast`) are declared in `src/server/game/Scripting/ScriptDefines/*.h`. Implement by inheriting the matching base (`PlayerScript`, `WorldScript`, …) and registering with `new MyClass();` (or its `RegisterXxxScript` macro where one exists) inside `AddSC_<name>()`. Full hook list: https://www.azerothcore.org/wiki/hooks-script.

Custom (non-upstream) scripts go in `src/server/scripts/Custom/` (gitignored).
