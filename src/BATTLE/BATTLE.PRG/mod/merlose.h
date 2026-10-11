/* The resident part of Callo's menus, other per-frame work and the HUD. Included at the end of
 * callo.c. The menu code itself is in the menu module (menu.c, see callo.h); the redirected
 * MENU3/MENU0 calls land here and call into it. */

u_char _menuMode; /* callo.h */
u_char _respawnPending; /* respawn her once the menu has closed (see below) */

vs_battle_actor* _calloActor(void)
{
    vs_battle_actor* actor;
    int i;

    if (_calloSlot == 0) {
        return NULL;
    }
    actor = vs_battle_actors[_calloSlot];
    if ((actor == NULL) || (actor->unk3C == NULL)) {
        return NULL;
    }
    for (i = 0; i < 8; ++i) {
        if (actor->unk3C->name[i] != _calloInitData.name[i]) {
            return NULL;
        }
    }
    return actor;
}

/* Gear grows in battle (classes, affinities, DP/PP) on her in-battle copy of it; Ashley's is
 * copied back to the inventory when the menu opens (vs_mainMenu_initInventory). Do the same
 * for her, but only where the inventory slot still holds that item per her loadout: her items
 * can be dropped or disassembled while she wears them, and the slot reused. */
void _saveCalloGear(vs_battle_actor* callo)
{
    u_char* loadout = LOADOUT;
    vs_battle_actor2* stats = callo->unk3C;
    vs_main_inventoryWeapon* weapon;
    int slot;
    int i;

    if (loadout[loadoutVersion] != LOADOUT_VERSION) {
        return;
    }
    slot = stats->weapon.index;
    if ((slot != 0) && (slot <= 8) && (loadout[loadoutWeapon] == slot)
        && (stats->weapon.blade.id == loadout[loadoutWeapon + 1])) {
        weapon = &vs_main_inventory.weapons[slot - 1];
        if ((weapon->blade != 0)
            && (vs_main_inventory.blades[weapon->blade - 1].id == stats->weapon.blade.id)) {
            vs_battle_copyUiWeaponStats(weapon, &stats->weapon);
        }
    }
    slot = stats->shield.index;
    if ((slot != 0) && (slot <= 8) && (loadout[loadoutShield] == slot)
        && (vs_main_inventory.shields[slot - 1].base.id == stats->shield.base.id)) {
        vs_battle_copyUiShieldStats(&vs_main_inventory.shields[slot - 1], &stats->shield);
    }
    for (i = 0; i < 6; ++i) {
        slot = stats->limbs[i].armor.index;
        if ((slot != 0) && (slot <= 16) && (loadout[loadoutArmor + i * 2] == slot)
            && (vs_main_inventory.armor[slot - 1].id == stats->limbs[i].armor.armor.id)) {
            vs_battle_copyUiArmorStats(&vs_main_inventory.armor[slot - 1], &stats->limbs[i].armor);
        }
    }
    slot = stats->accessory.index;
    if ((slot != 0) && (slot <= 16) && (loadout[loadoutAccessory] == slot)
        && (vs_main_inventory.armor[slot - 1].id == stats->accessory.accessory.id)) {
        vs_battle_copyUiAccessoryStats(&vs_main_inventory.armor[slot - 1], &stats->accessory);
    }
}

/* --- Menu module --------------------------------------------------------------------------------
 * Loaded on demand into the gap above the menu overlays (callo.h). */
int vs_main_diskLoadFile(int sector, int bytes, void* buf);
void FlushCache(void);

static int _menuModuleLoaded(void)
{
    u_int* module = MENU_MODULE;
    u_int sum = 0;
    int i;

    if ((module[0] != MENU_MODULE_MAGIC) || (module[1] > MENU_MODULE_LOAD_SIZE)) {
        return 0;
    }
    for (i = 8; i < module[1] / 4; ++i) {
        sum += module[i];
    }
    return sum == module[7];
}

/* Synchronous; refused if the drive is busy (then it's tried again next time) */
static int _loadMenuModule(void)
{
    if (_menuModuleLoaded()) {
        return 1;
    }
    if (!vs_main_diskLoadFile(MENU_MODULE_LBA, MENU_MODULE_LOAD_SIZE, MENU_MODULE)) {
        return 0;
    }
    FlushCache();
    return _menuModuleLoaded();
}

/* --- MENU3.PRG call sites ---------------------------------------------------------------------
 * MENU3 can't be rebuilt (it would outgrow its sectors), so some of its calls are redirected
 * here by tools/mod/build_mod.py menu3. Each wrapper calls the original function it replaced. */

/* D_800F4EE8.unk3A.currentItemCategory (5BF94.h clashes with callo.c's declarations) */
extern u_char D_800F4EE8[];
#define CURRENT_ITEM_CATEGORY D_800F4EE8[0x3A]

