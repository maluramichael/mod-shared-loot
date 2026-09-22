/*
 * mod-shared-loot
 *
 * Kanboard #799: "no round robin / need-greed / master loot - everyone in the group can loot
 * everything off a corpse, and taking an item doesn't remove it for the others."
 *
 * IMPORTANT - read this before trusting the module name: on this fork, true per-player
 * duplicated loot (every eligible group member gets his own copy of every item and the full
 * gold, taking a copy does not remove it for the others, no round robin / need-greed / master
 * loot rolls) is ALREADY a CORE feature: `Loot.Everyone.Enable` in worldserver.conf (default 1,
 * see src/server/apps/worldserver/worldserver.conf.dist), implemented in Loot::PrepareEveryoneLoot
 * / Loot::GiveEveryoneGold / Loot::GetLootSlotsFor (src/server/game/Loot/LootMgr.{h,cpp}). It
 * covers creature corpses and gameobject chests that use group-loot rules. This module does
 * NOT reimplement that (out of scope: "do NOT rewrite core LootMgr"), and it does NOT control
 * that core switch (it lives in worldserver.conf, not a module conf - out of this module's
 * folder). See README.md "Current capability vs. limitations" for the full honest breakdown.
 *
 * What this module actually adds, fully behind SharedLoot.Enable (default 0):
 *   - Forces a group's loot method to FREE_FOR_ALL (Group::SetLootMethod) so that ANY loot this
 *     server's Loot.Everyone core feature does NOT cover (loot generated while Loot.Everyone is
 *     off, or a group whose persisted DB loot method survived a restart before this module got a
 *     chance to touch it) still never shows a need/greed/round-robin/master-loot roll popup, and
 *     any member can simply take what's there. This is belt-and-suspenders, not the mechanism
 *     that duplicates loot.
 *   - Warns once at startup if SharedLoot.Enable=1 but the core Loot.Everyone.Enable is off, since
 *     in that case this module cannot deliver "everyone loots everything" by itself.
 *
 * Hooks used (read from src/server/game/Scripting/ScriptDefines/{GroupScript.h,PlayerScript.h}
 * and src/server/game/Groups/Group.cpp to confirm they actually fire on this fork):
 *   - GroupScript::OnCreate      - fires at the end of Group::Create(), AFTER the group's default
 *                                  m_lootMethod (GROUP_LOOT, or unchanged for LFG groups) is set
 *                                  in memory but AFTER the initial DB row was already written with
 *                                  that default. Our override only changes the in-memory method;
 *                                  see the "DB persistence" limitation below.
 *   - GroupScript::OnAddMember   - fires at the end of Group::AddMember(), covers every runtime
 *                                  join (invite accept, bot auto-join, etc.), not just group
 *                                  creation.
 *   - PlayerScript::OnPlayerLogin - fires on every login; re-applies FFA to the player's current
 *                                  group. This is the fallback that catches a group that was
 *                                  loaded straight from the DB at world boot (Group::LoadGroupFromDB
 *                                  does not go through Create()/AddMember() and therefore never
 *                                  calls OnCreate/OnAddMember), so its persisted loot method would
 *                                  otherwise stick until the group is next created/joined.
 *
 * LootScript (LootScript.h) only exposes OnLootMoney (informational, fires after the money is
 * already awarded) - nothing there can change loot distribution, so it is not used here.
 *
 * Released under GNU GPL v2; redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "Config.h"
#include "Group.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"

#include "SharedLoot.h"

namespace SharedLoot
{
    Config& GetConfig()
    {
        static Config cfg;
        return cfg;
    }
}

using SharedLoot::GetConfig;

namespace
{
    // Force FREE_FOR_ALL on a group, honoring IncludeRaid. Safe to call repeatedly - it is a
    // no-op once the method already matches.
    void ForceFreeForAll(Group* group)
    {
        if (!group)
            return;

        SharedLoot::Config const& cfg = GetConfig();
        if (!cfg.Enable || !cfg.ForceFreeForAll)
            return;

        if (!cfg.IncludeRaid && group->isRaidGroup())
            return;

        if (group->GetLootMethod() != FREE_FOR_ALL)
            group->SetLootMethod(FREE_FOR_ALL);
    }
}

// =====================================================================
//  GroupScript: force FFA on group creation and on every member join.
// =====================================================================
class SharedLootGroupScript : public GroupScript
{
public:
    SharedLootGroupScript() : GroupScript("SharedLoot_GroupScript",
        {
            GROUPHOOK_ON_CREATE,
            GROUPHOOK_ON_ADD_MEMBER
        })
    {
    }

    void OnCreate(Group* group, Player* /*leader*/) override
    {
        ForceFreeForAll(group);
    }

    void OnAddMember(Group* group, ObjectGuid /*guid*/) override
    {
        ForceFreeForAll(group);
    }
};

// =====================================================================
//  PlayerScript: re-apply on login (catches groups restored from the DB,
//  which never go through Group::Create()/AddMember()).
// =====================================================================
class SharedLootPlayerScript : public PlayerScript
{
public:
    SharedLootPlayerScript() : PlayerScript("SharedLoot_PlayerScript") { }

    void OnPlayerLogin(Player* player) override
    {
        if (!GetConfig().Enable || !player)
            return;

        ForceFreeForAll(player->GetGroup());
    }
};

// =====================================================================
//  WorldScript: config load + one-time sanity warning.
// =====================================================================
class SharedLootWorldScript : public WorldScript
{
public:
    SharedLootWorldScript() : WorldScript("SharedLoot_WorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        SharedLoot::Config& cfg = GetConfig();
        cfg.Enable          = sConfigMgr->GetOption<bool>("SharedLoot.Enable", false);
        cfg.ForceFreeForAll = sConfigMgr->GetOption<bool>("SharedLoot.ForceFreeForAll", true);
        cfg.IncludeRaid     = sConfigMgr->GetOption<bool>("SharedLoot.IncludeRaid", true);
    }

    void OnStartup() override
    {
        if (!GetConfig().Enable)
            return;

        // Same worldserver.conf key the core LootMgr reads via CONFIG_LOOT_EVERYONE. Reading it
        // through sConfigMgr here is diagnostic only - this module never writes it and never
        // touches core files.
        bool const coreLootEveryone = sConfigMgr->GetOption<bool>("Loot.Everyone.Enable", true);
        if (!coreLootEveryone)
        {
            LOG_WARN("server.loading",
                "mod-shared-loot: SharedLoot.Enable=1 but the core 'Loot.Everyone.Enable' option is "
                "OFF. Without it, this module only forces Free-For-All group loot (no more rolls) - "
                "it does NOT duplicate loot per player and does NOT hand out full gold to everyone. "
                "Set Loot.Everyone.Enable = 1 in worldserver.conf to get the ticket's actual outcome.");
        }
    }
};

// =====================================================================
//  Registration
// =====================================================================
void AddSharedLootScripts()
{
    new SharedLootGroupScript();
    new SharedLootPlayerScript();
    new SharedLootWorldScript();
}
