#include "cbase.h"
#include "effect_dispatch_data.h"
#include "itempents.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "npcevent.h"
#include "eventlist.h"



#ifdef CLIENT_DLL
#include "prediction.h"
#else
#include "ilagcompensationmanager.h"
#endif


#include "zmr_basemelee.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define MASK_MELEE          ( MASK_SHOT_HULL | CONTENTS_HITBOX )


void DispatchEffect( const char *pName, const CEffectData &data );




IMPLEMENT_NETWORKCLASS_ALIASED( ZMBaseMeleeWeapon, DT_ZM_BaseMeleeWeapon )

BEGIN_NETWORK_TABLE( CZMBaseMeleeWeapon, DT_ZM_BaseMeleeWeapon )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMBaseMeleeWeapon )
END_PREDICTION_DATA()
#endif

BEGIN_DATADESC( CZMBaseMeleeWeapon )
END_DATADESC()

LINK_ENTITY_TO_CLASS( weapon_zm_basemelee, CZMBaseMeleeWeapon );


#define BLUDGEON_HULL_DIM		16

static const Vector g_bludgeonMins(-BLUDGEON_HULL_DIM,-BLUDGEON_HULL_DIM,-BLUDGEON_HULL_DIM);
static const Vector g_bludgeonMaxs(BLUDGEON_HULL_DIM,BLUDGEON_HULL_DIM,BLUDGEON_HULL_DIM);


CZMBaseMeleeWeapon::CZMBaseMeleeWeapon()
{
    m_fMinRange1 = m_fMinRange2 = 0.0f;
    m_fMaxRange1 = m_fMaxRange2 = 128.0f;


    m_bFiresUnderwater = true;
    
#ifndef CLIENT_DLL
    SetSlotFlag( ZMWEAPONSLOT_MELEE );
#endif


    m_flAttackHitTime = 0.0f;
}

void CZMBaseMeleeWeapon::ItemPostFrame()
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;

    // We have to go around the ammo requirement.
    if ( pPlayer->m_nButtons & IN_ATTACK && m_flNextPrimaryAttack <= gpGlobals->curtime )
    {
        PrimaryAttack();
    }
    else if ( pPlayer->m_nButtons & IN_ATTACK2 && m_flNextSecondaryAttack <= gpGlobals->curtime )
    {
        SecondaryAttack();
    }
    else
    {
        WeaponIdle();
    }

    // It's time to attack!
    if ( m_flAttackHitTime != 0.0f && m_flAttackHitTime <= gpGlobals->curtime )
    {
        StartHit( nullptr, GetActivity() );
        m_flAttackHitTime = 0.0f;
    }
}

bool CZMBaseMeleeWeapon::Deploy()
{
    bool res = BaseClass::Deploy();

    if ( res )
    {
        m_flAttackHitTime = 0.0f;
    }

    return res;
}

