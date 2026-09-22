/*
 * mod-shared-loot loader.
 *
 * The playerbots fork auto-globs every module's sources into one lib and looks
 * up a loader symbol derived from the folder name: for folder "mod-shared-loot"
 * that symbol is exactly "Addmod_shared_lootScripts". It must exist and call our
 * real registration function.
 *
 * Released under GNU GPL v2 or (at your option) any later version.
 */

void AddSharedLootScripts();

void Addmod_shared_lootScripts()
{
    AddSharedLootScripts();
}
