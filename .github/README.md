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
| [felworld/mod-ollama-chat](https://github.com/felworld/mod-ollama-chat) (`modules/mod-ollama-chat`) | [DustinHendrickson/mod-ollama-chat](https://github.com/DustinHendrickson/mod-ollama-chat) | LLM-powered bot chat via Ollama |
| [felworld/configs](https://github.com/felworld/configs) (`env/dist/etc`) | — | Our playtested server configs |

The forks track their upstreams; we periodically merge improvements back in.
The two modules and the configs repo are wired in as git submodules, so clone
with `--recurse-submodules` (or run `git submodule update --init` after).

## Running

Only containerized usage is supported — the upstream "install from source"
instructions don't apply here. We develop on Linux with Podman (using
`docker-compose` via `podman compose`); Docker on Linux, Docker Desktop on
Windows/macOS, and OrbStack on macOS should work equally well. The containers
are intended to build and run out of the box:

```sh
git clone --recurse-submodules https://github.com/felworld/azerothcore
cd azerothcore
podman compose --env-file .env.dumbbots up -d
```

First startup takes a while: `ac-client-data-init` downloads the client data
files and `ac-db-import` populates the databases before `ac-worldserver`
comes up.

### Session modes

Which flavor of Felworld you get is chosen at startup with a per-mode env
file:

| Mode          | Command                                          | Playerbots | LLM chat (Ollama) |
| ------------- | ------------------------------------------------ | :--------: | :---------------: |
| Solo          | `podman compose --env-file .env.solo up -d`      |    off     |        off        |
| Dumb bots     | `podman compose --env-file .env.dumbbots up -d`  |     on     |        off        |
| Ollama + bots | `podman compose --env-file .env.ollama up -d`    |     on     |        on         |

Switching modes is just a different `--env-file` + `up -d`. It only recreates
`ac-worldserver` (the database and authserver keep running) and needs no
rebuild or `.conf` edit: each env file sets `AC_AI_PLAYERBOT_ENABLED` /
`AC_OLLAMA_CHAT_ENABLE`, which the server reads in preference to the
`AiPlayerbot.Enabled` / `OllamaChat.Enable` module options.

Notes:

- Only `.env.ollama` starts the GPU-backed `ac-ollama` container (gated by
  `COMPOSE_PROFILES=ollama`); the other modes never start it, so it uses no
  VRAM. On first run it pulls the configured model.
- A bare `podman compose up -d` (no `--env-file`) defaults to bots-on /
  Ollama-off.
- Switching *away* from Ollama does not stop an already-running `ac-ollama` —
  stop it explicitly with `podman compose --profile ollama stop ac-ollama` to
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
    command.
  - A `grind quests` strategy — tell a bot `!grind quests` in chat and it
    proactively pulls mobs that anyone in the group still needs for a quest
    (and only those), so questing with a bot feels like questing with a
    person (see the mod-playerbots README).
- **Runtime admin toggles**: `.playerbots enable|disable|status` and
  `.ollama enable|disable|status` GM commands flip bots and LLM chat live,
  without a restart (see the module repos).
- **Container/infra**: rootless-Podman compatibility, GPU passthrough to
  Ollama via CDI, module and client-data volumes mounted at runtime (no image
  rebuild to pick up changes), and MySQL tuned for the playerbots write load.

A high-level tour of the C++ codebase is in
[`doc/CodebaseOverview.md`](../doc/CodebaseOverview.md). For general
AzerothCore documentation (database schema, scripting, GM commands), the
[upstream wiki](https://www.azerothcore.org/wiki) still applies.

## License

AzerothCore and mod-playerbots are GNU GPL v2; mod-ollama-chat is GNU AGPL
v3. Felworld's changes carry the same licenses. Not affiliated with or
endorsed by Blizzard Entertainment.
