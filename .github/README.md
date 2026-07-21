# Felworld

Felworld is a tech demo: a persistent MMO world populated by AI "players" —
a mixture of LLM agents and classical game AI — that quest, group, trade,
banter, and fight alongside (and against) you. It replicates the feeling of
playing an MMORPG in a mostly single-player context, and doubles as a
testbed for agentic LLM behaviour embedded in a live game loop.

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

The forks track their upstreams; a weekly GitHub Actions run
([`upstream-sync.yml`](workflows/upstream-sync.yml)) checks for new upstream
commits and, when there are any, has [Claude Code](https://claude.com/claude-code)
merge them, regenerate the configs, and open a PR per affected repo — see
[`UPSTREAM_SYNC.md`](UPSTREAM_SYNC.md) for the procedure it follows.
The modules and the configs repo are wired in as git submodules, so clone
with `--recurse-submodules` (or run `git submodule update --init` after).

mod-llm is our own module (no upstream). Where its predecessor
(mod-ollama-chat, a fork of DustinHendrickson/mod-ollama-chat) prompted the
LLM for chat replies, mod-llm gives the model a set of *tools* — send a
message, emote, write or delete a note in its persistent private memory,
invite to a party, challenge to a duel — and lets it decide what, if
anything, to do. See its README for the architecture.

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
comes up. To actually play you connect with your own 3.3.5a game client —
none of the client or its assets is included in these repos (see the
[disclaimer](#license-and-disclaimer) below).

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

The same script runs in CI as the [`unit-tests.yml`](workflows/unit-tests.yml)
workflow (on every push and pull request, with the compiler cache persisted
across runs); the upstream workflows were removed since they only apply to
the upstream repos.

Alongside the inherited upstream tests, the suite covers the fork's own
features where they are unit-testable: the `.pause` command, profession
skill-up XP, `XP.Kill.GroupMode`, and multi-drop quest items in the core,
plus — contributed from each module's `test/` directory via its
`<module>.cmake` — the playerbot `!` command prefix, the `.playerbots`
runtime toggle, and mod-llm's tool-call parsing, argument-schema validation,
prompt assembly, and name-mention matching.

## What we've changed

Beyond wiring the AI pieces together, the fork carries a number of
quality-of-life changes — the full tour is in
[FEATURES.md](../FEATURES.md), with the module details in each module's own
FEATURES.md:

- **Gameplay options** (opt-in via config) — XP from profession skill-ups,
  unsplit group kill XP, multi-drop quest items.
- **`!` command prefix** for playerbot commands, so ordinary chat is never
  silently swallowed as a command.
- **Quest-aware grinding** — `!grind quests` makes a bot pull only what its
  group still needs, like a questing partner.
- **Warsong Gulch teamwork** — flag-carrier escorts and peels, flag-room
  defenders, stealthy approaches, incoming callouts.
- **World PvP** — bots travel to enemy towns to lurk and pick fights;
  defenders call out and hunt gankers, and beaten gankers pull
  reinforcements — real players included, on both ends.
- **Social behaviours** — bots rescue strangers about to die, buff
  passers-by, group up over quest-spawn competition, and end emote
  exchanges instead of looping them.
- **Runtime admin toggles** — `.playerbots` and `.llm` GM commands flip
  bots and LLM behaviour live; `.pause` freezes all gameplay while chat
  and GM commands keep working.
- **Container/infra** — rootless Podman, GPU passthrough to vLLM via CDI,
  runtime-mounted module/data volumes, MySQL tuned for the bot write load.

A high-level tour of the C++ codebase is in
[`doc/CodebaseOverview.md`](../doc/CodebaseOverview.md). For general
AzerothCore documentation (database schema, scripting, GM commands), the
[upstream wiki](https://www.azerothcore.org/wiki) still applies.

## License and disclaimer

AzerothCore and mod-playerbots are GNU GPL v2; Felworld's changes to them
carry the same licenses. mod-llm is MIT.

Felworld is a non-commercial research and educational project. These repos
contain no game client, no client assets, and no proprietary Blizzard
Entertainment code; you must supply your own legally obtained game client
and comply with the laws of your jurisdiction. Felworld is not affiliated
with or endorsed by Blizzard Entertainment.
