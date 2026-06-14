# Codebase Overview

A conceptual, high-level tour of this repository: AzerothCore itself (this is the
**mod-playerbots fork**) plus the two bundled modules in `modules/`. It is written for
someone familiar with WoW and hobby gamedev but new to this codebase. It favors mental
models over file paths; for granular detail, read the code or the per-area `CLAUDE.md`
notes.

## What AzerothCore actually is

When you play WoW, the game client (the `.exe`) is mostly a renderer + input handler. It
doesn't *know* the rules — it asks the server "I pressed Fireball, what happens?" and the
server decides. **AzerothCore is a from-scratch reimplementation of that server side**, for
the WotLK 3.3.5a client specifically. An unmodified WoW client connects to it and can't
really tell it isn't Blizzard. So the whole codebase exists to: *be the authoritative
simulation of the game world, enforce the rules, and persist everyone's data.*

It runs as **two programs**, which is normal for online games:

- A **login server** (authserver) — the bouncer. Handles username/password and hands you
  the list of realms. Listens on port 3724.
- A **world server** (worldserver) — the actual game. Once you pick a realm, your client
  connects here, and this is where the world is simulated. Listens on port 8085.

A client first authenticates against the login server, receives a session key and the realm
list, then connects to the chosen realm's world server and proves identity with that key.

## The world server is just a game loop

At its heart it's the same `while(true)` update loop any game engine has: every few
milliseconds, tick everything forward by a delta time. Combat, movement, spells, respawns —
all advanced one step per tick.

The one MMO-specific trick worth understanding: **you can't simulate an entire continent for
thousands of players every tick.** So each map is chopped into a grid, and only the cells
with players near them are "awake" and ticking. Empty wilderness goes dormant. It's the same
idea as chunk loading in Minecraft or culling in any engine — *simulate only what someone is
around to see.* This is the single biggest reason the server scales, and it shapes a lot of
the code.

A consequence that matters if you touch the code: because monsters and even players blink in
and out as grids load/unload, you're not allowed to hold onto a pointer to a creature across
ticks — you store its ID (a GUID) and re-look-it-up at use time. Same discipline as not
caching a reference to a pooled/destroyed object in a game engine.

