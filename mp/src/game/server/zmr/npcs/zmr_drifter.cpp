#include "cbase.h"
#include "npcevent.h"
#include "eventlist.h"

#include "zmr_gamerules.h"
#include "zmr_drifter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define ZOMBIE_MODEL            "models/humans/zm_draggy.mdl"


extern ConVar zm_sk_dragzombie_health;
extern ConVar zm_sk_dragzombie_dmg;


int CZMDrifter::AE_DRAGGY_SICK;


LINK_ENTITY_TO_CLASS( npc_dragzombie, CZMDrifter );
PRECACHE_REGISTER( npc_dragzombie );

IMPLEMENT_SERVERCLASS_ST( CZMDrifter, DT_ZM_Drifter )
END_SEND_TABLE()

CZMDrifter::CZMDrifter()
{
    SetZombieClass( ZMCLASS_DRIFTER );
    CZMRules::IncPopCount( GetZombieClass() );
}

CZMDrifter::~CZMDrifter()
{
}

void CZMDrifter::Precache()
{
    if ( !IsPrecacheAllowed() )
        return;


    PrecacheScriptSound( "NPC_DragZombie.Die" );
    PrecacheScriptSound( "NPC_DragZombie.Idle" );
    PrecacheScriptSound( "NPC_DragZombie.Pain" );
    PrecacheScriptSound( "NPC_DragZombie.Alert" );
    PrecacheScriptSound( "NPC_DragZombie.MeleeAttack" );

    PrecacheScriptSound( "NPC_DragZombie.FastBreath" );
    PrecacheScriptSound( "NPC_DragZombie.Moan1" );




    REGISTER_PRIVATE_ANIMEVENT( AE_DRAGGY_SICK );


    BaseClass::Precache();
}

void CZMDrifter::Spawn()
{
    SetMaxHealth( zm_sk_dragzombie_health.GetInt() );


    BaseClass::Spawn();
}

void CZMDrifter::HandleAnimEvent( animevent_t* pEvent )
{
    if ( pEvent->event == AE_DRAGGY_SICK )
    {
        Vector vecSpit;
        Vector vecSpitDir;
        QAngle angSpit;
        GetAttachment( "Mouth", vecSpit, angSpit );
        AngleVectors( angSpit, &vecSpitDir );

        // Move to client?
        UTIL_BloodSpray( vecSpit, vecSpitDir, BLOOD_COLOR_RED, random->RandomInt( 4, 16 ), FX_BLOODSPRAY_ALL );


        Vector vecMins, vecMaxs;
        GetAttackHull( vecMins, vecMaxs );

        Vector vecDir;
        if ( GetEnemy() ) // Make sure we're attacking the enemy head on.
        {
            vecDir = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
        }
        else
        {
            AngleVectors( GetAbsAngles(), &vecDir );
        }
        
        vecDir.z = 0.0f;


        CUtlVector<CBaseEntity*> vHitEnts;
        MeleeAttackTrace(
            vecMins, vecMaxs,
            GetClawAttackRange(),
            zm_sk_dragzombie_dmg.GetInt(),
            DMG_ACID,
            &vHitEnts,
            &vecDir );

        for ( int i = 0; i < vHitEnts.Count(); i++ )
        {
            CBasePlayer* pHurt = ToBasePlayer( vHitEnts[i] );
            if ( pHurt )
            {
                pHurt->ViewPunch( QAngle(
                    random->RandomFloat( -50.0f, 50.0f ),
                    random->RandomFloat( -50.0f, 50.0f ),
                    random->RandomFloat( -50.0f, 50.0f ) ) );

                pHurt->SetAbsVelocity( vec3_origin );

                // Disorientate them further.
                pHurt->SetAbsAngles( QAngle(
                    pHurt->GetAbsAngles().x + random->RandomInt( -10.0f, 10.0f ),
                    pHurt->GetAbsAngles().y + random->RandomInt( -10.0f, 10.0f ),
                    pHurt->GetAbsAngles().z ) );
            }
        }


        EmitSound( "NPC_DragZombie.MeleeAttack" );


        // Tell our components we just attacked.
        OnAttacked();


        return;
    }

    BaseClass::HandleAnimEvent( pEvent );
}

void CZMDrifter::GetAttackHull( Vector& mins, Vector& maxs ) const
{
    mins = -Vector( 64.0f, 64.0f, 0.0f );
    maxs = Vector( 64.0f, 64.0f, 0.0f );

    mins.z = GetAttackLowest();
    maxs.z = GetAttackHeight();
}

bool CZMDrifter::ShouldPlayIdleSound() const
{
    return  BaseClass::ShouldPlayIdleSound()
    &&      GetEnemy() == nullptr // We must be idling.
    &&      random->RandomInt( 0, 99 ) == 0;
}

float CZMDrifter::IdleSound()
{
    EmitSound( "NPC_DragZombie.Idle" );
    g_flLastZombieSound = gpGlobals->curtime;
    return 1.0f;
}

void CZMDrifter::AlertSound()
{
    EmitSound( "NPC_DragZombie.Alert" );
    g_flLastZombieSound = gpGlobals->curtime;
}

void CZMDrifter::DeathSound()
{
    EmitSound( "NPC_DragZombie.Die" );
    g_flLastZombieSound = gpGlobals->curtime;
}
