/* Callo Merlose companion mod.
 *
 * Built into its own file on the disc (ENDING/CALLO.BIN) and loaded into free RAM above
 * BATTLE.PRG by stub.c, so no existing game code moves: undecompiled binaries (effects
 * etc.) call into BATTLE.PRG at fixed addresses. See tools/mod/build_mod.sh.
 *
 * vs_mod_spawnCallo must stay the first function in this file: it is the entry point.
 */
#include "common.h"
#include "../146C.h"
#include "../2842C.h"
#include "../30DB0.h"

void vs_mod_spawnCallo(void);
/* AI state setup / teardown (6E644.c, not yet decompiled) */
int func_800E6178(vs_battle_actor*, int);
void func_800E7454(vs_battle_actor*);
void func_8009DF3C(int, int);
/* Per-slot AI states (0x484 bytes each). +0x1A4 + id * 4 is this AI's attitude to actor id:
 * top two bits of the first byte, 3 = hostile (attack candidate), 2 = friendly, 1 = ignore. */
extern u_char* D_800F5878[16];
#define AI_ATTITUDES 0x1A4

static vs_battle_charInitData _calloInitData;
static vs_battle_weaponIntermediate _vkpWeapon;
static void _makeEnemiesHostile(int calloSlot);
static void _copyAshleyBaseStats(vs_battle_actor2* callo);
static vs_main_inventoryWeapon* _loadoutWeapon(void);
static vs_main_inventoryShield* _loadoutShield(void);
static void _applyLoadout(vs_battle_actor2* callo);
static void _dropSharedItems(void);
static void _learnSpells(vs_battle_actor2* callo);
static void _giveStartingArmor(void);
static void _giveStartingWeapon(void);

#ifndef CALLO_MODEL
#define CALLO_MODEL 8
#endif
/* Spawn param bits 0-1 = 1: hostile spawn (team 4, weapon drawn); also makes
 * func_800771E0 load the weapon-type animation set for slots >= 2. */
#define CALLO_SPAWN_PARAM 1
/* AI mode lives in bits 21-22 of vs_battle_actor.unk38 (copied from the template's
 * int at 0x2C for room spawns). Mode 3 is Sydney's: fight the player's enemies. This
 * is Sydney's full value; the other bits are the usual humanoid AI parameters. */
#define CALLO_AI_PARAMS 0xC07C7F1C
#define CALLO_ENTRY_INDEX 15
#define CALLO_WEAPON_MODEL 83    /* Gastraph Bow */
#define CALLO_WEAPON_MATERIAL 3  /* Bronze */

static const u_char _calloSlots[2] = { 7, 6 };
/* Word-sized statics need an initialiser: that puts them in .data, which is aligned. maspsx
 * packs uninitialised (.bss) statics byte by byte, as ASPSX did, so an int there can land on
 * an odd address (DuckStation then rounds the access down onto a neighbouring variable). */
static int _calloSlot = 0; /* where she last spawned (0: not yet) */

/* Her loadout lives in save flags 0x400-0x43F (vs_main_stateFlags.unk400): saved and loaded with
 * the game, and verified unused (no script can address flags >= 0x400; no code touches them).
 * A new game zeroes them, meaning "use the defaults". Each piece is an inventory slot (1-based,
 * 0 = default) plus the item's id, so a slot that was emptied or reused is ignored. */
#define LOADOUT ((u_char*)&vs_main_stateFlags + 0x400)
#define LOADOUT_VERSION 'C'
#define LOADOUT_EMPTY 0xFF /* armour/accessory slot left empty on purpose (0 = her default) */
enum {
    loadoutVersion = 0,
    loadoutWeapon = 1,    /* slot in weapons[], blade id */
    loadoutShield = 3,    /* slot in shields[], base id */
    loadoutArmor = 5,     /* 6 x (slot in armor[], id), one per body part */
    loadoutAccessory = 17, /* slot in armor[], id */
    loadoutFlags = 19      /* LOADOUT_BOW_GIVEN; not cleared with the slots above */
};
#define LOADOUT_BOW_GIVEN 1 /* VKP Standard Issue has been put in the inventory */
/* Weapon categories she has battle animations for: dagger, sword, axe/mace, crossbow */
#define CALLO_WEAPON_CATEGORIES ((1 << 1) | (1 << 2) | (1 << 4) | (1 << 9))

