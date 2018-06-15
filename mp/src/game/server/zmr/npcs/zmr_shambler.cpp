#include "cbase.h"
#include "npcevent.h"
#include "activitylist.h"

#include "zmr_gamerules.h"
#include "zmr_shambler.h"


extern ConVar zm_sk_shambler_health;
extern ConVar zm_sk_shambler_dmg_oneslash;
extern ConVar zm_sk_shambler_dmg_bothslash;



Activity CZMShambler::ACT_ZOM_SWATLEFTMID = ACT_INVALID;
Activity CZMShambler::ACT_ZOM_SWATLEFTLOW = ACT_INVALID;
Activity CZMShambler::ACT_ZOM_SWATRIGHTMID = ACT_INVALID;
Activity CZMShambler::ACT_ZOM_SWATRIGHTLOW = ACT_INVALID;

extern ConVar zm_sk_shambler_hitmult_legs;
extern ConVar zm_sk_shambler_hitmult_stomach;
extern ConVar zm_sk_shambler_hitmult_chest;
extern ConVar zm_sk_shambler_hitmult_arms;


LINK_ENTITY_TO_CLASS( npc_zombie, CZMShambler );
PRECACHE_REGISTER( npc_zombie );

IMPLEMENT_SERVERCLASS_ST( CZMShambler, DT_ZM_Shambler )
END_SEND_TABLE()

CZMShambler::CZMShambler()
{
    SetZombieClass( ZMCLASS_SHAMBLER );
    CZMRules::IncPopCount( GetZombieClass() );
}

CZMShambler::~CZMShambler()
{
}

void CZMShambler::Precache()
{
    if ( !IsPrecacheAllowed() )
        return;
    

    PrecacheScriptSound( "Zombie.FootstepRight" );
    PrecacheScriptSound( "Zombie.FootstepLeft" );
    PrecacheScriptSound( "Zombie.ScuffRight" );
    PrecacheScriptSound( "Zombie.ScuffLeft" );
    PrecacheScriptSound( "Zombie.AttackHit" );
    PrecacheScriptSound( "Zombie.AttackMiss" );
    PrecacheScriptSound( "Zombie.Pain" );
    PrecacheScriptSound( "Zombie.Die" );
    PrecacheScriptSound( "Zombie.Alert" );
    PrecacheScriptSound( "Zombie.Idle" );
    PrecacheScriptSound( "Zombie.Attack" );

    PrecacheScriptSound( "NPC_BaseZombie.Moan1" );
    PrecacheScriptSound( "NPC_BaseZombie.Moan2" );
    PrecacheScriptSound( "NPC_BaseZombie.Moan3" );
    PrecacheScriptSound( "NPC_BaseZombie.Moan4" );

    REGISTER_PRIVATE_ACTIVITY( ACT_ZOM_SWATLEFTMID );
    REGISTER_PRIVATE_ACTIVITY( ACT_ZOM_SWATLEFTLOW );
    REGISTER_PRIVATE_ACTIVITY( ACT_ZOM_SWATRIGHTMID );
    REGISTER_PRIVATE_ACTIVITY( ACT_ZOM_SWATRIGHTLOW );


    CZMBaseZombie::Precache();
}

void CZMShambler::Spawn()
{
    SetMaxHealth( zm_sk_shambler_health.GetInt() );


    BaseClass::Spawn();
}

