# mod-shared-loot

An [AzerothCore](https://www.azerothcore.org/) module (WotLK 3.3.5a) that removes loot
rolls from group play.

## What it does

While enabled, the module forces every group (and optionally raid) to the **Free-For-All**
loot method: no need/greed prompts, no round-robin, no master looter — any member can pick
up whatever is on a corpse or chest.

It pairs with AzerothCore's built-in "everyone loot" behaviour (`Loot.Everyone.Enable`),
which gives each eligible member their own copy of the loot and full gold. When that core
option is enabled, the two together deliver true shared loot; when it is off, this module
still removes the roll pop-ups.

## Scope and limitations

- Personal-source loot (skinning, pickpocketing, gathering, fishing, and chests without
  group loot rules) stays single-recipient by design.
- The module never rewrites core loot distribution; it only sets the group's loot method.

## Configuration

`conf/mod_shared_loot.conf.dist`:

| Key                          | Default | Description                                  |
|------------------------------|---------|----------------------------------------------|
| `SharedLoot.Enable`          | `0`     | Master on/off switch (opt-in)                |
| `SharedLoot.ForceFreeForAll` | `1`     | Force groups to Free-For-All loot            |
| `SharedLoot.IncludeRaid`     | `1`     | Also apply to raid groups                    |

## Installation

Clone into your AzerothCore `modules/` directory and rebuild the worldserver.

## License

Released under the GNU GPL v2 (or later).