void vs_mod_spawnCallo(void)
{
    vs_battle_objectData_flags position;
    _mpdEnemy* anchor;
    vs_battle_actor* callo;
    u_char* raw;
    int i;
    int slot;
    vs_main_inventoryWeapon* weapon;
    vs_main_inventoryBlade* blade;
    vs_main_inventoryShield* shield;

    /* Test scope: every Wine Cellar room */
    if (_zoneContext.zndId != 9) { /* Wine Cellar */
        return;
    }
    /* An enemy slot (0-1 get player-only handling, creation allows < 10). Room scripts
     * move cutscene characters by slot and mostly use 2-5 and 8, so taking those made
     * scripts teleport her. Slot 9 is unusable: a character's palette goes to VRAM row
     * 0x16 + slot, and row 0x1F (slot 9) holds room objects' alternate palettes. */
    for (i = 0; i < 2; ++i) {
        slot = _calloSlots[i];
        if (vs_battle_actors[slot] == NULL) {
            break;
        }
    }
    if (i >= 2) {
        return;
    }

    /* Next to Ashley: his tile is already set for the new room at this point.
     * Live tile block is {x, facing, z, height}, the same layout as the spawn flags. */
    *(int*)&position = *(int*)((u_char*)D_800F4538[0] + 0x5C);
    position.unk0_0 += (position.unk0_0 != 0) ? -1 : 1;

    _dropSharedItems();
    _giveStartingArmor();
    _giveStartingWeapon();
    shield = _loadoutShield();
    weapon = _loadoutWeapon();
    if (weapon != NULL) {
        blade = &vs_main_inventory.blades[weapon->blade - 1];
        callo = func_800774FC(slot, CALLO_MODEL, blade->wepId, blade->material,
            shield ? shield->base.wepId : 0, shield ? shield->base.material : 0, &position,
            CALLO_SPAWN_PARAM);
    } else {
        /* No weapon: bare-handed */
        callo = func_800774FC(slot, CALLO_MODEL, 0, 0,
            shield ? shield->base.wepId : 0, shield ? shield->base.material : 0, &position,
            CALLO_SPAWN_PARAM);
    }
    if (callo == NULL) {
        return;
    }
    _calloSlot = slot;
    /* No 0x80: it sets actor->unk26, which room entries use for characters that can
     * only die by script (Grissom, Sydney), and made her immortal */
    func_80076F24(slot, &_calloInitData, 0x10, 0, CALLO_SPAWN_PARAM, 0);

    _copyAshleyBaseStats(callo->unk3C);
    _applyLoadout(callo->unk3C);
    _learnSpells(callo->unk3C);

    /* Fields func_800775C0 (enemy room spawn) copies from the room entry and Ashley's
     * spawn path leaves zeroed. Settings byte 0 matches the entry's index, so use an
     * index no real entry has, to keep saved room state (defeated/hidden) untouched. */
    raw = (u_char*)callo;
    if (_zoneContext.mpdEnemyCount != 0) {
        anchor = &_zoneContext.mpdEnemies[0];
        for (i = 0; i < 8; ++i) {
            raw[0x30 + i] = anchor->settings[i];
        }
        callo->unk1F = anchor->unk26;
    }
    raw[0x30] = CALLO_ENTRY_INDEX;
    callo->unk1E = 2; /* room entry team byte for a hostile placement */
    callo->unk27 = CALLO_ENTRY_INDEX;
    callo->defeated = 0;
    callo->unk29 = 0;

    /* Team 4 with weapon drawn comes from CALLO_SPAWN_PARAM, like Sydney's room spawn;
     * the AI mode below makes her fight the other enemies */
    /* The AI state is built from unk30-unk38 by func_800E6178, which the creation path
     * already ran with unk38 = 0 (mode 0). Room spawns run it last; rebuild it the same
     * way: tear down (as _removeActorAtIndex does) while she is still on the team the
     * creation path registered her under, change her fields, then initialise. */
    func_800E7454(callo);
#ifndef CALLO_NO_AI
    callo->unk38 = CALLO_AI_PARAMS;
#endif
    /* She must stay on the enemy team (4): team 1 is driven by the controller (no AI)
     * and crashed when entering/leaving rooms with enemies (tests L1/L2). */
    func_8009DF3C(slot, func_800E6178(callo, -1) ? 0 : callo->unk1F);
    _makeEnemiesHostile(slot);
}

