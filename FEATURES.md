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
- `Quests.MultiDropQuestItems` — allow one mob kill to drop a quest item
  for everyone in the group who needs it.

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

## Container / infrastructure

Rootless-Podman compatibility, GPU passthrough to vLLM via CDI, module and
client-data volumes mounted at runtime (no image rebuild to pick up
changes), and MySQL tuned for the playerbots write load.

---

Felworld is a non-commercial research project. It contains no game client,
assets, or proprietary code, and is not affiliated with or endorsed by
Blizzard Entertainment — see the
[project disclaimer](.github/README.md#license-and-disclaimer).
