# Upstream sync runbook

This is the procedure for the weekly upstream sync, executed by the Claude
Code agent in [`workflows/upstream-sync.yml`](workflows/upstream-sync.yml)
(and equally followable by a human). The goal each cycle: merge new upstream
commits into the Felworld forks, regenerate the deployed configs, and open
one reviewable PR per affected repo.

## Repos and upstreams

| Checkout path | Felworld repo | Upstream | Branch |
| ------------- | ------------- | -------- | ------ |
| `.` (core) | felworld/azerothcore | mod-playerbots/azerothcore-wotlk | `Playerbot` |
| `modules/mod-playerbots` | felworld/mod-playerbots | mod-playerbots/mod-playerbots | `master` |
| `modules/mod-ah-bot-plus` | felworld/mod-ah-bot-plus | NathanHandley/mod-ah-bot-plus | `master` |
| `env/dist/etc` | felworld/configs | — (derived: config regen only) | |
| `modules/mod-llm` | felworld/mod-llm | — (entirely ours; **never touch**) | |

**Version coupling:** the core `Playerbot` branch and mod-playerbots are
maintained in lockstep upstream (their PRs land as matched pairs). Always
sync both in the same cycle — never merge one and leave the other's new
commits behind.

## Ground rules

- Merge, never rebase. Never push to `main`. Never force-push.
- All work happens on a branch named `upstream-sync/YYYY-MM-DD` (today's
  date, `date +%F`), the same name in every repo, each based on that repo's
  `origin/main`.
- Submodules check out with a detached HEAD; explicitly
  `git checkout -b upstream-sync/<date> origin/main` in each repo you touch.
- End every commit message with the Claude Code co-author trailer, and every
  PR body with `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.

## Procedure

### 1. Merge

For each of core / mod-playerbots / mod-ah-bot-plus that has new upstream
commits (counts are provided in the prompt; the `upstream` remotes are
already fetched):

1. Record the range for the PR body: `old=$(git rev-parse --short origin/main)`,
   `new=$(git rev-parse --short upstream/<branch>)`, and skim
   `git log --oneline origin/main..upstream/<branch>` — you'll summarize it later.
2. `git checkout -b upstream-sync/<date> origin/main`
3. `git merge upstream/<branch> --no-edit`
4. **On conflicts:** resolve them yourself. Follow the conventions in
   `CLAUDE.md`; if the resolution touched C++, run
   `python apps/codestyle/codestyle-cpp.py` before committing. Keep a list of
   every conflicted file and a one-line rationale for how you resolved it —
   the PR body must surface these prominently.

### 2. Regenerate configs

Template → deployed pair mapping (source template on the left, pair in
`env/dist/etc` on the right):

| Template (after merge) | Pair in felworld/configs |
| ---------------------- | ------------------------ |
| `src/server/apps/worldserver/worldserver.conf.dist` | `worldserver.conf` + `.conf.dist` |
| `src/server/apps/authserver/authserver.conf.dist` | `authserver.conf` + `.conf.dist` |
| `src/tools/dbimport/dbimport.conf.dist` | `dbimport.conf` + `.conf.dist` |
| `modules/mod-playerbots/conf/playerbots.conf.dist` | `modules/playerbots.conf` + `.conf.dist` |
| `modules/mod-ah-bot-plus/conf/mod_ahbot.conf.dist` | `modules/mod_ahbot.conf` + `.conf.dist` |

For each template the merges changed (`git diff origin/main..HEAD -- <template>`
in the owning repo; skip this whole step if none changed):

1. Once: `git -C env/dist/etc checkout -b upstream-sync/<date> origin/main`.
2. Save the old pair:
   `git -C env/dist/etc show origin/main:<pair>.conf > /tmp/old.conf` and
   likewise for `<pair>.conf.dist`.
3. Copy the new template verbatim: `cp <template> env/dist/etc/<pair>.conf.dist`.
4. Re-apply our overrides:
   `tools/regen-config.py /tmp/old.conf /tmp/old.dist env/dist/etc/<pair>.conf.dist env/dist/etc/<pair>.conf`.
5. Verify the pair is clean: `.conf` and `.conf.dist` have identical line
   counts, and `diff` between them shows value-only changes (our overrides).
6. If the script exits non-zero (an override's key was dropped upstream, or a
   stray key existed only in the old `.conf`): keep going, but flag it
   prominently in the configs PR body so the human decides what replaces it.

### 3. Commit

- Each touched submodule: commit on its sync branch (the merge commit from
  step 1; for configs, one commit with all regenerated pairs).
- Core: the merge commit from step 1, then a **separate** commit bumping
  every changed submodule pointer (`git add modules/mod-playerbots
  env/dist/etc ...`) to its sync-branch head. Even if core itself had no
  upstream commits, this pointer-bump commit (and its PR) still happens
  whenever any submodule synced.

### 4. Push and open PRs

Push each sync branch to its felworld `origin`, then create PRs with
`gh pr create --repo felworld/<repo> --base main`, **module and configs PRs
first, the core PR last** so it can link to them. After creating the core
PR, append a line to each earlier PR body (`gh pr edit --add-...` or body
rewrite) linking to the core PR.

Every PR body must include this merge-order warning verbatim:

> **Merge order:** merge the module/configs PRs first, each with a **merge
> commit (not squash)** — the core PR's submodule pointers reference these
> branch heads, and squashing would leave them unreachable. Merge the core
> PR last.

### 5. On failure

If a step fails in a way you cannot recover (a merge you cannot responsibly
resolve, pushes rejected, tooling broken): stop, clean up nothing, and open
an issue on felworld/azerothcore titled `Upstream sync failed <date>`
describing exactly what happened, what was completed, and what remains.

## PR description style

Write the digest a maintainer wants to read: lead with the commit count and
range, then group highlights by theme with upstream PR numbers, emphasizing
what matters to Felworld — playerbot/bot behaviour, hooks mod-llm could use,
battlegrounds/loot/mail/AH, config surface changes. Not a raw commit list;
select and summarize. Model the tone and structure on this abbreviated
example (from the 2026-07-21 core sync):

> Merges 102 upstream commits (`fa76115eb..bf25eae70`) from
> mod-playerbots/azerothcore-wotlk `Playerbot`, which includes two fresh
> acore/master merges.
>
> **Playerbot-relevant fixes**
> - Battleground invited-count accounting on BG entry (#26556) and
>   invited-count leak on failed BG teleports (#26422).
> - Pets start combat on contact instead of on attack command (#26463).
>
> **New module hooks**
> - `OnUnitExitCombat` (#25215) and `OnPlayerBeforeSendLoot` (#26364) —
>   both potentially useful for mod-llm event narration.
>
> **Content**
> - A large Ulduar rework batch (XT-002, Flame Leviathan, Freya, Mimiron,
>   Vezax, Yogg-Saron).
>
> **Config impact**
> - Drops `NpcEvadeIfTargetIsUnreachable` / `NpcRegenHPTimeIfTargetIsUnreachable`;
>   adds `Creature.Instance.TeleportToUnreachableTarget` and
>   `Wintergrasp.DeferShutdownTimer` (kept at defaults) — see the paired
>   felworld/configs PR.

Sections to add when applicable: **Conflicts** (file list + resolution
rationale — omit entirely when there were none), **Config impact** (omit
when no templates changed), and the sibling-PR links + merge-order warning
from step 4.