#define MENU3_ITEM_NAVIGATION 0x801053EC  /* _itemNavigation */
#define MENU3_EQUIP_MENU 0x80108088       /* _equipMenu */
#define MAINMENU_SET_MENU_ROWS 0x801005E0 /* vs_mainmenu_setMenuRows */

static int _inOptions(void)
{
    return ((CURRENT_ITEM_CATEGORY - 1) & 7) == 7;
}

/* _itemNavigation's ITEMS > OPTIONS rows: add MERLOSE after EQUIP and SETUP (greyed out
 * when she isn't in the room) */
void vs_mod_setMenuRows(int rowCount, int rowInfo, char** strings, int* rowTypes)
{
    if (_inOptions() && (rowCount == 2)) {
        _loadMenuModule(); /* ready for EQUIP / MERLOSE */
        strings[4] = _merloseRow;
        if (_calloActor() != NULL) {
            strings[5] = _merloseRowDesc;
            rowTypes[2] = rowTypes[0];
        } else {
            strings[5] = _merloseRowAbsent;
            rowTypes[2] = 1;
        }
        rowCount = 3;
    }
    ((void (*)(int, int, char**, int*))MAINMENU_SET_MENU_ROWS)(
        rowCount, rowInfo, strings, rowTypes);
}

/* vs_menu3_exec's `row = _itemNavigation(0)`: MERLOSE opens EQUIP for Callo */
int vs_mod_itemNavigation(int initialize)
{
    vs_battle_actor* callo;
    int row = ((int (*)(int))MENU3_ITEM_NAVIGATION)(initialize);

    if (_inOptions()) {
        if (row == 1) {
            if (_loadMenuModule()) {
                ((void (*)(vs_battle_actor*))MENU_ENTRY(menuEquipEnter))(NULL);
            }
        } else if (row == 3) {
            callo = _calloActor();
            if ((callo == NULL) || !_loadMenuModule()) {
                return 0;
            }
            ((void (*)(vs_battle_actor*))MENU_ENTRY(menuEquipEnter))(callo);
            row = 1;
        }
    }
    return row;
}

/* vs_menu3_exec's `row = _equipMenu(0)`: non-zero means EQUIP was left */
int vs_mod_equipMenu(int initialize)
{
    int row = ((int (*)(int))MENU3_EQUIP_MENU)(initialize);

    if ((row != 0) && (_menuMode != 0) && _menuModuleLoaded()) {
        ((void (*)(void))MENU_ENTRY(menuEquipLeave))();
    }
    return row;
}

/* The weapon and shield screens' model loads, each frame until they return 0: in MERLOSE, her
 * equip sequence (menu module) */
int vs_mod_loadObjectData(int category, int wepId, int material)
{
    vs_battle_actor* callo;

    if (_menuMode != 2) {
        return ((int (*)(int, int, int))0x801090C4)(category, wepId, material); /* _loadObjectData */
    }
    callo = _calloActor();
    if ((callo == NULL) || (MENU_MODULE[0] != MENU_MODULE_MAGIC)) {
        return 0;
    }
    return ((int (*)(vs_battle_actor*, int, int, int))MENU_ENTRY(menuEquipCallo))(
        callo, category, wepId, material);
}

/* --- Respawn after MERLOSE ----------------------------------------------------------------------
 * Changing her gear in place loads the new models but doesn't attach them or switch her weapon
 * type's animations; spawning does all of that. So once the menu has closed she is respawned
 * next to Ashley (also a way to free her if she gets stuck), keeping her HP and MP.
 * vs_battle_exec's per-frame call to func_80088B6C (upkeep, normal gameplay only) is
 * redirected here by tools/mod/build_mod.py battle. */

int func_80088554(void);
void func_8007D340(int); /* _removeActorAtIndex(index, 0) */
void func_8007BF6C(int); /* clear an actor's skip flag (shown and updated) */

/* Spawns start skipped (hidden, not updated); a room load un-skips everyone once loaded
 * (func_8007BFC0). After a respawn mid-room, un-skip her once her model has loaded. */
static u_char _unskipPending;

static void _respawnCallo(void)
{
    vs_battle_actor* callo = _calloActor();
    vs_battle_actor2* stats;
    int hp;
    int mp;

    if (callo == NULL) {
        return;
    }
    hp = callo->unk3C->currentHP;
    mp = callo->unk3C->currentMP;
    if (hp == 0) {
        return; /* defeated: leave her be */
    }
    _saveCalloGear(callo);
    func_8007D340(_calloSlot);
    vs_mod_spawnCallo();
    callo = _calloActor();
    if (callo != NULL) {
        _unskipPending = 1;
        stats = callo->unk3C;
        stats->currentHP = (hp < stats->maxHP) ? hp : stats->maxHP;
        stats->currentMP = (mp < stats->maxMP) ? mp : stats->maxMP;
    }
}

