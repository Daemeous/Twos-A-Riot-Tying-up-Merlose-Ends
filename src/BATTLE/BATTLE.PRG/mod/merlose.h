/* MERLOSE equip menu. Included at the end of callo.c; called from MENU3.PRG through
 * redirected calls (see the wrappers at the end and tools/mod/build_mod.py menu3).
 *
 * The game's own EQUIP screens are reused. They only list items whose inventory slot appears in
 * vs_main_inventoryIndices, so while they are open the other character's items are removed from
 * those lists (and restored on leaving): items can only be on one character at a time.
 *
 * For MERLOSE, Ashley's actor is pointed at Callo's stats (unk3C) while the screen is open, so
 * the screens show and equip her real stats and gear, and the preview shows her model (as the
 * status screen's L1 does). Ashley's "equipped" markers are swapped for her loadout on entry; on
 * leaving, her new loadout is read from the markers and everything of Ashley's is put back. */

static u_char _menuMode; /* 0 closed, 1 Ashley's EQUIP, 2 MERLOSE */
static u_char _savedWeapons[8];
static u_char _savedShields[8];
static u_char _savedArmor[16];
static char _ashleyWeaponEquipped[8];
static char _ashleyShieldEquipped[8];
static u_char _ashleyArmorLimb[16];
static vs_battle_actor2* _ashleyStats = NULL; /* initialised: see _calloSlot in callo.c */
static u_char _respawnPending; /* respawn her once the menu has closed (end of file) */
static u_char _ashleyWeaponCategory;

void func_800F9A24(int); /* MAINMENU: build the menu preview from a character */
void func_800F9E0C(void); /* MAINMENU: free the menu preview */

/* Callo's actor if she is in the room */
static vs_battle_actor* _calloActor(void)
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

/* Show her in the preview: a copy of her in-game model, weapon models included */
static void _previewCallo(void)
{
    func_800F9E0C();
    func_800F9A24(_calloSlot);
}

static void _hideSlot(u_char* list, int count, int slot)
{
    int i;

    for (i = 0; i < count; ++i) {
        if (list[i] == slot) {
            list[i] = 0;
        }
    }
}

/* Mark a loadout armour piece as worn on body part `limb` (1-6) or as the accessory (7) */
static void _markLoadoutArmor(int offset, int limb)
{
    u_char* loadout = LOADOUT;
    int slot = loadout[offset];

    if ((slot != 0) && (slot <= 16)
        && (vs_main_inventory.armor[slot - 1].id == loadout[offset + 1])) {
        vs_main_inventory.armor[slot - 1].limb = limb;
    }
}

static void _equipEnter(vs_battle_actor* callo)
{
    u_char* loadout = LOADOUT;
    vs_main_inventoryWeapon* weapon;
    vs_main_inventoryShield* shield;
    vs_main_inventoryBlade* blade;
    int i;

    _dropSharedItems();

    for (i = 0; i < 8; ++i) {
        _savedWeapons[i] = vs_main_inventoryIndices.weapons[i];
        _savedShields[i] = vs_main_inventoryIndices.shields[i];
    }
    for (i = 0; i < 16; ++i) {
        _savedArmor[i] = vs_main_inventoryIndices.armor[i];
    }

    if (callo == NULL) {
        /* Ashley's EQUIP: hide Callo's items */
        _menuMode = 1;
        if (loadout[loadoutVersion] != LOADOUT_VERSION) {
            return;
        }
        _hideSlot(vs_main_inventoryIndices.weapons, 8, loadout[loadoutWeapon]);
        _hideSlot(vs_main_inventoryIndices.shields, 8, loadout[loadoutShield]);
        for (i = 0; i < 6; ++i) {
            _hideSlot(vs_main_inventoryIndices.armor, 16, loadout[loadoutArmor + i * 2]);
        }
        _hideSlot(vs_main_inventoryIndices.armor, 16, loadout[loadoutAccessory]);
        return;
    }

    _menuMode = 2;

    /* Swap Ashley's markers out, hiding his items and weapons she has no animations for */
    for (i = 0; i < 8; ++i) {
        weapon = &vs_main_inventory.weapons[i];
        _ashleyWeaponEquipped[i] = weapon->isEquipped;
        _ashleyShieldEquipped[i] = vs_main_inventory.shields[i].isEquipped;
        if (_ashleyWeaponEquipped[i]) {
            _hideSlot(vs_main_inventoryIndices.weapons, 8, i + 1);
        } else if (weapon->blade != 0) {
            blade = &vs_main_inventory.blades[weapon->blade - 1];
            if (!(CALLO_WEAPON_CATEGORIES & (1 << blade->category))) {
                _hideSlot(vs_main_inventoryIndices.weapons, 8, i + 1);
            }
        }
        if (_ashleyShieldEquipped[i]) {
            _hideSlot(vs_main_inventoryIndices.shields, 8, i + 1);
        }
        weapon->isEquipped = 0;
        vs_main_inventory.shields[i].isEquipped = 0;
    }
    for (i = 0; i < 16; ++i) {
        _ashleyArmorLimb[i] = vs_main_inventory.armor[i].limb;
        if (_ashleyArmorLimb[i]) {
            _hideSlot(vs_main_inventoryIndices.armor, 16, i + 1);
        }
        vs_main_inventory.armor[i].limb = 0;
    }

    /* ...and Callo's in */
    weapon = _loadoutWeapon();
    if (weapon != NULL) {
        weapon->isEquipped = 1;
    }
    shield = _loadoutShield();
    if (shield != NULL) {
        shield->isEquipped = 1;
    }
    if (loadout[loadoutVersion] == LOADOUT_VERSION) {
        for (i = 0; i < 6; ++i) {
            _markLoadoutArmor(loadoutArmor + i * 2, i + 1);
        }
        _markLoadoutArmor(loadoutAccessory, 7);
    }

    /* The EQUIP screens work on vs_battle_characterState: give it her stats */
    _ashleyStats = vs_battle_characterState->unk3C;
    _ashleyWeaponCategory = vs_battle_characterState->equippedWeaponCategory;
    vs_battle_characterState->unk3C = callo->unk3C;
    _previewCallo();
}

