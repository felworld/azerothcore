![Felworld logo](felworld.png)

# Felworld

Felworld (so-called after the various parks in the Westworld TV series)
replicates the feeling of an MMORPG in a mostly single-player context: a
private World of Warcraft 3.3.5a (Wrath of the Lich King) server where "other
players" are emulated by AI.

This repo is the server core, forked from
[mod-playerbots/azerothcore-wotlk](https://github.com/mod-playerbots/azerothcore-wotlk)
(the AzerothCore fork required by mod-playerbots, itself forked from
[azerothcore/azerothcore-wotlk](https://github.com/azerothcore/azerothcore-wotlk)).

## The Felworld repos

| Repo | Forked from | What it is |
| ---- | ----------- | ---------- |
| [felworld/azerothcore](https://github.com/felworld/azerothcore) (this one) | [mod-playerbots/azerothcore-wotlk](https://github.com/mod-playerbots/azerothcore-wotlk) | Server core, container setup, gameplay tweaks |
| [felworld/mod-playerbots](https://github.com/felworld/mod-playerbots) (`modules/mod-playerbots`) | [mod-playerbots/mod-playerbots](https://github.com/mod-playerbots/mod-playerbots) | The playerbots themselves |
| [felworld/mod-llm](https://github.com/felworld/mod-llm) (`modules/mod-llm`) | — | Agentic LLM-driven bots (tool-call architecture, vLLM) |
| [felworld/mod-ah-bot-plus](https://github.com/felworld/mod-ah-bot-plus) (`modules/mod-ah-bot-plus`) | [NathanHandley/mod-ah-bot-plus](https://github.com/NathanHandley/mod-ah-bot-plus) | Stocks and trades on the auction house |
| [felworld/configs](https://github.com/felworld/configs) (`env/dist/etc`) | — | Our playtested server configs |

The forks track their upstreams; we periodically merge improvements back in.
The modules and the configs repo are wired in as git submodules, so clone
with `--recurse-submodules` (or run `git submodule update --init` after).

mod-llm is our own module (no upstream). Where its predecessor
(mod-ollama-chat, a fork of DustinHendrickson/mod-ollama-chat) prompted the
LLM for chat replies, mod-llm gives the model a set of *tools* — send a
message, emote, adjust its opinion of a player, invite to a party, challenge
to a duel — and lets it decide what, if anything, to do. See its README for
the architecture.

## Running

Only containerized usage is supported — the upstream "install from source"
instructions don't apply here. We develop on Linux with Podman (using
`docker-compose` via `podman compose`); Docker on Linux, Docker Desktop on
Windows/macOS, and OrbStack on macOS should work equally well. The containers
are intended to build and run out of the box:

```sh
git clone --recurse-submodules https://github.com/felworld/azerothcore
cd azerothcore
podman compose build
podman compose --env-file .env.dumbbots up -d
```

First startup takes a while: `ac-client-data-init` downloads the client data
files and `ac-db-import` populates the databases before `ac-worldserver`
comes up.

### Session modes

Which flavor of Felworld you get is chosen at startup with a per-mode env
file:

| Mode          | Command                                          | Playerbots | LLM bots |
| ------------- | ------------------------------------------------ | :--------: | :------: |
| Solo          | `podman compose --env-file .env.solo up -d`      |    off     |   off    |
| Dumb bots     | `podman compose --env-file .env.dumbbots up -d`  |     on     |   off    |
| LLM bots      | `podman compose --env-file .env.llm up -d`       |     on     |    on    |

Switching modes is just a different `--env-file` + `up -d`. It only recreates
`ac-worldserver` (the database and authserver keep running) and needs no
rebuild or `.conf` edit: each env file sets `AC_AI_PLAYERBOT_ENABLED` /
`AC_LLM_ENABLE`, which the server reads in preference to the
`AiPlayerbot.Enabled` / `LLM.Enable` module options.

Notes:

- Only `.env.llm` starts the GPU-backed `ac-vllm` container (gated by
  `COMPOSE_PROFILES=llm`); the other modes never start it, so it uses no
  VRAM. On first run it downloads the configured model from Hugging Face
  (export `HF_TOKEN` first to avoid anonymous rate limits). The default is
  NVIDIA's DGX Spark agent-ready recipe (Qwen3.6-35B-A3B, NVFP4) and needs
  Spark-class memory — on smaller GPUs override `VLLM_MODEL`, plus
  `VLLM_TOOL_PARSER` / `VLLM_REASONING_PARSER` to match the model family,
  and `VLLM_EXTRA_ARGS` for anything else — see `docker-compose.yml`.
- A bare `podman compose up -d` (no `--env-file`) defaults to bots-on /
  LLM-off.
- Switching *away* from LLM mode does not stop an already-running `ac-vllm` —
  stop it explicitly with `podman compose --profile llm stop ac-vllm` to
  free VRAM.
- The worldserver console (account creation, GM commands, etc.) is the
  `ac-worldserver` container's TTY: `podman attach ac-worldserver` (detach
  with `Ctrl-p Ctrl-q`, not `Ctrl-c`).

## Configuration

All server configs live in the [felworld/configs](https://github.com/felworld/configs)
submodule at `env/dist/etc/`, which is bind-mounted into the server
containers. It versions our "best" settings as determined through
playtesting — see its README for the `.conf` / `.conf.dist` conventions and
how to update configs when upstream templates change.

## Tests

The C++ unit tests (`src/test/`, Google Test) are not built or run by
default. To build and run them in a container:

```sh
apps/docker/run-unit-tests.sh
```

The script builds the `unit-tests` stage of the server Dockerfile, which
recompiles the sources with `-DBUILD_TESTING=ON` and runs the suite as the
last build step — a successful image build means a green test run. It uses
Podman if available and Docker otherwise (set `CONTAINER_ENGINE` to
override), so the same command works locally and in CI.

The same script is the repo's only GitHub Actions workflow
([`unit-tests.yml`](workflows/unit-tests.yml), run on every push and pull
request, with the compiler cache persisted across runs); the upstream
workflows were removed since they only apply to the upstream repos.

Alongside the inherited upstream tests, the suite covers the fork's own
features where they are unit-testable: the `.pause` command, profession
skill-up XP, `XP.Kill.GroupMode`, and multi-drop quest items in the core,
plus — contributed from each module's `test/` directory via its
`<module>.cmake` — the playerbot `!` command prefix, the `.playerbots`
runtime toggle, and mod-llm's tool-call parsing, argument-schema validation,
prompt assembly, and name-mention matching.

## What we've changed

Beyond wiring the AI pieces together, the fork carries a number of
quality-of-life changes:

- **Gameplay** (all opt-in via config, enabled in our configs where noted):
  - `Rate.XP.Profession.SkillUp` — profession skill-ups grant a little XP,
    so crafting sessions aren't dead time.
  - `XP.Kill.GroupMode` — options for full (unsplit) kill XP while grouped, so
    grouping doesn't slow down leveling.
  - `Quests.MultiDropQuestItems` — allow one mob kill to drop a quest item
    for everyone in the group who needs it.
- **Playerbots**:
  - All playerbot commands require a `!` prefix: `!follow`, `!attack`,
    `!who warrior`, and so on (the
    [playerbot command list](https://github.com/mod-playerbots/mod-playerbots/wiki/Playerbot-Commands)
    still applies, just prefixed). Anything without the prefix is ordinary
    chat, so a sentence that happens to start with a command word ("who said
    that?") gets an LLM reply instead of being silently swallowed as a
    command. To make room for this, the core's legacy `!` alias for GM
    commands is disabled — GM commands use `.` only.
  - A `grind quests` strategy — tell a bot `!grind quests` in chat and it
    proactively pulls mobs that anyone in the group still needs for a quest
    (and only those), so questing with a bot feels like questing with a
    person (see the mod-playerbots README).
  - Warsong Gulch teamwork — bots escort their flag carrier, peel off
    attackers closing in on it, station defenders in the flag room before
    the flag is taken, re-pick roles when they die, use Stealth, Prowl,
    and Shadowmeld where sneaking matters, and call incoming enemies in
    battleground chat (see the mod-playerbots README).
  - World PvP excursions — bots occasionally travel to enemy or contested
    towns (Southshore/Tarren Mill, the Crossroads, sometimes even Goldshire)
    to lurk and pick fights for a while, with stealth-class goading,
    level-gap-curved gankers, and defenders calling invaders out in
    LocalDefense; `.playerbots wpvp` GM commands provide a test hook and a
    kill switch (see the mod-playerbots README).
  - Bots emoting at each other no longer loop forever — bot-to-bot emote
    replies roll a configurable chance (`AiPlayerbot.EmoteReplyChanceToBots`),
    and only one bot from a crowd replies to a given emoter at a time
    (`AiPlayerbot.EmoteReplyClaimSeconds`), so exchanges trail off after a
    reply or two while replies to real players stay as-is (see the
    mod-playerbots README).
- **Runtime admin toggles**: `.playerbots enable|disable|status` and
  `.llm enable|disable|status|reload` GM commands flip bots and LLM behaviour
  live, without a restart (see the module repos).
- **World pause**: the `.pause [on|off]` GM command freezes all gameplay —
  creatures, spells, battlegrounds, environment, and playerbot decision-making —
  while chat and GM commands keep working. Without an argument it toggles.
- **Container/infra**: rootless-Podman compatibility, GPU passthrough to
  vLLM via CDI, module and client-data volumes mounted at runtime (no image
  rebuild to pick up changes), and MySQL tuned for the playerbots write load.

A high-level tour of the C++ codebase is in
[`doc/CodebaseOverview.md`](../doc/CodebaseOverview.md). For general
AzerothCore documentation (database schema, scripting, GM commands), the
[upstream wiki](https://www.azerothcore.org/wiki) still applies.

## License

AzerothCore and mod-playerbots are GNU GPL v2; Felworld's changes to them
carry the same licenses. mod-llm is MIT. Not affiliated with or endorsed by
Blizzard Entertainment.