/* Enemies that appear mid-room (summoned, script spawns) get their attitude table after she
 * spawned: repeat _makeEnemiesHostile every so often so they treat her like Ashley too. */
#define HOSTILITY_INTERVAL 15 /* frames */

static u_char _hostilityTimer;

/* Leash: if she has been far from Ashley for a while (left behind by a cutscene, stuck on
 * scenery), respawn her beside him. Distance in tiles, the larger of the x and z gaps. */
#define LEASH_TILES 10
#define LEASH_FRAMES 90 /* about 3 seconds */

static u_char _leashFrames;

static int _farFromAshley(void)
{
    D_800F4538_t* ashley = D_800F4538[0];
    D_800F4538_t* callo = D_800F4538[_calloSlot];
    int dx;
    int dz;

    if ((ashley == NULL) || (callo == NULL) || callo->unk0.skip) {
        return 0;
    }
    dx = ashley->unk0.currentTileX - callo->unk0.currentTileX;
    dz = ashley->unk0.currentTileZ - callo->unk0.currentTileZ;
    if (dx < 0) {
        dx = -dx;
    }
    if (dz < 0) {
        dz = -dz;
    }
    return (dx > LEASH_TILES) || (dz > LEASH_TILES);
}

/* Her gear's growth is copied to the inventory every so often: a room change removes her
 * without notice. */
#define GEAR_SAVE_INTERVAL 60 /* frames */

static u_char _gearSaveTimer;

int vs_mod_frame(void)
{
    int result = func_80088554();
    vs_battle_actor* callo;

    if (++_gearSaveTimer >= GEAR_SAVE_INTERVAL) {
        _gearSaveTimer = 0;
        callo = _calloActor();
        if (callo != NULL) {
            _saveCalloGear(callo);
        }
    }

    if (++_hostilityTimer >= HOSTILITY_INTERVAL) {
        _hostilityTimer = 0;
        if (_calloActor() != NULL) {
            _makeEnemiesHostile(_calloSlot);
        }
    }
    if ((_calloActor() != NULL) && _farFromAshley()) {
        if (++_leashFrames >= LEASH_FRAMES) {
            _leashFrames = 0;
            _respawnPending = 1;
        }
    } else {
        _leashFrames = 0;
    }

    if (_unskipPending && (D_800F4538[_calloSlot] != NULL)) {
        _unskipPending = 0;
        func_8007BF6C(_calloSlot);
    }
    if (_respawnPending) {
        _respawnPending = 0;
        _respawnCallo();
    }
    return result;
}

/* Room scripts count the enemy team (4) to tell whether a room is cleared (func_8007CF80, from
 * a script op and the event-condition check). She is on that team, so leave her out of it:
 * both calls are redirected here by tools/mod/build_mod.py battle. */
int func_8007CF80(int);

int vs_mod_countTeam(int team)
{
    int count = func_8007CF80(team);
    vs_battle_actor* callo = _calloActor();

    if ((callo != NULL) && (callo->unk1C == team) && (count > 0)) {
        --count;
    }
    return count;
}

/* --- Ashley's targeting ---------------------------------------------------------------------------
 * The player's target lists (the attack sphere, area of effect, spells and items) take every
 * character whose status says "targetable" and whose team the action allows - and nearly all
 * actions allow every team, which is why he could hit her. Each list's status call is
 * redirected here (build_mod.py battle): for Ashley's actions she's only a target of support
 * ones (action target kind 1: Heal, Prostasia, Herakles, the elemental guards, items...), so
 * his weapon, Break Arts and attack spells can't pick her or her body parts. */
extern u_char* D_800F19CC; /* the action being set up: action id (u16) at 8, actor at 12 */
#define SUPPORT_ACTION 1    /* ACTION_TARGET_KIND */
#define TARGETABLE 1        /* func_800A0BE0 status bit */

u_int vs_mod_targetStatus(int actor)
{
    u_int status = func_800A0BE0(actor);

    if ((actor == _calloSlot) && (D_800F19CC[12] == 0) && (_calloActor() != NULL)
        && (ACTION_TARGET_KIND(*(u_short*)(D_800F19CC + 8)) != SUPPORT_ACTION)) {
        status &= ~TARGETABLE;
    }
    return status;
}

/* --- Her HP/MP/RISK display --------------------------------------------------------------------
 * A smaller copy of Ashley's (func_800CA2DC, top left): half-width bars under his, drawn the
 * same way (vs_battle_renderStatBar), sliding out with his (D_800EB9B0). vs_battle_exec's
 * call to his display is redirected here by tools/mod/build_mod.py battle. */