/* The loadout's weapon, if it is still in its slot and is a type she can animate */
static vs_main_inventoryWeapon* _loadoutWeapon(void)
{
    u_char* loadout = LOADOUT;
    vs_main_inventoryWeapon* weapon;
    vs_main_inventoryBlade* blade;
    int slot = loadout[loadoutWeapon];

    if ((loadout[loadoutVersion] != LOADOUT_VERSION) || (slot == 0) || (slot > 8)) {
        return NULL;
    }
    weapon = &vs_main_inventory.weapons[slot - 1];
    if (weapon->blade == 0) {
        return NULL;
    }
    blade = &vs_main_inventory.blades[weapon->blade - 1];
    if ((blade->id != loadout[loadoutWeapon + 1])
        || !(CALLO_WEAPON_CATEGORIES & (1 << blade->category))) {
        return NULL;
    }
    return weapon;
}

/* The loadout's shield, if it is still in its slot */
static vs_main_inventoryShield* _loadoutShield(void)
{
    u_char* loadout = LOADOUT;
    int slot = loadout[loadoutShield];

    if ((loadout[loadoutVersion] != LOADOUT_VERSION) || (slot == 0) || (slot > 8)
        || (vs_main_inventory.shields[slot - 1].base.id != loadout[loadoutShield + 1])) {
        return NULL;
    }
    return &vs_main_inventory.shields[slot - 1];
}

/* Her default armour isn't in the inventory, so the menus showed her with none. Each armour
 * slot still on the default (0) gets a copy of the piece Ashley wears there, as a real item,
 * if the inventory has room. Slots emptied on purpose (LOADOUT_EMPTY) stay empty. */
static void _giveStartingArmor(void)
{
    u_char* loadout = LOADOUT;
    vs_main_inventoryArmor* armor = vs_main_inventory.armor;
    int part;
    int from;
    int to;
    int entry;

    if (loadout[loadoutVersion] != LOADOUT_VERSION) {
        for (part = 0; part <= loadoutAccessory + 1; ++part) {
            loadout[part] = 0;
        }
        loadout[loadoutVersion] = LOADOUT_VERSION;
    }
    for (part = 0; part < 6; ++part) {
        if (loadout[loadoutArmor + part * 2] != 0) {
            continue;
        }
        for (from = 0; (from < 16) && (armor[from].limb != part + 1); ++from)
            ;
        for (to = 0; (to < 16) && (armor[to].id != 0); ++to)
            ;
        for (entry = 0; (entry < 16) && (vs_main_inventoryIndices.armor[entry] != 0); ++entry)
            ;
        if ((from >= 16) || (to >= 16) || (entry >= 16)) {
            continue;
        }
        armor[to] = armor[from];
        armor[to].limb = 0;
        armor[to].index = to + 1;
        vs_main_inventoryIndices.armor[entry] = to + 1;
        loadout[loadoutArmor + part * 2] = to + 1;
        loadout[loadoutArmor + part * 2 + 1] = armor[to].id;
    }
}

