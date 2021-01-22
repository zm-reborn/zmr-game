#include "cbase.h"

#include "zmr_weaponconfig.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace ZMWeaponConfig;


//
// These are the animations that map to a specific sequence/gesture
//
// Format:
// Generic Activity (called by code), Specific Activity (can be a gesture) (defined in the model), Required (?)
//
// If you're doing custom player models, make sure to implement the ACT_HL2MP_* and ACT_ZM_* activities.
//

static acttable_t rifle_anims[] =
{
    { ACT_MP_STAND_IDLE,                ACT_HL2MP_IDLE_AR2,                     false },
    { ACT_MP_CROUCH_IDLE,               ACT_HL2MP_IDLE_CROUCH_AR2,              false },
    { ACT_MP_RUN,                       ACT_HL2MP_RUN_AR2,                      false },
    { ACT_MP_CROUCHWALK,                ACT_HL2MP_WALK_CROUCH_AR2,              false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,     false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,     false },
    { ACT_MP_RELOAD_STAND,              ACT_HL2MP_GESTURE_RELOAD_AR2,           false },
    { ACT_MP_RELOAD_CROUCH,             ACT_HL2MP_GESTURE_RELOAD_AR2,           false },
    { ACT_MP_JUMP,                      ACT_HL2MP_JUMP_AR2,                     false },
};