#include "gpu.h"

void func_800CA2DC(void);
extern int D_800EB9B0;
extern int D_800EBC04[]; /* bar colours: HP, MP, RISK */

#define MINI_BAR_WIDTH 32
#define MINI_HUD_X 10
#define MINI_HUD_Y 32
#define MINI_HUD_ROW 6

static void _renderMiniBar(int colorIndex, int current, int max, int xy)
{
    u_long* before = (u_long*)((u_char*)vs_scratch.unk4 - 0xC);
    u_long* prim = vs_scratch.unk0;
    u_int rgb1 = D_800EBC04[colorIndex];
    int w;
    int s;
    int rgb0;

    if (max <= 0) {
        return;
    }
    if (current < 0) {
        current = 0;
    }
    if (current > max) {
        current = max;
    }
    w = (current * MINI_BAR_WIDTH + max - 1) / max;
    s = w * (0x40 / MINI_BAR_WIDTH); /* colour blend on the full bar's 0-64 scale */
    rgb0 = (((s * 0xFF) + ((rgb1 & 0xFF) * (0x40 - s))) >> 6)
         | ((((s * 0xF0) + (((rgb1 >> 8) & 0xFF) * (0x40 - s))) >> 6) << 8)
         | ((((s * 0x9E) + ((rgb1 >> 16) * (0x40 - s))) >> 6) << 16);

    prim[0] = (*before & 0xFFFFFF) | (w == 0 ? 0x03000000 : 0x0D000000);
    prim[1] = vs_getRGB0(primTile, 0x00, 0x28, 0x40);
    prim[2] = xy;
    prim[3] = vs_getXY(MINI_BAR_WIDTH + 2, 4);
    prim[4] = vs_getTpage(0, 0, clut4Bit, semiTransparencyHalf, ditheringOn);
    prim[5] = rgb0 | (primPolyG4 << 24);
    xy += vs_getXY(1, 1);
    prim[6] = xy;
    prim[7] = rgb1;
    prim[8] = xy + w;
    xy += vs_getXY(0, 2);
    prim[9] = rgb0;
    prim[10] = xy;
    prim[11] = rgb1;
    prim[12] = xy + w;
    prim[13] = vs_getTpage(0, 0, 0, 0, 0);
    *before = ((u_long)prim << 8) >> 8;
    vs_scratch.unk0 = prim + 14;
}

void vs_mod_hud(void)
{
    vs_battle_actor* callo;
    vs_battle_actor2* stats;
    int xy;

    func_800CA2DC();
    callo = _calloActor();
    if ((callo == NULL) || (D_800F4538[_calloSlot] == NULL) || D_800F4538[_calloSlot]->unk0.skip) {
        return;
    }
    stats = callo->unk3C;
    xy = vs_getXY(MINI_HUD_X, MINI_HUD_Y) - D_800EB9B0;
    _renderMiniBar(0, stats->currentHP, stats->maxHP, xy);
    _renderMiniBar(1, stats->currentMP, stats->maxMP, xy + vs_getXY(0, MINI_HUD_ROW));
    if (stats->risk != 0) {
        _renderMiniBar(2, stats->risk, 100, xy + vs_getXY(0, MINI_HUD_ROW * 2));
    }
}

/* --- MAGIC menu (MENU0.PRG): choosing her spells ------------------------------------------------
 * Each school list's vs_mainmenu_setMenuRows call and its per-frame _setMPCost(highlighted
 * spell) call are redirected here by tools/mod/build_mod.py menu0; the work is in the module. */
#include <libetc.h>

#define FIRST_MAGIC_LIST 7 /* list ids: warlock, shaman, sorcerer, enchanter */

void vs_mod_magicRows(int rowCount, int rowInfo, char** strings, int* rowTypes)
{
    int school = (rowInfo & 0xFF) - FIRST_MAGIC_LIST;

    if ((school >= 0) && (school < 4) && _loadMenuModule()) {
        ((void (*)(int, int*))MENU_ENTRY(menuMarkSpells))(school, rowTypes);
    }
    ((void (*)(int, int, char**, int*))MAINMENU_SET_MENU_ROWS)(
        rowCount, rowInfo, strings, rowTypes);
}

void vs_mod_magicCost(int id)
{
    ((void (*)(int))0x80102884)(id); /* MENU0's _setMPCost */
    if ((vs_main_buttonsPressed.all & PADRleft) && (MENU_MODULE[0] == MENU_MODULE_MAGIC)) {
        ((void (*)(int))MENU_ENTRY(menuToggleSpell))(id);
    }
}
