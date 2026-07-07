#!/usr/bin/env python3
"""Regenerate a deployed .conf as a clean parallel of its (new) .conf.dist template.

We keep the env/dist/etc config tree as parallel pairs: each ``.conf.dist`` is a
verbatim copy of the upstream template, and each ``.conf`` is that same template
with our value-overrides re-applied -- structurally identical except for the values
we deviate on, so ``diff foo.conf foo.conf.dist`` shows exactly our deltas.

On an upstream bump the template reflows (new keys, moved keys, whitespace), so the
``.conf`` must be regenerated to stay parallel. This is a 3-way merge:

    base   = OLD .conf.dist   (the template we were tracking)
    ours   = OLD .conf        (base + our value-overrides)
    theirs = NEW .conf.dist   (the freshly-copied upstream template)
    result = NEW .conf        (theirs with our overrides re-applied)

Override set = keys whose value in OLD .conf differs from OLD .conf.dist (same
version, so a pure value diff). New keys in the template keep their default unless
you later choose to override them.

Typical use (old versions come from git; new .conf.dist is already copied in):

    git show HEAD:worldserver.conf      > /tmp/old.conf
    git show HEAD:worldserver.conf.dist > /tmp/old.dist
    cp /path/to/src/.../worldserver.conf.dist worldserver.conf.dist   # new template
    tools/regen-config.py /tmp/old.conf /tmp/old.dist worldserver.conf.dist worldserver.conf

Then verify the pair is clean: identical line counts and a value-only diff.
"""
import argparse
import re
import sys

# Matches an active "Key.Sub = value" directive (not a '#'-comment), capturing
# indent / key / separator / value / trailing-space separately so we can swap only
# the value while preserving the template's exact formatting.
LINE_RE = re.compile(r'^(\s*)([A-Za-z][\w.]*)(\s*=\s*)(.*?)(\s*)$')


def parse_values(path):
    """Return {key: value} for every active directive line in *path*."""
    values = {}
    with open(path) as handle:
        for line in handle:
            match = LINE_RE.match(line.rstrip('\n'))
            if match:
                values[match.group(2)] = match.group(4)
    return values


def regen(old_conf, old_dist, new_dist, out_path):
    ours = parse_values(old_conf)
    base = parse_values(old_dist)
    overrides = {key: val for key, val in ours.items() if key in base and val != base[key]}
    conf_only = [key for key in ours if key not in base]

    applied = set()
    out_lines = []
    with open(new_dist) as handle:
        for line in handle.readlines():
            match = LINE_RE.match(line.rstrip('\n'))
            if match and match.group(2) in overrides:
                key = match.group(2)
                newline = '\n' if line.endswith('\n') else ''
                line = f"{match.group(1)}{key}{match.group(3)}{overrides[key]}{newline}"
                applied.add(key)
            out_lines.append(line)

    with open(out_path, 'w') as handle:
        handle.writelines(out_lines)

    dropped = [key for key in overrides if key not in applied]
    print(f"{out_path}: {len(applied)}/{len(overrides)} overrides re-applied")
    if conf_only:
        print(f"  !! keys in old .conf but NOT in old .conf.dist (investigate): {conf_only}",
              file=sys.stderr)
    if dropped:
        print(f"  !! overrides DROPPED because the key is gone from the new template:",
              file=sys.stderr)
        for key in dropped:
            print(f"       {key} = {overrides[key]}", file=sys.stderr)
    return 1 if (conf_only or dropped) else 0


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('old_conf', help='OLD .conf (base + our overrides)')
    parser.add_argument('old_dist', help='OLD .conf.dist (the template we tracked)')
    parser.add_argument('new_dist', help='NEW .conf.dist (freshly-copied upstream template)')
    parser.add_argument('out', help='path to write the regenerated .conf')
    args = parser.parse_args()
    return regen(args.old_conf, args.old_dist, args.new_dist, args.out)


if __name__ == '__main__':
    sys.exit(main())