void CZMBaseMeleeWeapon::PrimaryAttack()
{
    if ( !CanPrimaryAttack() )
        return;


    Swing( false );


    // Setup our next attack times
    m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
    m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CZMBaseMeleeWeapon::SecondaryAttack()
{
    if ( !CanSecondaryAttack() )
        return;

        
    Swing( true );


    // Setup our next attack times
    m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
    m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CZMBaseMeleeWeapon::StartHit( trace_t* traceRes, Activity iActivityDamage )
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;


#ifndef CLIENT_DLL
    lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

    trace_t traceHit;
    TraceMeleeAttack( traceHit );

    Hit( traceHit, iActivityDamage );

#ifndef CLIENT_DLL
    lagcompensation->FinishLagCompensation( pPlayer );
#endif

    if ( traceRes )
    {
        *traceRes = traceHit;
    }
}

void CZMBaseMeleeWeapon::TraceMeleeAttack( trace_t& traceHit )
{
    traceHit.fraction = 0.0f;
    traceHit.m_pEnt = nullptr;


    CZMPlayer* pOwner = GetPlayerOwner();
    if ( !pOwner ) return;


    

    Vector swingStart = pOwner->Weapon_ShootPosition();
    Vector forward;

    pOwner->EyeVectors( &forward, nullptr, nullptr );

    Vector swingEnd = swingStart + forward * GetRange();
    UTIL_TraceLine( swingStart, swingEnd, MASK_MELEE, pOwner, COLLISION_GROUP_NONE, &traceHit );

    if ( traceHit.fraction == 1.0f )
    {
        float bludgeonHullRadius = 1.732f * BLUDGEON_HULL_DIM;  // hull is +/- 16, so use cuberoot of 2 to determine how big the hull is from center to the corner point

        // Back off by hull "radius"
        swingEnd -= forward * bludgeonHullRadius;

        UTIL_TraceHull( swingStart, swingEnd, g_bludgeonMins, g_bludgeonMaxs, MASK_MELEE, pOwner, COLLISION_GROUP_NONE, &traceHit );
        if ( traceHit.fraction < 1.0 && traceHit.m_pEnt )
        {
            Vector vecToTarget = traceHit.m_pEnt->GetAbsOrigin() - swingStart;
            VectorNormalize( vecToTarget );

            float dot = vecToTarget.Dot( forward );

            // YWB:  Make sure they are sort of facing the guy at least...
            if ( dot < 0.70721f )
            {
                // Force amiss
                traceHit.fraction = 1.0f;
            }
            else
            {
                ChooseIntersectionPoint( traceHit, g_bludgeonMins, g_bludgeonMaxs );
            }
        }
    }
}

void CZMBaseMeleeWeapon::Hit( trace_t& traceHit, Activity nHitActivity )
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;

    CBaseEntity* pHitEntity = traceHit.m_pEnt;


    const bool bRunEffects =
#ifdef CLIENT_DLL
    prediction->IsFirstTimePredicted();
#else
    true;
#endif


    if ( pHitEntity )
    {
        Vector hitDirection;
        pPlayer->EyeVectors( &hitDirection, NULL, NULL );
        VectorNormalize( hitDirection );

#ifdef CLIENT_DLL
        int dmg = GetDamageForActivity( nHitActivity );
#else
        int dmg = GetOverrideDamage() > -1 ? GetOverrideDamage() : GetDamageForActivity( nHitActivity );
#endif
        CTakeDamageInfo info( GetOwner(), GetOwner(), this, dmg, DMG_CLUB, 0 );

        //if( pHitEntity->IsNPC() )
        //{
            // If bonking an NPC, adjust damage.
        //    info.AdjustPlayerDamageInflictedForSkillLevel();
        //}

        CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos );

        pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit ); 
        ApplyMultiDamage();

#ifndef CLIENT_DLL
        // Now hit all triggers along the ray that... 
        TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );
#endif

        WeaponSound( MELEE_HIT );
    }


    if ( bRunEffects )
    {
        // Apply an impact effect
        ImpactEffect( traceHit );
    }
}

void CZMBaseMeleeWeapon::Swing( bool bSecondary )
{
    CZMPlayer* pOwner = GetPlayerOwner();
    if ( !pOwner ) return;



    const bool bUseAnimEvent = UsesAnimEvent( bSecondary );



    Activity nHitActivity = bSecondary ? GetSecondaryAttackActivity() : GetPrimaryAttackActivity();

    if ( !bUseAnimEvent )
    {
        trace_t traceHit;
        traceHit.fraction = 1.0f;
        StartHit( &traceHit, nHitActivity );

        if ( traceHit.fraction == 1.0f ) nHitActivity = bSecondary ? ACT_VM_MISSCENTER2 : ACT_VM_MISSCENTER;
    }
    
    // Send the anim
    SendWeaponAnim( nHitActivity );


    if ( bUseAnimEvent )
    {
        // Currently, only supports firstperson
        float waittime = GetFirstInstanceOfAnimEventTime( GetSequence(), AE_ZM_MELEEHIT );
        if ( waittime >= 0.0f )
            m_flAttackHitTime = gpGlobals->curtime + waittime;
    }


    WeaponSound( bSecondary ? GetSecondaryAttackSound() : GetPrimaryAttackSound() );
    pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );


    if ( !bUseAnimEvent )
    {
        AddViewKick();

#ifndef CLIENT_DLL
        PlayAISound();
#endif
    }
}

