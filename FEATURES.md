# Felworld features

Everything the Felworld fork adds or changes, in one place. Features that
live in a module are summarized here and documented in full in that module's
own `FEATURES.md`; features of the server core itself are documented here.

## Gameplay options

All opt-in via config, enabled in [our configs](https://github.com/felworld/configs)
where noted:

- `Rate.XP.Profession.SkillUp` — profession skill-ups grant a little XP,
  so crafting sessions aren't dead time.
- `XP.Kill.GroupMode` — options for full (unsplit) kill XP while grouped, so
  grouping doesn't slow down leveling.
- `WorldPvP.GiveXPForKills` — open-world PvP kills grant XP, mirroring what
  `Battleground.GiveXPForKills` does inside battlegrounds; arena kills stay
  XP-free, and gray-level victims give nothing.
- `Quests.MultiDropQuestItems` — allow one mob kill to drop a quest item
  for everyone in the group who needs it.
- `Rate.MoveSpeed.Ghost` — movement speed rate for dead player ghosts
  (corpse runs), with a module hook to adjust it per player —
  [mod-playerbots](https://github.com/felworld/mod-playerbots/blob/main/FEATURES.md#corpse-run-pacing)
  uses it so bot corpse runs can be paced separately from human ones. Our
  configs double it for humans and cancel that back out for bots.
- `Death.CorpseReclaimDelay.Steps` — the three escalating corpse reclaim
  delays for deaths in quick succession, hardcoded upstream as 30/60/120
  seconds. Our configs halve them to 15/30/60 so death chains sting less.
- `Death.SicknessRealPlayers` — set to `0` to spare real (human) players
  the resurrection sickness debuff at spirit healers; playerbots still
  follow `Death.SicknessLevel`, so instant bot rezzes keep their cost. Our
  configs disable it for humans.

## Login and logout announcements

`ShowLoginLogoutInWorld` broadcasts a system message to everyone online when
a player logs in on a character ("*Name* has come online.") or logs out of
one ("*Name* has gone offline."), in the same vein as upstream's
`ShowKickInWorld` / `ShowBanInWorld`. It fires for both fresh logins and
reconnects to a character still in the world, and on every kind of logout
(including disconnects and kicks); the departing player doesn't get told
about their own logout. The strings live in `acore_string` entries 4600001
and 4600002, translated into all nine client locales, so the wording is
editable in the world DB.

Bot sessions are never announced — with 1500+ playerbots rotating in and out
the channel would be nothing but login spam, so only real client sessions
count. That makes this a "who else is actually here" signal rather than a
population ticker. Default `0` upstream, `1` in
[our configs](https://github.com/felworld/configs).

## Joinable WorldDefense channel

On 3.3.5 clients WorldDefense is a stranded relic: the vanilla PvP-rank
gate that governed joining it left with the old honor system, so the client
never auto-joins it, never shows it in the channel lists, and gives
`/join WorldDefense` none of the usual feedback — while the server happily
force-joined players into it on every zone change.

Felworld makes it a normal opt-in channel instead: the core never
force-joins anyone, and
[mod-playerbots](https://github.com/felworld/mod-playerbots) creates it as
a custom channel (no client-side DBC identity) that bots enter at login.
`/join WorldDefense` now behaves like joining any player-made channel —
join confirmation, `/chatlist` and chat-pane listing, `/leave` — and it
carries the bots' world-PvP pleas and shouts, so joining it is tuning into
your faction's defense chatter. A `channels_rights` row keeps it
civilized: no join/leave announces (bots rotate in and out constantly), no
ownership or moderation, no persistence across restarts.

## Instant boat and zeppelin travel

Every boat dock and zeppelin tower has a resident **Expediter** NPC — the
transport-network analogue of `InstantFlightPaths`. Each one offers exactly
the destinations that dock's real transports serve (Menethil's ferryman
sells passage to Theramore and Valgarde, the Orgrimmar dispatchers cover
Undercity, Grom'gol, Thunder Bluff, and Warsong Hold, and so on) and
teleports you to the far dock on the spot, free of charge. The scheduled
boats and zeppelins still run — the expediters just let you skip the wait.
All 3.3.5a routes are covered: Alliance ships, Horde zeppelins, the neutral
Booty Bay–Ratchet line, and the Dragonblight turtle ferries. Implemented as
pure world content (gossip + SmartAI, one SQL update); always on, no
config.

## GM accounts as players

On Felworld the humans usually hold GM accounts, so upstream's GM social
isolation mostly gets in the way. Two changes:

- **GM-to-GM exemptions (always on)** — between two GM accounts, whispers
  bypass the accept-whispers filter, friends lists show real online status,
  and group invites always go through. Upstream hard-codes GM characters as
  offline/unreachable with no exemption even when the viewer is also a GM.
- **`GM.AllowFriend` now governs friends-list status too** — upstream only
  applied it to *adding* a GM friend, while the status display stayed
  hard-coded to offline; now the option controls both. Our configs default
  `GM.AllowFriend`, `GM.AllowInvite`, and `GM.WhisperingTo` to open, so GM
  characters are messageable, friendable, and invitable by everyone.
  Per-character toggles (`.whisper off`, `.gm visible off`, …) still work.

## Playerbots

Quality-of-life changes in our mod-playerbots fork; each is documented in
full in [mod-playerbots' FEATURES.md](https://github.com/felworld/mod-playerbots/blob/main/FEATURES.md):

- **Command prefix**: all playerbot commands require a `!` prefix —
  `!follow`, `!attack`, `!who warrior`, and so on (the
  [playerbot command list](https://github.com/mod-playerbots/mod-playerbots/wiki/Playerbot-Commands)
  still applies, just prefixed). Anything without the prefix is ordinary
  chat, so a sentence that happens to start with a command word ("who said
  that?") gets an LLM reply instead of being silently swallowed as a
  command. To make room for this, the core's legacy `!` alias for GM
  commands is disabled — GM commands use `.` only.
- **Quest-aware grinding**: tell a bot `!grind quests` and it proactively
  pulls mobs that anyone in the group still needs for a quest (and only
  those), so questing with a bot feels like questing with a person.
- **Warsong Gulch teamwork**: bots escort their flag carrier, peel off
  attackers closing in on it, station defenders in the flag room before
  the flag is taken, re-pick roles when they die, use Stealth, Prowl,
  and Shadowmeld where sneaking matters, and call incoming enemies in
  battleground chat.
- **Bystander assist**: solo bots rescue nearby non-group players — real
  players included — who look like they're about to die, healing or
  charging in when the fight looks winnable.
- **Social buffing**: idle bots cast their class buff on passers-by who
  lack it, buff back whoever buffs them, and thank strangers for heals.
- **Quest-competition groups**: a bot competing with you for quest spawns
  invites you to group, grinds alongside you as a peer, and politely
  leaves when the shared objectives are done.
- **World PvP excursions**: bots occasionally travel to enemy or contested
  towns (Southshore/Tarren Mill, the Crossroads, sometimes even Goldshire)
  to lurk and pick fights for a while, with goading emotes at unflagged
  passers-by and level-gap-curved gankers; `.playerbots wpvp` GM commands
  provide a test hook and a kill switch.
- **World PvP defense and reinforcements**: gank sprees draw
  LocalDefense/WorldDefense callouts from eyewitnesses, idle bots travel in
  to hunt the attacker, and a ganker beaten by outside help can pull a wave
  of faction reinforcements — all keyed off actual PvP kills, so it works
  the same when the ganker or victim is a real player.
- **World PvP threat reactions**: attacked in the open world, bots get up
  from meals, abort long casts (unless nearly done), turn from mobs to
  the player attacking them, hold a soulstone res while the killer lurks,
  and wait out corpse campers instead of rezzing into them.
- **Bandage crafting**: idle bots with First Aid craft bandages from the
  cloth they carry, keep the stock level-appropriate (no level 55 hoarding
  linen bandages), and use their best bandage first.
- **Engineering in combat**: engineer bots stock skill-appropriate
  explosives and gadgets and actually use them — bombs and grenades
  thrown at targets that matter, stun-grenade interrupts, sapper charges
  when surrounded, target dummies to shed aggro, explosive sheep, Nitro
  Boosts popped on Warsong Gulch flag runs, glove tinkers on cooldown,
  and jumper-cable resurrection attempts by bots with no real rez spell.
- **Emote exchanges that end**: bot-to-bot emote replies roll a
  configurable chance and only one bot from a crowd replies to a given
  emoter at a time, so exchanges trail off after a reply or two while
  replies to real players stay as-is.
- **Faction-honest chat**: bots speak their faction's language, honor
  `AllowTwoSide.Interaction.Chat`, and ignore speech they couldn't
  understand.

## LLM bots

mod-llm is Felworld's own module: instead of prompting a model for chat
replies, it hands the model a tool list (speak, emote, remember/forget
notes, invite to party, challenge to a duel) per situation and lets it
decide what — if anything — to do, with LLM-routed replies, human-paced
typing, persistent per-bot memory, and event narration. See
[mod-llm's README](https://github.com/felworld/mod-llm#readme) for the
architecture and [its FEATURES.md](https://github.com/felworld/mod-llm/blob/main/FEATURES.md)
for the full behavior reference.

## Runtime admin toggles

`.playerbots enable|disable|status` and `.llm enable|disable|status|reload`
GM commands flip bots and LLM behaviour live, without a restart (see the
module repos). Felworld's session modes drive the same switches at startup
via the `AC_AI_PLAYERBOT_ENABLED` / `AC_LLM_ENABLE` env vars.

## World pause

The `.pause [on|off]` GM command freezes all gameplay — creatures, spells,
battlegrounds, environment, and playerbot decision-making — while chat and
GM commands keep working. Without an argument it toggles.

## Per-player XP rate

The `.modify xp <rate>` GM command sets an XP-gain multiplier for the
selected player (yourself if no one is selected), applied to every XP
source — kills, quests, exploration, battlegrounds, dungeon-finder
rewards — with rested and recruit-a-friend bonuses scaling to match.
`1` is normal, `0` disables XP gain, and `.modify xp` without a value
shows the current rate. Like the other `.modify` commands, the rate
resets at logout.

## Account lookup by character

The `.lookup player character [$name] [$limit]` GM command (also available
from the console) lists every character on the account a character belongs
to — the named character's account, or the selected player's (yourself if
no one is selected) when no name is given. It complements upstream's
`.lookup player account`, which does the same starting from an account
name; both show each character's race, class, level, and online status.

## Observability

An optional monitoring stack (compose profile `obs`, on by default in every
tracked session mode; a bare `up -d` starts none of it) gives the whole
Felworld deployment a web dashboard on the host's LAN:

- **VictoriaMetrics** ingests the worldserver's built-in metrics — the
  stock AzerothCore metrics client speaks InfluxDB line protocol, which
  VictoriaMetrics accepts natively, so no core changes were needed for the
  transport. Emission is toggled per session mode via `AC_METRIC_ENABLE`
  (all tracked `.env.<mode>` files set it alongside the profile).
- **Vector + VictoriaLogs** persist and index every container's
  stdout/stderr. The console appenders are configured to prefix lines with
  `LEVEL [category]` so severity and category become queryable fields;
  timestamps come from the container engine. Logs survive container
  replacement and are searchable from the browser (LogsQL).
- **Grafana** (port 3000, anonymous read-only viewer; admin login for
  editing via `GRAFANA_ADMIN_PASSWORD`) serves dashboards provisioned from
  [`apps/observability/`](apps/observability/) — dashboards are code, not
  click-ops:
  - **Server Health** — tick-time percentiles (the `.server info`
    distribution, continuous), world-update phase timings, map update
    times, DB queue depths, login/logout churn, worldserver warn/error
    log rate, WPvP callout annotations.
  - **Bot Census** — level histogram by faction, class/race/role splits,
    engine and activity states, top zones, quest throughput, deaths (by
    world/BG/arena context, plus a deadliest-zones ranking), level-ups.
  - **Behavior** — chat sends by destination, broadcast sent-vs-suppressed
    rolls, WPvP defense-board events and excursion outcomes, battleground
    matches and wins by faction.
  - **Economy** — total and per-faction bot gold, wealth by level
    bracket, richest characters, live auction-house listings (per house,
    top items, newest posts), mail in transit, guild and party counts —
    straight from the characters DB plus the census gold series.
  - **LLM** — endpoint status, latency percentiles, request/failure and
    token rates, queue depth, conversation-depth histogram, tool calls,
    and a *find utterance* search over the full exchange trace: type a
    phrase a bot said and read the exact prompt that produced it
    ([details](https://github.com/felworld/mod-llm/blob/main/FEATURES.md#exchange-trace)).
  - **Character Inspector** — pick any character: their `felworld_events`
    timeline, mod-llm memory scratchpad, recent conversations, and recent
    LLM exchanges (full prompt and response), live from the characters DB.
  - **Logs** — full log search across all containers.

Discrete per-character events (WPvP kills/deaths/callouts/excursions,
deaths, level-ups, LLM tool invocations) are recorded in a
`felworld_events` table in the
characters database via a small core helper (`Felworld::LogEvent`), purged
after `Felworld.Events.RetentionDays` (default 30; 0 keeps everything).
Metric and log retention default to 90 days (`OBS_METRICS_RETENTION` /
`OBS_LOGS_RETENTION`). The metrics/logs stores publish only loopback ports
for host-side debugging (VMUI on 8428, VictoriaLogs on 9428); Grafana is the
only LAN-visible surface.

## Container / infrastructure

Rootless-Podman compatibility, GPU passthrough to vLLM via CDI, module and
client-data volumes mounted at runtime (no image rebuild to pick up
changes), and MySQL tuned for the playerbots write load.

Service images are published to `ghcr.io/felworld/ac-*` as multi-arch
(linux/amd64 + linux/arm64) manifests, built on native runners by the `ci`
workflow after the unit tests pass — on every push to `main` (tagged `main`,
the compose default, with `latest` as an alias) and on every git tag (tagged
with the tag name; `DOCKER_IMAGE_TAG` selects one). The compose services set
`pull_policy: missing`, so a fresh checkout pulls the prebuilt images while
an explicit `docker compose build` still takes precedence. The dev-server
image is dev-only and never published.

---

Felworld is a non-commercial research project. It contains no game client,
assets, or proprietary code, and is not affiliated with or endorsed by
Blizzard Entertainment — see the
[project disclaimer](.github/README.md#license-and-disclaimer).