static acttable_t mac10_anims[] = 
{
    { ACT_MP_STAND_IDLE,                ACT_HL2MP_IDLE_PISTOL,                  false },
    { ACT_MP_CROUCH_IDLE,               ACT_HL2MP_IDLE_CROUCH_PISTOL,           false },
    { ACT_MP_RUN,                       ACT_HL2MP_RUN_PISTOL,                   false },
    { ACT_MP_CROUCHWALK,                ACT_HL2MP_WALK_CROUCH_PISTOL,           false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,    false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,    false },
    { ACT_MP_RELOAD_STAND,              ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
    { ACT_MP_RELOAD_CROUCH,             ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
    { ACT_MP_JUMP,                      ACT_HL2MP_JUMP_PISTOL,                  false },
};

static acttable_t pistol_anims[] = 
{
    { ACT_MP_STAND_IDLE,                ACT_HL2MP_IDLE_PISTOL,                  false },
    { ACT_MP_CROUCH_IDLE,               ACT_HL2MP_IDLE_CROUCH_PISTOL,           false },
    { ACT_MP_RUN,                       ACT_HL2MP_RUN_PISTOL,                   false },
    { ACT_MP_CROUCHWALK,                ACT_HL2MP_WALK_CROUCH_PISTOL,           false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,  false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,  false },
    { ACT_MP_RELOAD_STAND,              ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
    { ACT_MP_RELOAD_CROUCH,             ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
    { ACT_MP_JUMP,                      ACT_HL2MP_JUMP_PISTOL,                  false },
};

static acttable_t molotov_anims[] = 
{
    { ACT_MP_STAND_IDLE,                ACT_HL2MP_IDLE_GRENADE,                 false },
    { ACT_MP_CROUCH_IDLE,               ACT_HL2MP_IDLE_CROUCH_GRENADE,          false },
    { ACT_MP_RUN,                       ACT_HL2MP_RUN_GRENADE,                  false },
    { ACT_MP_CROUCHWALK,                ACT_HL2MP_WALK_CROUCH_GRENADE,          false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, false },
    { ACT_MP_RELOAD_STAND,              ACT_HL2MP_GESTURE_RELOAD_GRENADE,       false },
    { ACT_MP_RELOAD_CROUCH,             ACT_HL2MP_GESTURE_RELOAD_GRENADE,       false },
    { ACT_MP_JUMP,                      ACT_HL2MP_JUMP_GRENADE,                 false },
};

static acttable_t fists_anims[] = 
{
    { ACT_MP_STAND_IDLE,                ACT_HL2MP_IDLE_MELEE,                   false },
    { ACT_MP_CROUCH_IDLE,               ACT_HL2MP_IDLE_CROUCH_MELEE,            false },
    { ACT_MP_RUN,                       ACT_HL2MP_RUN_MELEE,                    false },
    { ACT_MP_CROUCHWALK,                ACT_HL2MP_WALK_CROUCH_MELEE,            false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,   false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,   false },
    { ACT_MP_RELOAD_STAND,              ACT_HL2MP_GESTURE_RELOAD_PHYSGUN,       false },
    { ACT_MP_RELOAD_CROUCH,             ACT_HL2MP_GESTURE_RELOAD_PHYSGUN,       false },
    { ACT_MP_JUMP,                      ACT_HL2MP_JUMP_MELEE,                   false },
};

static acttable_t crowbar_anims[] = 
{
    { ACT_MP_STAND_IDLE,                ACT_HL2MP_IDLE_MELEE,                   false },
    { ACT_MP_CROUCH_IDLE,               ACT_HL2MP_IDLE_CROUCH_MELEE,            false },
    { ACT_MP_RUN,                       ACT_HL2MP_RUN_MELEE,                    false },
    { ACT_MP_CROUCHWALK,                ACT_HL2MP_WALK_CROUCH_MELEE,            false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,   false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,   false },
    { ACT_MP_RELOAD_STAND,              ACT_HL2MP_GESTURE_RELOAD_MELEE,         false },
    { ACT_MP_RELOAD_CROUCH,             ACT_HL2MP_GESTURE_RELOAD_MELEE,         false },
    { ACT_MP_JUMP,                      ACT_HL2MP_JUMP_MELEE,                   false },
};


static acttable_t sledge_anims[] = 
{
    { ACT_MP_STAND_IDLE,                ACT_ZM_IDLE_SLEDGE,                     false },
    { ACT_MP_CROUCH_IDLE,               ACT_ZM_IDLE_CROUCH_SLEDGE,              false },
    { ACT_MP_RUN,                       ACT_ZM_RUN_SLEDGE,                      false },
    { ACT_MP_CROUCHWALK,                ACT_ZM_WALK_CROUCH_SLEDGE,              false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_ZM_GESTURE_RANGE_ATTACK_SLEDGE,     false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_ZM_GESTURE_RANGE_ATTACK_SLEDGE,     false },
    { ACT_MP_RELOAD_STAND,              ACT_HL2MP_GESTURE_RELOAD_MELEE,         false },
    { ACT_MP_RELOAD_CROUCH,             ACT_HL2MP_GESTURE_RELOAD_MELEE,         false },
    { ACT_MP_JUMP,                      ACT_ZM_JUMP_SLEDGE,                     false },
};

static acttable_t shotgun_anims[] = 
{
    { ACT_MP_STAND_IDLE,                ACT_HL2MP_IDLE_SHOTGUN,                 false },
    { ACT_MP_CROUCH_IDLE,               ACT_HL2MP_IDLE_CROUCH_SHOTGUN,          false },
    { ACT_MP_RUN,                       ACT_HL2MP_RUN_SHOTGUN,                  false },
    { ACT_MP_CROUCHWALK,                ACT_HL2MP_WALK_CROUCH_SHOTGUN,          false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
    { ACT_MP_RELOAD_STAND,              ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,       false },
    { ACT_MP_RELOAD_CROUCH,             ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,       false },
    { ACT_MP_JUMP,                      ACT_HL2MP_JUMP_SHOTGUN,                 false },
};

static acttable_t sporting_anims[] = 
{
    { ACT_MP_STAND_IDLE,                ACT_HL2MP_IDLE_AR2,                     false },
    { ACT_MP_CROUCH_IDLE,               ACT_HL2MP_IDLE_CROUCH_AR2,              false },
    { ACT_MP_RUN,                       ACT_HL2MP_RUN_AR2,                      false },
    { ACT_MP_CROUCHWALK,                ACT_HL2MP_WALK_CROUCH_AR2,              false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,     false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,     false },
    { ACT_MP_RELOAD_STAND,              ACT_HL2MP_GESTURE_RELOAD_AR2,           false },
    { ACT_MP_RELOAD_CROUCH,             ACT_HL2MP_GESTURE_RELOAD_AR2,           false },
    { ACT_MP_JUMP,                      ACT_HL2MP_JUMP_AR2,                     false },
};

#define IMPLEMENT_WEAPON_ACTTABLE( wep_name, arr ) \
    m_mAnimationMap.Insert( wep_name, { arr, ARRAYSIZE( arr ) } );


void CZMWeaponConfigSystem::InitPlayerAnimMap()
{
    IMPLEMENT_WEAPON_ACTTABLE( "rifle", rifle_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "sniper", rifle_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "pistol", pistol_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "revolver", pistol_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "mac10", mac10_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "smg", mac10_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "molotov", molotov_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "throwable", molotov_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "fists", fists_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "crowbar", crowbar_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "sledge", sledge_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "shotgun", shotgun_anims );
    IMPLEMENT_WEAPON_ACTTABLE( "sporting", sporting_anims );
}

acttable_t* CZMWeaponConfigSystem::GetActivityList( const char* szAnimName, int& nActivityCount ) const
{
    auto index = m_mAnimationMap.Find( szAnimName );
    if ( index != m_mAnimationMap.InvalidIndex() )
    {
        auto& elem = m_mAnimationMap.Element( index );
        nActivityCount = elem.nActivityCount;
        return elem.actTable;
    }

    Assert( 0 );
    nActivityCount = 0;
    return nullptr;
}

acttable_t* CZMBaseWeaponConfig::GetActivityList( int& nActivityCount ) const
{
    if ( pszPlayerAnimsName )
    {
        return GetWeaponConfigSystem()->GetActivityList( pszPlayerAnimsName, nActivityCount );
    }

    AssertMsg( 0, "Weapon config couldn't find player animations '%s'!", pszPlayerAnimsName ? pszPlayerAnimsName : "" );
    nActivityCount = 0;
    return nullptr;
}
