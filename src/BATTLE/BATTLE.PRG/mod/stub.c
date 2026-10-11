/* Loader for the mod file (ENDING/CALLO.BIN), living in INITBTL.PRG's spare tail.
 *
 * INITBTL.PRG runs once whenever battle mode starts (new game, loaded save), before any
 * room is loaded. Its synchronous SYSTEM.DAT load is patched to call this instead, so the
 * mod is read at a moment when the drive is known to be free, into RAM just above
 * BATTLE.PRG's loaded image. The room-spawn hook in BATTLE.PRG then calls the mod directly.
 *
 * Not in BATTLE.PRG's spare tail: other mods use it (Zenith keeps its variables right after
 * its code there, which wiped an earlier stub and crashed the game).
 *
 * Addresses, disc position and size come from build/mod/mod_config.h, written by
 * tools/mod/build_mod.sh.
 */
#include "common.h"
#include "build/mod/mod_config.h"

int vs_main_diskLoadFile(int sector, int bytes, void* vram);
void FlushCache(void);

/* Replaces INITBTL's vs_main_diskLoadFile(SYSTEM.DAT) call; same signature and result */
int vs_mod_loadSystemDat(int sector, int bytes, void* buf)
{
    int result = vs_main_diskLoadFile(sector, bytes, buf);

    vs_main_diskLoadFile(MOD_LBA, MOD_SIZE, (void*)MOD_BASE);
    FlushCache();
    return result;
}