/* Her default weapon (VKP Standard Issue) as a real inventory weapon, equipped on her: a blade
 * and a grip assembled into a weapon, as the game stores one. Done once (LOADOUT_BOW_GIVEN), so
 * an empty weapon slot later doesn't keep making copies; she is then bare-handed. */
static void _giveStartingWeapon(void)
{
    u_char* loadout = LOADOUT;
    vs_main_inventoryWeapon* weapon;
    int w;
    int b;
    int g;
    int i;

    if ((loadout[loadoutFlags] & LOADOUT_BOW_GIVEN) || (loadout[loadoutWeapon] != 0)) {
        return;
    }
    for (w = 0; (w < 8) && (vs_main_inventory.weapons[w].blade != 0); ++w)
        ;
    for (b = 0; (b < 16) && (vs_main_inventory.blades[b].id != 0); ++b)
        ;
    for (g = 0; (g < 16) && (vs_main_inventory.grips[g].id != 0); ++g)
        ;
    if ((w >= 8) || (b >= 16) || (g >= 16)) {
        return; /* no room: try again at the next spawn */
    }

    vs_battle_copyUiBladeStats(&vs_main_inventory.blades[b], &_vkpWeapon.blade);
    vs_main_inventory.blades[b].index = b + 1;
    vs_main_inventory.blades[b].assembledWeaponIndex = w + 1;
    vs_battle_copyUiGripStats(&vs_main_inventory.grips[g], &_vkpWeapon.grip);
    vs_main_inventory.grips[g].index = g + 1;
    vs_main_inventory.grips[g].assembledWeaponIndex = w + 1;

    weapon = &vs_main_inventory.weapons[w];
    weapon->index = w + 1;
    weapon->blade = b + 1;
    weapon->grip = g + 1;
    weapon->isEquipped = 0;
    for (i = 0; i < 4; ++i) {
        weapon->gems[i] = 0;
    }
    for (i = 0; i < 24; ++i) {
        weapon->name[i] = _vkpWeapon.name[i];
    }
    for (i = 0; (i < 8) && (vs_main_inventoryIndices.weapons[i] != 0); ++i)
        ;
    if (i < 8) {
        vs_main_inventoryIndices.weapons[i] = w + 1;
    }

    loadout[loadoutWeapon] = w + 1;
    loadout[loadoutWeapon + 1] = vs_main_inventory.blades[b].id;
    loadout[loadoutFlags] |= LOADOUT_BOW_GIVEN;
}

/* An item can't be on both characters; loadouts from before the MERLOSE menu could share one.
 * Ashley keeps it. Called while his "equipped" markers are in place. */
static void _dropSharedItems(void)
{
    u_char* loadout = LOADOUT;
    int slot;
    int i;

    if (loadout[loadoutVersion] != LOADOUT_VERSION) {
        return;
    }
    slot = loadout[loadoutWeapon];
    if ((slot != 0) && (slot <= 8) && vs_main_inventory.weapons[slot - 1].isEquipped) {
        loadout[loadoutWeapon] = 0;
    }
    slot = loadout[loadoutShield];
    if ((slot != 0) && (slot <= 8) && vs_main_inventory.shields[slot - 1].isEquipped) {
        loadout[loadoutShield] = 0;
    }
    /* The six armour pieces, then the accessory (loadoutAccessory follows them) */
    for (i = 0; i < 7; ++i) {
        slot = loadout[loadoutArmor + i * 2];
        if ((slot != 0) && (slot <= 16) && vs_main_inventory.armor[slot - 1].limb) {
            loadout[loadoutArmor + i * 2] = 0;
        }
    }
}

/* Equip her loadout on top of the template's gear (bare hands, starting armour), the
 * way INITBTL.PRG equips Ashley. Pieces that are unset or no longer valid keep the template's. */
