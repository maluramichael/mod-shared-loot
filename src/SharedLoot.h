/*
 * mod-shared-loot - shared declarations.
 *
 * Kanboard #799.
 *
 * Released under GNU GPL v2; redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef MOD_SHARED_LOOT_H
#define MOD_SHARED_LOOT_H

#include <cstdint>

namespace SharedLoot
{
    // Cached config (populated in WorldScript::OnAfterConfigLoad).
    struct Config
    {
        bool Enable          = false; // module master switch (DEFAULT OFF - see README)
        bool ForceFreeForAll = true;  // force Group::SetLootMethod(FREE_FOR_ALL) on group create/join/login
        bool IncludeRaid     = true;  // also force it for raid groups (false = party groups only)
    };

    Config& GetConfig();
}

#endif // MOD_SHARED_LOOT_H