void CZMShambler::HandleAnimEvent( animevent_t* pEvent )
{
    if ( pEvent->event == AE_ZOMBIE_ATTACK_RIGHT )
    {
        Vector right, forward;
        AngleVectors( GetLocalAngles(), &forward, &right, NULL );
        
        right = right * 100;
        forward = forward * 200;

        QAngle viewpunch( -15, -20, -10 );
        Vector punchvel = right + forward;

        ClawAttack( GetClawAttackRange(), zm_sk_shambler_dmg_oneslash.GetInt(), viewpunch, punchvel );
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_ATTACK_LEFT )
    {
        Vector right, forward;
        AngleVectors( GetLocalAngles(), &forward, &right, NULL );

        right = right * -100;
        forward = forward * 200;

        QAngle viewpunch( -15, -20, -10 );
        Vector punchvel = right + forward;

        ClawAttack( GetClawAttackRange(), zm_sk_shambler_dmg_oneslash.GetInt(), viewpunch, punchvel );
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_ATTACK_BOTH )
    {
        Vector forward;
        QAngle qaPunch( 45, random->RandomInt(-5,5), random->RandomInt(-5,5) );
        AngleVectors( GetLocalAngles(), &forward );
        forward = forward * 200;
        ClawAttack( GetClawAttackRange(), zm_sk_shambler_dmg_bothslash.GetInt(), qaPunch, forward );
        return;
    }

    BaseClass::HandleAnimEvent( pEvent );
}

bool CZMShambler::CanBreakObject( CBaseEntity* pEnt, bool bSwat ) const
{
    if ( pEnt->MyCombatCharacterPointer() != nullptr )
        return false;

    // As long as we can break it it's all good.
    return ( pEnt->GetHealth() > 0 && pEnt->m_takedamage == DAMAGE_YES );
}

Activity CZMShambler::GetSwatActivity( CBaseEntity* pEnt, bool bBreak ) const
{
    if ( !pEnt )
    {
        return ACT_ZOM_SWATLEFTMID;
    }

    if ( bBreak && CanBreakObject( pEnt ) )
    {
        return ACT_MELEE_ATTACK1;
    }


    float   flDot;
    Vector  vecRight, vecDirToObj;
    float   z;

    const Vector vecMyCenter = WorldSpaceCenter();
    const Vector vecObjCenter = pEnt->WorldSpaceCenter();


    AngleVectors( GetLocalAngles(), nullptr, &vecRight, nullptr );
    
    vecDirToObj = vecObjCenter - vecMyCenter;
    VectorNormalize( vecDirToObj );

    // compare in 2D.
    vecRight.z = 0.0;
    vecDirToObj.z = 0.0;

    flDot = DotProduct( vecRight, vecDirToObj );


    z = vecMyCenter.z - vecObjCenter.z;

    if( flDot >= 0 )
    {
        // Right
        if( z < 0 )
        {
            return ACT_ZOM_SWATRIGHTMID;
        }

        return ACT_ZOM_SWATRIGHTLOW;
    }
    else
    {
        // Left
        if( z < 0 )
        {
            return ACT_ZOM_SWATLEFTMID;
        }

        return ACT_ZOM_SWATLEFTLOW;
    }
}

bool CZMShambler::ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const
{
    bool res = BaseClass::ScaleDamageByHitgroup( iHitGroup, info );
    if ( res )
        return res;


    switch ( iHitGroup )
    {
    case HITGROUP_LEFTARM :
    case HITGROUP_RIGHTARM :
        info.ScaleDamage( zm_sk_shambler_hitmult_arms.GetFloat() );
        return true;
    case HITGROUP_CHEST :
        info.ScaleDamage( zm_sk_shambler_hitmult_chest.GetFloat() );
        return true;
    case HITGROUP_STOMACH :
        info.ScaleDamage( zm_sk_shambler_hitmult_stomach.GetFloat() );
        return true;
    case HITGROUP_LEFTLEG :
    case HITGROUP_RIGHTLEG :
        info.ScaleDamage( zm_sk_shambler_hitmult_legs.GetFloat() );
        return true;
    default :
        break;
    }

    return false;
}

void CZMShambler::AlertSound()
{
    EmitSound( "Zombie.Alert" );
}

void CZMShambler::AttackSound()
{
    EmitSound( "Zombie.Attack" );
}

void CZMShambler::DeathSound()
{
    EmitSound( "Zombie.Die" );
}

void CZMShambler::FootstepSound( bool bRightFoot )
{
    EmitSound( bRightFoot ? "Zombie.FootstepRight" : "Zombie.FootstepLeft" );
}

void CZMShambler::FootscuffSound( bool bRightFoot )
{
    EmitSound( bRightFoot ? "Zombie.ScuffRight" : "Zombie.ScuffLeft" );
}