static void _applyLoadout(vs_battle_actor2* callo)
{
    u_char* loadout = LOADOUT;
    vs_main_inventoryWeapon* weapon;
    vs_main_inventoryArmor* armor;
    int slot;
    int i;

    if (loadout[loadoutVersion] != LOADOUT_VERSION) {
        return;
    }

    weapon = _loadoutWeapon();
    if (weapon != NULL) {
        vs_battle_applyWeapon(&callo->weapon, weapon);
        callo->currentRange = callo->weapon.range.range + callo->reach;
        callo->unk39 = callo->weapon.range.unk1 + callo->reach;
        callo->unk3A = callo->weapon.range.unk2 + callo->reach;
        callo->currentAttackShapeAngle = callo->weapon.range.attackShapeAngle;
    }

    if (_loadoutShield() != NULL) {
        vs_battle_applyShield(&callo->shield, _loadoutShield());
    }

    for (i = 0; i < 6; ++i) {
        slot = loadout[loadoutArmor + i * 2];
        if (slot == LOADOUT_EMPTY) {
            vs_battle_applyArmor(&callo->limbs[i].armor, NULL);
        } else if ((slot != 0) && (slot <= 16)) {
            armor = &vs_main_inventory.armor[slot - 1];
            if (armor->id == loadout[loadoutArmor + i * 2 + 1]) {
                vs_battle_applyArmor(&callo->limbs[i].armor, armor);
            }
        }
    }

    slot = loadout[loadoutAccessory];
    if (slot == LOADOUT_EMPTY) {
        vs_battle_applyAccessory(&callo->accessory, NULL);
    } else if ((slot != 0) && (slot <= 16)) {
        armor = &vs_main_inventory.armor[slot - 1];
        if (armor->id == loadout[loadoutAccessory + 1]) {
            vs_battle_applyAccessory(&callo->accessory, armor);
        }
    }
}

/* She knows the spells Ashley has learned. Her AI picks from the action table in her stats
 * (6 body parts x 4 actions; the template uses three: an attack on each arm and Heal), so the
 * learned spells go in its free entries. Until there's a menu to choose them, only attack and
 * cure spells: the AI re-casts buffs constantly (as Sydney did with Prostasia). Ids are
 * vs_main_actions[]: Ashley's spells are 54-97; the AI already casts ones from that range
 * (Grissom: Clearance, Drain Mind). */
#define MAX_LEARNED_SPELLS 8

static const u_char _defaultSpells[] = {
    78, /* Lightning Bolt */
    81, /* Fireball */
    84, /* Vulcan Lance */
    86, /* Aqua Blast */
    97, /* Solid Shock */
    87, /* Restoration */
    88, /* Antidote */
    91, /* Clearance */
};

static void _learnSpells(vs_battle_actor2* callo)
{
    vs_battle_actor_unk8C0* entry = &callo->armor[0][0];
    int learned = 0;
    int i;
    int j;

    for (i = 0; (i < sizeof _defaultSpells) && (learned < MAX_LEARNED_SPELLS); ++i) {
        if (!vs_main_actions[_defaultSpells[i]].unlocked) {
            continue;
        }
        for (j = 0; (j < 6 * 4) && (entry[j].id != 0); ++j)
            ;
        if (j >= 6 * 4) {
            return;
        }
        entry[j].id = _defaultSpells[i];
        entry[j].index = 0;
        entry[j].unk2_4 = 0;
        entry[j].unk3 = 1;
        ++learned;
    }
}

/* She grows with Ashley, as his counterpart: copy his base stats (total* = permanent values
 * incl. stat-up items, without temporary buffs; equipment is never part of these, it is added
 * at hit time) with STR and INT swapped, so a strength-built Ashley gets a caster Callo and
 * vice versa. Full HP/MP at spawn; her own gear then applies through the normal formulas. */
