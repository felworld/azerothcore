# AGENTS.md

AzerothCore is a C++ MMORPG server emulator for World of Warcraft 3.3.5a (WotLK), built with CMake, backed by MySQL.

## Felworld

This checkout is part of **Felworld**: the feel of an MMORPG in a mostly single-player context, with "other players" emulated by AI (game-AI playerbots + LLM-driven behavior). Repos are public, sanitized as a tech demo:

- **felworld/azerothcore** (this repo) — fork of mod-playerbots/azerothcore-wotlk (itself forked from azerothcore/azerothcore-wotlk); server core, containers, gameplay QOL. GPL v2.
- **felworld/mod-playerbots** (`modules/mod-playerbots`) — fork of mod-playerbots/mod-playerbots; QOL on top. GPL v2. Version-coupled with the core fork (see below).
- **felworld/mod-llm** (`modules/mod-llm`) — our own module (no upstream, MIT): agentic LLM-driven bots via tool calls (say/emote/remember/forget, party/guild/duel/follow, …) against the ac-vllm OpenAI-compatible endpoint.
- **felworld/mod-ah-bot-plus** (`modules/mod-ah-bot-plus`) — fork of NathanHandley/mod-ah-bot-plus (GPLv2+); deterministic auction house market maker. Inert until `AuctionHouseBot.GUIDs` names dedicated non-playerbot character(s) created in-game.
- **felworld/configs** (`env/dist/etc`) — our playtested configs, no upstream.

Containerized usage only (`docker compose`; `podman compose` works equally well) — upstream install-from-source instructions are obsolete here. Session modes: `.env.solo` / `.env.dumbbots` / `.env.llm` (vLLM profile).

### Upstream syncing

The felworld repos are not GitHub forks (`parent` is null) — find the sync point via `git merge-base` against the last non-Justin commit, not the fork API. `upstream` remotes are configured locally in the main repo and submodules.