Everything in the world — you, a monster, a chest, a dropped item — is an "Object" in a
class hierarchy, where players and monsters share a common "Unit" base (both have health,
can cast, can fight). When something changes (a mob's health drops), the server replicates
that to nearby clients. Standard networked-game state replication; nothing exotic.

Networking detail worth knowing: reading packets off the socket happens on separate network
threads, but the actual game logic for those packets runs on the main world thread. I/O is
parallel; simulation is single-threaded.

## The most important idea: engine vs. content

This is the thing to internalize, and it'll feel familiar from gamedev. There's a hard split
between:

- **The C++ code = the engine.** It knows what "deal damage," "apply an aura," "move to a
  point," "a spell with effect X does Y" *mean*. The actual rules and math live here and are
  compiled.
- **A MySQL database = the content.** What monsters exist, their stats, where they spawn,
  their loot tables, every item, every quest's text and objectives, vendor inventories —
  none of that is in the code. It's **data, in database rows.**

So "building out the game" is overwhelmingly *editing database rows*, not recompiling. It's
exactly like Unity-engine-vs-your-scene-and-prefabs, or the D&D rulebook (engine) vs. a
specific monster manual and adventure module (content).

There are **three databases**:

- `acore_auth` — accounts, realm list, bans. Shared across realms.
- `acore_characters` — everyone's saved characters, inventory, mail, guilds. Changes
  constantly. One per realm.
- `acore_world` — the static catalog of what exists (creatures, items, quests, spawns, loot).
  Read constantly, rarely changes.

### What's hardcoded vs. data-driven

- **Hardcoded in C++:** the *rules* — combat formulas, what each spell effect actually does,
  movement, stat calculations. A database row for a spell only *picks* which effect and its
  numbers; the implementation of that effect is compiled code.
- **In the database:** the *catalog* of content, plus simple scripted behavior.
- **The interesting middle ground is monster behavior.** Simple stuff ("at 50% HP cast
  Enrage; on aggro yell this line") is written as data rows in a system called **SmartAI** —
  think of it as a built-in visual-scripting/event system, no code needed. But a complex
  multi-phase raid boss needs real C++. That trade-off — data-driven and quick but limited,
  vs. coded and unlimited but needs a rebuild — recurs all over the codebase.

## Modules = the mod/plugin system

`modules/` is the plugin folder. A module is a self-contained folder that can **hook into
events** the core broadcasts — "a player logged in," "the world ticked," "someone cast a
spell" — and react, *without modifying the core code*. At build time the modules get folded
in automatically (compiled into the server). This is how you extend the server cleanly. Both
of the bundled things below are just modules.

## mod-playerbots — the big one

The problem it solves: a private server is lonely. Playerbots fills the world with **AI that
plays actual characters** — not NPCs, real player-characters driven by the computer. Two
flavors:

- **Your bots** — you summon some to group up and play alongside you (like having your alts
  follow and help).
- **The random-bot population** — a background crowd of bots that log in, level, quest, run
  dungeons, and roam, so the world feels alive even on an empty server.

The neat technical trick: **a bot is a completely real character in the world** — loaded from
the database, standing in the world, usable on the auction house — it just has the AI driving
it instead of a network connection from a real client. The rest of the server treats it like
any player. (The one change the core needed to support this is a flag marking a session as a
bot, since a bot has no real network socket behind it.)

### How the AI thinks

If you know the usual gamedev options — finite state machines, behavior trees, utility AI —
**this is utility AI**, and leaning on that framing is the fastest way to get it.

It is *not* a state machine and *not* a behavior tree. Instead, **every tick the bot asks:
"what are all the things I could do right now, and how good is each one?"** Each candidate
action gets a numeric **score** (the code calls it "relevance"), and the bot performs the
single highest-scoring action that's actually possible right now. Then it does it all again
next tick. Nothing is remembered as "I am in state X"; the world is just re-evaluated
continuously.

The pieces that feed this:

- **Triggers** are conditions that watch the world: "I'm below 20% health," "a Brain Freeze
  proc just lit up," "there's an enemy in melee range." When a trigger fires, it **proposes
  one or more actions with a score attached.**
- **Actions** are the things a bot does — cast a spell, move, loot, drink. Each action knows
  whether it's currently *possible* (in range? off cooldown? enough mana?) and can declare
  fallbacks (if I can't cast Frostbolt, try Shoot).
- **Values** are cached readings of world state (e.g. "current target," "am I low on
  health") that triggers and actions share, so the same thing isn't recomputed many times
  per tick.
- Plus there's always a baseline rotation proposed at low scores, so a bot with nothing
  urgent happening just does its normal DPS rotation.

**The scores create priority automatically.** Your filler rotation proposes actions around
score ~5. "I'm about to die → Ice Block" proposes at ~90. So emergencies naturally preempt
the rotation without any explicit state logic — it just falls out of "highest score wins."
There's a shared scale (idle ≈ 0, normal ≈ 10, movement ≈ 30, interrupt ≈ 40, raid mechanic
≈ 60, emergency ≈ 90).

**Strategies are togglable bundles of this behavior** — basically loadouts. A "frost"
strategy carries the frost rotation and its proc-reaction triggers; a "tank" strategy carries
tanking behavior; a "passive" strategy makes the bot do nothing. They **stack**, and this is
where the elegance is:

- A bot's class spec is just a set of strategies turned on (a Frost Mage gets frost + DPS +
  crowd-control + AoE strategies, chosen from its talents).
- A specific raid boss is *another* strategy layered on top — but it doesn't rewrite the
  rotation. It just **adds high-scoring actions** ("move out of the void zone" at
  raid-priority) and can **veto** lower ones (multiply "stand still and cast" by zero while
  this boss is active). So generic behavior + situational overrides compose cleanly instead
  of fighting each other. This is the cleverest design decision in the whole module.

There are three of these "engines" running the same machinery with different strategy sets
loaded — one for combat, one for out-of-combat, one for when the bot is dead — and the bot
switches between them.

Two practical notes:

- You command your bots by **whispering them** — a whisper becomes a trigger, which proposes
  actions. There are also chat shortcuts and `+`/`-` prefixes to toggle strategies.
- There's a big **performance optimization**: bots far from any real player drop to a
  near-idle mode, because you can't run a full AI every tick for thousands of them.

### Lifecycle and supporting systems

- Two managers run the show: one **per real player** (manages your summoned bots), and one
  **global singleton** that maintains the random-bot population (logs bots in/out to hit a
  target headcount, honoring faction balance).
- Random bots are backed by dedicated auto-created accounts (named like `rndbot0`,
  `rndbot1`, …), each holding one character per class.
- A **factory** equips and specs a bot for its level (talents, gear, enchants, glyphs,
  consumables, professions).
- Supporting managers handle guilds, item/gear scoring, travel (a waypoint graph the bots
  path over), talents, localized bot chat text, and a security system for who's allowed to
  command a given bot.
- Bots run on the map threads, but anything touching shared/global state (groups, guilds,
  LFG, even bot login) is queued and run on the main world thread to stay thread-safe.
- Playerbot state persists in its own fourth database connection (separate from the three
  core DBs).

## mod-ollama-chat — bots that talk

A small add-on (depends on playerbots) that makes bots **chat using a local LLM** via Ollama.
It hooks the chat and game-event systems: when you talk near a bot or something happens (it
dings, completes a quest, gets a kill), it builds a text prompt describing who the bot is and
what's going on, sends it to the LLM, and the bot says the reply in character.

The one architectural thing that matters: **LLM calls are slow (seconds), so they never run
on the game loop** — they're fired off on background threads and the answer is spoken
whenever it comes back, so the world never freezes waiting.

On top of the basics it adds:

- **Personalities** — each bot gets a persona (gamer, roleplayer, pirate, …) that shapes its
  tone.
- **Sentiment** — a value tracking how each bot feels about you, nudged over time by a second
  LLM call that judges whether your messages were positive or negative, then fed back into
  future prompts.
- **RAG (lore retrieval)** — optional lightweight retrieval from local knowledge files to
  ground answers, using simple text similarity (no embedding model).

## Cross-cutting themes

- **Single-threaded simulation, parallelism at the edges.** The world logic runs on one
  thread (with an optional worker pool for updating maps); network I/O, database workers, and
  LLM calls all live on other threads and hand work back to the main tick via queues.
- **Grid-based spatial partitioning** is the core scalability lever, and it's why you don't
  hold raw pointers to world objects across ticks.
- **Data-driven content, compiled rules.** Templates and simple AI live in SQL; mechanics and
  spell effects are compiled C++, with "dummy"/script hooks as the escape hatch when data
  isn't enough.
- **Uniform event hooks** let modules extend the server without touching the core — and
  playerbots rides this same system.
- **The playerbot AI is utility-based, not a behavior tree** — relevance-scored candidate
  actions, re-derived every tick, with strategies layered so higher concerns (specs, bosses)
  override lower ones purely through scores and zeroing multipliers, never by rewriting code.