void CZMBaseMeleeWeapon::ChooseIntersectionPoint( trace_t& hitTrace, const Vector& mins, const Vector& maxs )
{
    int         i, j, k;
    float       distance;
    const float *minmaxs[2] = {mins.Base(), maxs.Base()};
    trace_t     tmpTrace;
    Vector      vecHullEnd = hitTrace.endpos;
    Vector      vecEnd;
    CBaseCombatCharacter* pOwner = GetOwner();

    distance = 1e6f;
    Vector vecSrc = hitTrace.startpos;


    vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
    UTIL_TraceLine( vecSrc, vecHullEnd, MASK_MELEE, pOwner, COLLISION_GROUP_NONE, &tmpTrace );
    if ( tmpTrace.fraction == 1.0f )
    {
        for ( i = 0; i < 2; i++ )
        {
            for ( j = 0; j < 2; j++ )
            {
                for ( k = 0; k < 2; k++ )
                {
                    vecEnd.x = vecHullEnd.x + minmaxs[i][0];
                    vecEnd.y = vecHullEnd.y + minmaxs[j][1];
                    vecEnd.z = vecHullEnd.z + minmaxs[k][2];

                    UTIL_TraceLine( vecSrc, vecEnd, MASK_MELEE, pOwner, COLLISION_GROUP_NONE, &tmpTrace );
                    if ( tmpTrace.fraction < 1.0 )
                    {
                        float thisDistance = (tmpTrace.endpos - vecSrc).Length();
                        if ( thisDistance < distance )
                        {
                            hitTrace = tmpTrace;
                            distance = thisDistance;
                        }
                    }
                }
            }
        }
    }
    else
    {
        hitTrace = tmpTrace;
    }
}

bool CZMBaseMeleeWeapon::ImpactWater( const Vector& start, const Vector& end )
{
    // We must start outside the water
    if ( UTIL_PointContents( start ) & (CONTENTS_WATER|CONTENTS_SLIME))
        return false;

    // We must end inside of water
    if ( !(UTIL_PointContents( end ) & (CONTENTS_WATER|CONTENTS_SLIME)))
        return false;

    trace_t	waterTrace;

    UTIL_TraceLine( start, end, (CONTENTS_WATER|CONTENTS_SLIME), GetOwner(), COLLISION_GROUP_NONE, &waterTrace );

    if ( waterTrace.fraction < 1.0f )
    {
#ifndef CLIENT_DLL
        CEffectData	data;

        data.m_fFlags  = 0;
        data.m_vOrigin = waterTrace.endpos;
        data.m_vNormal = waterTrace.plane.normal;
        data.m_flScale = 8.0f;

        // See if we hit slime
        if ( waterTrace.contents & CONTENTS_SLIME )
        {
            data.m_fFlags |= FX_WATER_IN_SLIME;
        }

        DispatchEffect( "watersplash", data );			
#endif
    }

    return true;
}

void CZMBaseMeleeWeapon::ImpactEffect( trace_t& traceHit )
{
    // See if we hit water (we don't do the other impact effects in this case)
    if ( ImpactWater( traceHit.startpos, traceHit.endpos ) )
        return;


    if ( traceHit.m_pEnt )
        UTIL_ImpactTrace( &traceHit, DMG_CLUB );
}

#ifndef CLIENT_DLL
void CZMBaseMeleeWeapon::Operator_HandleAnimEvent( animevent_t* pEvent, CBaseCombatCharacter* pOperator )
{
	switch( pEvent->event )
	{
	case AE_ZM_MELEEHIT:
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#else
bool CZMBaseMeleeWeapon::OnFireEvent( C_BaseViewModel* pViewModel, const Vector& origin, const QAngle& angles, int event, const char* options )
{
    if ( event == AE_ZM_MELEEHIT )
    {
        return true;
    }

    return false;
}
#endif