- **azerothcore** ← `https://github.com/mod-playerbots/azerothcore-wotlk.git` branch **`Playerbot`** (which merges acore/master). NOT `azerothcore/azerothcore-wotlk`.
- **mod-playerbots** ← `https://github.com/mod-playerbots/mod-playerbots.git` `master`. **Version-locked with the core `Playerbot` branch** — update as a pair, then rebuild; never bump one without the other (upstream marks pairs, e.g. #2470/#2492 "must accompany core PR").
- **mod-ah-bot-plus** ← `https://github.com/NathanHandley/mod-ah-bot-plus.git` `master` (our default branch is `main`). Not version-coupled. When upstream's `conf/mod_ahbot.conf.dist` changes, regenerate `env/dist/etc/modules/mod_ahbot.conf` with `tools/regen-config.py`.
- **mod-llm** — no upstream.

Sync = merge (not rebase) `upstream/<branch>` into `main`, then commit the submodule pointer bump in the main repo. README conflicts: keep ours; keep upstream's translated READMEs deleted.

### Config tree (`env/dist/etc`)

The `felworld/configs` submodule is the deployed config tree, bind-mounted into the containers, which read **only the `.conf` files** (`.conf.dist` are in-repo reference, never read at runtime). "Parallel pair" policy:

- Each `.conf.dist` is a **verbatim copy** of the pinned upstream template it tracks (`src/server/apps/worldserver/worldserver.conf.dist` for core, `modules/<mod>/conf/<mod>.conf.dist` per module).
- Each `.conf` is that template with our value-overrides re-applied — structurally identical (comments, order, whitespace, line count), so `diff <name>.conf <name>.conf.dist` shows exactly our deviations.
- Upstream bump: copy new templates over the `.conf.dist` files verbatim, then regenerate each `.conf` with `tools/regen-config.py` (3-way: base = old `.dist`, ours = old `.conf`, theirs = new `.dist`; new upstream keys keep the default). Verify identical line counts and a value-only diff; if upstream reformatted, the `.conf` reflows to match — accepted cost.
- Commit in the submodule, then bump the pointer in the main repo.

### No backwards compatibility

Felworld is pre-production; DB data is disposable. When a schema or feature replaces an old one, drop/replace directly (e.g. `DROP TABLE IF EXISTS` in module base SQL) — no migrations, no seeding code, no deprecated config options.

### Human-vs-bot gameplay costs (the usual pattern)

When a gameplay cost (debuff, penalty, delay, rate) should be spared for humans but kept for bots, build it as a pair:

1. **Core** (`worldserver.conf`): an option that turns the cost off server-wide — reuse the upstream option if one exists (`Death.SicknessLevel`, `Battleground.CastDeserter`, `DungeonFinder.CastDeserter`), else add one — plus a `PlayerScript` hook at the decision point letting a module adjust the computed value per player (`OnPlayerResurrectSicknessLevel`, `OnPlayerGhostSpeedRate`, `OnPlayerBattlegroundDeserterDebuff` / `OnPlayerDungeonDeserterDebuff`). The core never checks `IsBot()` itself.
2. **mod-playerbots** (`playerbots.conf`): an `AiPlayerbot.*` option restoring/replacing the cost for bot sessions (`player->GetSession()->IsBot()`) via that hook. Default = follow the server setting, so upstream behaviour is unchanged until configured.
3. **Configs** (`env/dist/etc`): cost off in `worldserver.conf`, playerbots override on.

Document the core side in the hub `FEATURES.md` and the override in mod-playerbots' `FEATURES.md`, cross-linked.

### Many bots reacting to one event (the usual pattern)

When one event is noticed by a whole group of bots at once (invite batch landing, player leaving, camp finishing, enemies at the flag room): **never let every eligible bot act, and never hard-cap at one** — all reacting is a tell, exactly-one-every-time is a quieter tell. Ration with a **geometric falloff**:

1. The first bot to reach the event **rolls the whole quota** — a `*Chance` option for whether anybody reacts, then a `*Falloff` option (0-100, typically ~30) rolled per additional actor, each conditional on the previous, capped at a sane maximum.
2. Everyone else **claims a slot** against that quota; losers stay silent. The quota expires after a window (~15s) so a later, unrelated event rolls fresh.
3. State lives in a **mutex-guarded board keyed by the room** (group, attacker, zone, …) — claimants run on different map-update threads. Reference implementations: `GroupChatterBoard` (`src/Ai/World/Group/`), `WpvpDefenseBoard::TryClaimResponseSlot`.
4. **Stagger the winners**: slot *n* acts a beat after slot *n-1* (`GroupChatterDelayMs`), and nobody acts on the event's own tick — a second line has to read as an answer, not an echo.

Roll the quota up front, not per-bot independent chances: a per-bot roll scales the count with bots in earshot, so the same option means different things in a party and a raid; a rolled quota has the same distribution either way (at 85/30: 15% nobody, 60% one, 18% two, 5% three). This is the shape behind group hellos/goodbyes, battleground callouts ("one callout per team per wave"), wpvp defense waves, and mod-llm's group greetings — mirror it in mod-llm whenever the LLM voices a moment playerbots rations, so llm mode doesn't reintroduce the wall.

### Documentation upkeep

READMEs are hub-and-spokes, the onboarding surface for closed-beta invitees — keep them current:

- **Hub** `.github/README.md`: pitch, repo/fork table, containerized quickstart, session-modes table, "What we've changed" QOL summary, license note.
- **Spokes** `modules/mod-playerbots/README.md`, `modules/mod-llm/README.md`, `env/dist/etc/README.md`: what the fork/module is, "Felworld changes" list, links to upstream + hub.
- **FEATURES.md** (uniform name on purpose) at the hub root, mod-playerbots, and mod-llm: detailed feature/behavior docs (config knobs + rationale); READMEs carry only one-liner summaries linking to it.

When landing a user-visible change (gameplay/QOL option, GM command, compose/session-mode change, config-convention change), update the matching docs in the same piece of work: full detail in that repo's FEATURES.md, a one-liner in the README. Style: to-the-point, closed-beta audience, server-side only (no client-setup docs), no public-repo fluff (badges, contributing, socials).

**Framing**: all docs and repo descriptions present Felworld as *a tech demo of AI "players" (LLM agents + classical game AI) populating and interacting in an MMO world* — not "a private WoW server". Avoid "blizzlike"/"World of Warcraft" in our own prose (factual, nominative mentions like "3.3.5a client" are fine; quoted upstream text stays verbatim). The hub README's "License and disclaimer" section (`#license-and-disclaimer` anchor) is linked from every spoke — keep the anchor stable and new docs consistent with it. Verify licenses from LICENSE files, not upstream READMEs' claims.

### LLM prompt/context design (mod-llm)

- **Commands + tools, not chat parsing**: mod-playerbots never interprets free-form chat — NLU lives in mod-llm. When bots should react to player speech (e.g. BG callouts like "inc" or "fc mid"), add an explicit playerbot command that performs the behavior deterministically, then expose it to mod-llm as a tool. Real players get the command for free; keyword heuristics stay out of C++.
- **Context heuristic**: anything a player would see **on their screen** (party/raid frames, target, zone, chat, own activity) must be in the bot's context; anything they'd **remember over 1–5 minutes** is a strong candidate. Missing on-screen facts causes nonsense like inviting someone already in the party. Check ContextSnapshot against this bar before reaching for tool-availability filtering or prompt rules. Where a server config defines player rules (e.g. `ListenRange.*` hear distances), mirror bot behavior to it.
- **No anti-examples in prompts**: never quote what not to say — on small local models a negatively-framed phrase can *increase* its generation probability, since the tokens are in context regardless. Steer register with positive few-shot exemplars and positive rules only.

## Agent rules

- **Always build after completing code changes** with `docker compose build` (see Build) — containerized only, never native cmake/make. Commit first (per the rule below), then build; if the build fails, fix forward with a follow-up commit.
- **Discuss design questions before implementing.** When the user poses a design question or proposal ("what do you think?", "an alternative idea…"), answer with a recommendation and trade-offs and wait for agreement — no implementation, builds, or commits. The commit rule applies to agreed work, not mid-discussion drafts.
- **Never edit SQL files outside `data/sql/updates/pending_db_*/`** — `base/`, `archive/`, and `updates/db_*/` are immutable.
- **Commit after every completed change, without being asked.** Don't wait for a running build/test — commit when the change is done; fix forward if it later fails. `modules/*` and `env/dist/etc` are git submodules: commit inside, then bump the pointer in the parent as a separate commit (both directly to `main`). Stage only the files you changed — never sweep in unrelated dirty files; if a pointer bump would pull in submodule commits you didn't author, flag it and let the user decide. **Push only when asked.**
- **Close issues from commit messages**: one line per issue in the body, `Fixes <owner>/<repo>#<n>`, the keyword directly preceding each reference (prose like "fixes a family of bugs (#12, #13)" doesn't auto-close). Use the full `owner/repo#n` form when committing in a submodule whose issues live in another repo.
- **Read GitHub issues/PRs with `gh`** (`gh issue view` / `gh pr view`, add `--comments` when needed), not WebFetch — far cheaper in tokens.
- **Track in-flight work on the GitHub issue**, not notes or agent memory. Post findings/status via `gh`; reserve memory for durable preferences, workflows, environment facts.
- **Run long-running commands (builds, etc.) bare** — no `| tail` / `| head`, which buffer output the user follows live. If only the end matters, tail the output file after completion.

## Build

Containerized only — never native cmake/make:

```bash
docker compose build ac-worldserver                 # the main (slow) build
docker compose build ac-authserver ac-db-import     # also worth verifying after core merges
```

**C++20** required (`CMAKE_CXX_STANDARD 20`). Full CMake flag set in `conf/dist/config.cmake`. Google Test unit tests in `src/test/` (built with `-DBUILD_TESTING=ON`).

## Repository layout

- `src/common/` — networking (Asio), crypto, config, logging, shared utilities.
- `src/server/game/` — core gameplay; compiled into worldserver.
- `src/server/scripts/` — content scripts grouped by region (`EasternKingdoms/`, `Northrend/`, …), class (`Spells/spell_mage.cpp`, …), and domain (`Commands/`, `Pet/`, `OutdoorPvP/`, `World/`).
- `src/server/database/` — DB abstraction and schema updater.
- `src/server/shared/` — code shared by auth and world servers.
- `src/server/apps/{authserver,worldserver}/` — entry points (ports 3724 and 8085).
- `src/test/` — Google Test unit tests + mocks.
- `data/sql/` — `base/` (historical schema), `updates/db_*/` (merged), `updates/pending_db_*/` (in-flight, **edit here**), `custom/` (gitignored).
- `modules/` — external modules (each with its own `CMakeLists.txt`). Disable with `-DDISABLED_AC_MODULES="mod1;mod2"`. See `modules/how_to_make_a_module.md`.
- `apps/` — helper scripts; `apps/codestyle/` holds the lint scripts.
- `conf/dist/` — distributed config templates; `conf/*.conf` is gitignored.
- `deps/` — vendored third-party dependencies.

## Adding SQL updates

1. `cd data/sql/updates/pending_db_world/` (or `pending_db_auth` / `pending_db_characters`).
2. `./create_sql.sh` generates an empty `rev_<timestamp>.sql` you write into.
3. Conventions (enforced by `apps/codestyle/codestyle-sql.py`): every `INSERT` preceded by a matching `DELETE` (idempotency); 4-space indent, no tabs; trailing newline; no double semicolons; no multiple blank lines; InnoDB engine.

The three databases:

- `acore_auth` — accounts, realm list, bans, session keys. Shared across realms.
- `acore_characters` — per-character state: characters, inventory, quests in progress, mail, guilds, arena teams, achievements. One per realm.
- `acore_world` — static game content: creature/gameobject/item/quest templates, spawns, loot, SmartAI, gossip, conditions. Read-mostly; rebuilt from SQL.

## Code style

Run the linters before claiming a change is done:

```bash
python apps/codestyle/codestyle-cpp.py     # C++
python apps/codestyle/codestyle-sql.py     # SQL (compares to origin/master)
```

Hard rules (also enforced by CI with `-Werror`; CI also runs `cppcheck`):

- 4-space indent for C++ (tabs forbidden); 2-space for JSON/YAML/sh/ts/js. UTF-8, LF, max 120 cols, trailing newline.
- Allman braces. No braces around single-line statements. `if (x)` — never `if(x)` or `if ( x )`.
- `auto const&` (not `const auto&`); `Type const*` (not `const Type*`).
- `{}` format specifiers (`fmt`-style), not `%u`/`%s`.
- Typed helpers, not raw flag access: `IsPlayer()`/`IsCreature()`/`IsItem()` (not `GetTypeId() == TYPEID_*`); `GetNpcFlags()`/`HasNpcFlag()`/`SetNpcFlag()`/`RemoveNpcFlag()`/`ReplaceAllNpcFlags()` (not `*Flag(UNIT_NPC_FLAGS, …)`); `IsRefundable()`/`IsBOPTradable()`/`IsWrapped()` (not `HasFlag(ITEM_FIELD_FLAGS, …)`); `HasFlag(ItemFlag)`/`HasFlag2(ItemFlag2)`/`HasFlagCu(ItemFlagsCustom)` (not bitwise `Flags & ITEM_FLAG…`); `ObjectGuid::ToString().c_str()` (not `ObjectGuid::GetCounter()`).

## Project conventions

- **Logging**: `LOG_INFO("category.sub", "msg with {}", arg)` (also `LOG_WARN`/`LOG_ERROR`/`LOG_DEBUG`/`LOG_TRACE`); hierarchical dot-separated categories (`server.loading`, `entities.player`, `sql.dev`). No `printf`-style, `sLog->`, or `TC_LOG_*`. Macro in `src/common/Logging/Log.h`.
- **Random**: helpers from `src/common/Utilities/Random.h` — `urand`, `irand`, `frand`, `rand32`, `rand_chance`, `roll_chance_f`, `roll_chance_i`. Never `std::rand` or `<random>` directly.
- **Strings**: `Acore::StringFormat(fmt, args...)` (`{}` placeholders) — `src/common/Utilities/StringFormat.h`.
- **Config**: `sConfigMgr->GetOption<T>("Name", default)`.
- **Namespace**: `Acore::` project-wide (no `Trinity::` remnants — rename when porting from upstream forks).
- **Long-lived references**: never store a raw `Player*`/`Creature*`/`Unit*` past the current call/tick — it can dangle (logout, despawn, instance unload). Store the `ObjectGuid`, resolve at use time via `ObjectAccessor::FindPlayer(guid)`, `ObjectAccessor::GetCreature(*from, guid)`, `Map::GetCreature(guid)`, etc.
- **DB queries**: `PreparedStatement` (via `WorldDatabase`/`CharacterDatabase`/`LoginDatabase` and the prepared-statement enums), not raw query strings. Non-blocking reads go async: `_queryProcessor.AddCallback(db.AsyncQuery(stmt).WithPreparedCallback(...))` (`WithCallback` for non-prepared). Multi-statement writes: `SQLTransaction` + `Execute`/`AppendPreparedStatement`.
- **Timed actions in AI**: `EventMap` (event id → delay; simple) or `TaskScheduler` (lambdas, repeats, cancellation) — both `CreatureAI` members; see any boss script for examples. Don't roll your own tick counters.

## Scripting registration

Scripts inherit from a `ScriptObject` subclass (`SpellScript`, `AuraScript`, `CreatureScript`, `InstanceMapScript`, `GameObjectScript`, `CommandScript`, …). Registration inside `AddSC_<name>()`: spell/aura scripts use `RegisterSpellScript(ClassName)` (or `RegisterSpellAndAuraScriptPair(...)`); creature scripts prefer `RegisterCreatureAI(ClassName)` for new code (legacy zones use `new ClassName();` — match the surrounding pattern). Declare and call `AddSC_<name>()` from the regional loader (`Spells/spells_script_loader.cpp`, `EasternKingdoms/eastern_kingdoms_script_loader.cpp`, …).

**SmartAI** (data-driven creature behaviour) lives in the world DB's `smart_scripts` table, not C++ (engine: `src/server/game/AI/SmartScripts/`). Prefer SmartAI for new creature behaviour (via the SQL update workflow); use `CreatureScript` only when SmartAI's event/action vocabulary isn't enough.

**Module hooks** (`OnPlayerLogin`, `OnWorldUpdate`, `OnSpellCast`, …) are declared in `src/server/game/Scripting/ScriptDefines/*.h`. Inherit the matching base (`PlayerScript`, `WorldScript`, …), register with `new MyClass();` (or its `RegisterXxxScript` macro) inside `AddSC_<name>()`. Full hook list: https://www.azerothcore.org/wiki/hooks-script.

Custom (non-upstream) scripts go in `src/server/scripts/Custom/` (gitignored).