static void _copyAshleyBaseStats(vs_battle_actor2* callo)
{
    vs_battle_actor2* ashley = vs_battle_actors[0]->unk3C;

    callo->maxHP = callo->currentHP = ashley->maxHP;
    callo->maxMP = callo->currentMP = ashley->maxMP;
    callo->strength = callo->totalStrength = ashley->totalIntelligence;
    callo->intelligence = callo->totalIntelligence = ashley->totalStrength;
    callo->agility = callo->totalAgility = ashley->totalAgility;
}

/* Enemies regard Callo as a teammate (she is on team 4), so they never pick her as a target.
 * Give every enemy AI the same attitude towards her as towards Ashley (actor 0): verified
 * live that they then fight back when she attacks them, and otherwise go for Ashley. */
static void _makeEnemiesHostile(int calloSlot)
{
    int i;
    u_char* ai;

    for (i = 2; i < 16; ++i) {
        if ((i == calloSlot) || (vs_battle_actors[i] == NULL)
            || (vs_battle_actors[i]->unk1C != 4)) {
            continue;
        }
        ai = D_800F5878[i];
        if (ai != NULL) {
            ai[AI_ATTITUDES + calloSlot * 4] = ai[AI_ATTITUDES];
        }
    }
}


/* Callo Merlose companion (mod). Stats template modelled on Ashley's (INITBTL.PRG),
 * bare-handed so her BTA animation set is used, with Sydney's Heal. */
#pragma vsstring(start)
static vs_battle_charInitData _calloInitData = {
    .unk2         = 8,
    .reach        = 7,
    .name         = "Callo|>6|Merlose\0",
    /* Placeholders: replaced by Ashley's base stats at spawn (_copyAshleyBaseStats) */
    .hp           = 200,
    .mp           = 120,
    .strength     = 80,
    .intelligence = 120,
    .agility      = 100,
    .unk23        = 2,
    .unk24        = 2,
    .walkSpeed    = 10,
    .unk26        = 4,
    .runSpeed     = 20,
    .unk30        = 4,
    .unk32        = 3,
    .unk28        = 0x6009D2BF, /* Sydney's value (unique to him); purpose unknown */
    .accessory = {
        .equip = {
            .id           = 223,
            .subId        = 97,
            .category     = 7,
            .intelligence = 1,
            .classes      = {0, 0, 5, -5, -5, 5},
            .affinities   = {5, 0, 0, 0, 0, 5, -5, 0},
        },
    },
    .limbs = {
        {
            .hp = 200,
            .actions = {
                {.id = 1},
                {.index = 1},
                {.index = 2},
                {.index = 3},
            },
            .unk20 = {
                .equip = {
                    .id           = 191,
                    .subId        = 65,
                    .category     = 5,
                    .strength     = 1,
                    .intelligence = 13,
                    .currentDp    = 3350,
                    .maxDp        = 3350,
                    .types        = {0, 1, 0, 0},
                    .affinities   = {2, 5, 5, -1, -1, -5, -5, 0},
                },
                .material = 2,
                .unk32    = 1,
            },
            .dpSplash = {100, 0, 0, 5, 0, 0},
        },
        {
            .hp = 200,
            .actions = {
                {.id = 1},
                {.index = 1},
                {.index = 2},
                {.index = 3},
            },
            .unk20 = {
                .equip = {
                    .id           = 191,
                    .subId        = 65,
                    .category     = 5,
                    .strength     = 1,
                    .intelligence = 13,
                    .currentDp    = 3350,
                    .maxDp        = 3350,
                    .types        = {0, 1, 0, 0},
                    .affinities   = {2, 5, 5, -1, -1, -5, -5, 0},
                 },
                .material = 2,
                .unk32    = 1,
            },
            .dpSplash = {0, 100, 0, 5, 0, 0},
        },
        {
            .hp = 200,
            .actions = {
                {0},
                {.index = 1},
                {.index = 2},
                {.index = 3},
            },
            .unk20 = {
                .equip = {
                    .id           = 143,
                    .subId        = 17,
                    .category     = 2,
                    .strength     = 1,
                    .intelligence = 8,
                    .currentDp    = 3800,
                    .maxDp        = 3800,
                    .affinities = {2, 5, 5, -1, -1, -5, -5, 0},
                },
                .material = 2,
                .unk32    = 1,
            },
            .dpSplash = {0, 0, 100, 10, 0, 0},
        },
        {
            .hp = 200,
            .actions = {
                {.id = 0x60}, /* Heal */
                {.index = 1}, /* Sydney's Prostasia (0x3F) removed: re-cast constantly */
                {.index = 2},
                {.index = 3},
            },
            .unk20 = {
                .equip = {
                    .id           = 159,
                    .subId        = 33,
                    .category     = 3,
                    .strength     = 5,
                    .intelligence = 10,
                    .currentDp    = 5500,
                    .maxDp        = 5500,
                    .types        = {0, 1, 1, 0},
                    .affinities   = {2, 5, 5, -1, -1, -5, -5, 0},
                },
                .material = 2,
                .unk32 = 1,
            },
            .dpSplash = {5, 5, 5, 100, 5, 0},
        },
        {
            .hp = 200,
            .actions = {
                {0},
                {.index = 1},
                {.index = 2},
                {.index = 3},
            },
            .unk20 = {
                .equip = {
                    .id           = 175,
                    .subId        = 49,
                    .category     = 4,
                    .strength     = 1,
                    .intelligence = 12,
                    .currentDp    = 4050,
                    .maxDp        = 4050,
                    .affinities   = {2, 5, 5, -1, -1, -5, -5, 0},
                },
                .material = 2,
                .unk32    = 1,
            },
            .dpSplash = {0, 0, 0, 5, 100, 0},
        },
        {
            .actions = {
                {0},
                {.index = 1},
                {.index = 2},
                {.index = 3},
            },
        },
    },
};