static void _equipLeave(void)
{
    u_char* loadout = LOADOUT;
    vs_battle_actor2* stats;
    int i;
    int part;

    if (_menuMode == 2) {
        stats = vs_battle_characterState->unk3C;
        for (i = 0; i <= loadoutAccessory + 1; ++i) {
            loadout[i] = 0;
        }
        loadout[loadoutVersion] = LOADOUT_VERSION;

        /* Read her new loadout from the markers. Nothing marked: her default weapon (the
         * bow) and armour, unless she wore none of it any more (taken off here). */
        for (i = 0; i < 8; ++i) {
            if (vs_main_inventory.weapons[i].isEquipped) {
                loadout[loadoutWeapon] = i + 1;
                loadout[loadoutWeapon + 1] =
                    vs_main_inventory.blades[vs_main_inventory.weapons[i].blade - 1].id;
            }
            if (vs_main_inventory.shields[i].isEquipped) {
                loadout[loadoutShield] = i + 1;
                loadout[loadoutShield + 1] = vs_main_inventory.shields[i].base.id;
            }
        }
        for (part = 0; part < 6; ++part) {
            if (stats->limbs[part].armor.armor.id == 0) {
                loadout[loadoutArmor + part * 2] = LOADOUT_EMPTY;
            }
        }
        if (stats->accessory.accessory.id == 0) {
            loadout[loadoutAccessory] = LOADOUT_EMPTY;
        }
        for (i = 0; i < 16; ++i) {
            part = vs_main_inventory.armor[i].limb;
            if ((part >= 1) && (part <= 6)) {
                loadout[loadoutArmor + (part - 1) * 2] = i + 1;
                loadout[loadoutArmor + (part - 1) * 2 + 1] = vs_main_inventory.armor[i].id;
            } else if (part == 7) {
                loadout[loadoutAccessory] = i + 1;
                loadout[loadoutAccessory + 1] = vs_main_inventory.armor[i].id;
            }
        }

        /* Give Ashley his markers, stats and preview back */
        for (i = 0; i < 8; ++i) {
            vs_main_inventory.weapons[i].isEquipped = _ashleyWeaponEquipped[i];
            vs_main_inventory.shields[i].isEquipped = _ashleyShieldEquipped[i];
        }
        for (i = 0; i < 16; ++i) {
            vs_main_inventory.armor[i].limb = _ashleyArmorLimb[i];
        }
        vs_battle_characterState->unk3C = _ashleyStats;
        vs_battle_characterState->equippedWeaponCategory = _ashleyWeaponCategory;
        func_800F9E0C();
        func_800F9A24(0);
        _respawnPending = 1;
    }

    if (_menuMode != 0) {
        for (i = 0; i < 8; ++i) {
            vs_main_inventoryIndices.weapons[i] = _savedWeapons[i];
            vs_main_inventoryIndices.shields[i] = _savedShields[i];
        }
        for (i = 0; i < 16; ++i) {
            vs_main_inventoryIndices.armor[i] = _savedArmor[i];
        }
    }
    _menuMode = 0;
}

/* --- MENU3.PRG call sites ---------------------------------------------------------------------
 * MENU3 can't be rebuilt (it would outgrow its sectors), so some of its calls are redirected
 * here by tools/mod/build_mod.py menu3. Each wrapper calls the original function it replaced. */

/* D_800F4EE8.unk3A.currentItemCategory (5BF94.h clashes with callo.c's declarations) */
extern u_char D_800F4EE8[];
#define CURRENT_ITEM_CATEGORY D_800F4EE8[0x3A]

#define MENU3_ITEM_NAVIGATION 0x801053EC  /* _itemNavigation */
#define MENU3_EQUIP_MENU 0x80108088       /* _equipMenu */
#define MENU3_LOAD_OBJECT_DATA 0x801090C4 /* _loadObjectData(category, wepId, material) */
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
            _equipEnter(NULL);
        } else if (row == 3) {
            callo = _calloActor();
            if (callo == NULL) {
                return 0;
            }
            _equipEnter(callo);
            row = 1;
        }
    }
    return row;
}

/* vs_menu3_exec's `row = _equipMenu(0)`: non-zero means EQUIP was left */
int vs_mod_equipMenu(int initialize)
{
    int row = ((int (*)(int))MENU3_EQUIP_MENU)(initialize);

    if (row != 0) {
        _equipLeave();
    }
    return row;
}

/* The weapon and shield screens' model loads (Ashley's in game, then the preview from him),
 * called each frame until they return 0. In MERLOSE her models are left alone: attaching a new
 * weapon model while she still has the old weapon type's animations crashed the renderer. She
 * gets the right models and animations when she respawns after the menu (see below). */
int vs_mod_loadObjectData(int category, int wepId, int material)
{
    if (_menuMode != 2) {
        return ((int (*)(int, int, int))MENU3_LOAD_OBJECT_DATA)(category, wepId, material);
    }
    return 0;
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

int vs_mod_frame(void)
{
    int result = func_80088554();

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