/* Her starting weapon, put in the inventory by _giveStartingWeapon. "VKP Standard Issue": a copy
 * of Seventh Heaven (The Reckoning Room's chest) - Bronze Gastraph Bow + Simple Bolt, stats as
 * stored in that chest (MAP018 loot). */
static vs_battle_weaponIntermediate _vkpWeapon = {
    .blade = {
        .id           = 83,
        .subId        = 83,
        .wepId        = CALLO_WEAPON_MODEL,
        .category     = 9, /* crossbow: her BT9 animation set */
        .strength     = 11,
        .intelligence = 1,
        .agility      = -4,
        .currentDp    = 6600,
        .maxDp        = 6600,
        .maxPp        = 150,
        .damageType   = 3,
        .costType     = 2,
        .cost         = 2,
        .material     = CALLO_WEAPON_MATERIAL,
        .range        = {
            .range       = 9,
            .unk1        = 8,
            .unk2        = 9,
            .attackShape = 1,
        },
        .classes      = {-1, -1, -2, -1, -1, -5, 0, 0},
        .affinities   = {8, 5, 5, -3, -3, 2, 2, 0},
    },
    .grip = {
        .id           = 121, /* Simple Bolt */
        .subId        = 26,
        .category     = 4,
        .strength     = 1,
        .agility      = -1,
        .types        = {0, 1, 0, 10},
    },
    .material = CALLO_WEAPON_MATERIAL,
    .name     = "VKP|>6|Standard|>6|Issue\0",
};

/* The MERLOSE row of ITEMS > OPTIONS (see merlose.h) */
static char _merloseRow[] = "MERLOSE\0";
static char _merloseRowDesc[] = "Equip|>6|Callo|>6|Merlose\0";
static char _merloseRowAbsent[] = "Callo|>6|is|>6|not|>6|here\0";
#pragma vsstring(end)

#include "merlose.h"
