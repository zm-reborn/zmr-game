#include "cbase.h"


#include "zmr_player_shared.h"

#ifdef CLIENT_DLL
#include "prediction.h"

#include "zmr/c_zmr_player.h"
#include "zmr/npcs/c_zmr_zombiebase.h"
#else
#include "zmr/zmr_player.h"
#include "zmr/npcs/zmr_zombiebase.h"
#endif


#ifdef CLIENT_DLL
//#define CZMPlayer C_ZMPlayer
#define CZMBaseZombie C_ZMBaseZombie


extern ConVar zm_cl_participation;
#endif

extern ConVar zm_sv_resource_max;


ConVar zm_sv_bulletspassplayers( "zm_sv_bulletspassplayers", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED, "Do bullets players shoot pass through other players?" );


//
CZMPlayerAttackTraceFilter::CZMPlayerAttackTraceFilter( CBaseEntity* pAttacker, CBaseEntity* pIgnore, int collisionGroup ) : CTraceFilter()
{
    Assert( pAttacker != nullptr && pAttacker->IsPlayer() );
    m_pAttacker = pAttacker;
    m_pIgnore = pIgnore;
}

bool CZMPlayerAttackTraceFilter::ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask )
{
    CBaseEntity* pEntity = EntityFromEntityHandle( pHandleEntity );

    if ( !pEntity ) return false;
    if ( pEntity == m_pAttacker ) return false;
    if ( pEntity == m_pIgnore ) return false;


    // It's a bone follower of the entity to ignore (toml 8/3/2007)
    if ( pEntity->GetOwnerEntity() == m_pAttacker )
        return false;

    if ( m_pAttacker->IsPlayer() )
    {
#ifdef GAME_DLL
        // Clientside hit reg will do this for us.
        if ( g_ZMUserCmdSystem.UsesClientsideDetection( pEntity ) )
            return false;
#endif
        if (pEntity->IsPlayer()
        &&  pEntity->GetTeamNumber() == m_pAttacker->GetTeamNumber()
        &&  zm_sv_bulletspassplayers.GetBool())
            return false;
    }

    return true;
}
//


bool CZMPlayer::HasEnoughResToSpawn( ZombieClass_t zclass )
{
    return GetResources() >= CZMBaseZombie::GetCost( zclass );
}

bool CZMPlayer::HasEnoughRes( int cost )
{
    return GetResources() >= cost;
}

int CZMPlayer::GetWeaponSlotFlags()
{
    return m_ZMLocal.m_fWeaponSlotFlags;
}

int CZMPlayer::GetResources()
{
    return m_ZMLocal.m_nResources;
}

void CZMPlayer::IncResources( int res, bool bLimit )
{
#ifdef CLIENT_DLL

#else
    int oldres = GetResources();
    int newres = oldres + res;
    int max = zm_sv_resource_max.GetInt();


    if ( bLimit && newres > max )
    {
        if ( oldres < max )
            newres = max;
        else
            return;
    }

    SetResources( newres );
#endif
}

void CZMPlayer::SetResources( int res )
{
#ifdef CLIENT_DLL

#else
    if ( res < 0 ) res = 0;

    m_ZMLocal.m_nResources = res;
#endif
}

float CZMPlayer::GetFlashlightBattery()
{
    return m_ZMLocal.m_flFlashlightBattery;
}

void CZMPlayer::SetFlashlightBattery( float battery )
{
#ifdef CLIENT_DLL

#else
    m_ZMLocal.m_flFlashlightBattery = battery;
#endif
}

// This + AllowsAutoSwitchFrom was causing problems with switching to an empty weapon.
bool CZMPlayer::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
    if ( !IsAlive() )
        return false;

    if ( !pWeapon->CanDeploy() )
        return false;
    

    CBaseCombatWeapon* pActive = GetActiveWeapon();
    if ( pActive && !pActive->CanHolster() )
    {
        return false;
    }

    return true;
}

Participation_t CZMPlayer::GetParticipation()
{
    Participation_t part;
#ifdef CLIENT_DLL
    part = (Participation_t)zm_cl_participation.GetInt();
#else
    part =  (Participation_t)atoi( engine->GetClientConVarValue( entindex(), "zm_cl_participation" ) );
#endif

    if ( part <= ZMPART_INVALID || part >= ZMPART_MAX ) part = ZMPART_ALLOWZM;

    return part;
}

#ifdef CLIENT_DLL
Participation_t CZMPlayer::GetLocalParticipation()
{
    Participation_t part = (Participation_t)zm_cl_participation.GetInt();

    if ( part <= ZMPART_INVALID || part >= ZMPART_MAX ) part = ZMPART_ALLOWZM;

    return part;
}

void CZMPlayer::SetLocalParticipation( Participation_t part )
{
    zm_cl_participation.SetValue( (int)part );
}
#endif


Vector CZMPlayer::GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	if ( pWeapon )
		return pWeapon->GetBulletSpread( WEAPON_PROFICIENCY_PERFECT );
	
	return VECTOR_CONE_15DEGREES;
}

Vector CZMPlayer::GetAutoaimVector( float flScale )
{
    Vector fwd;
    AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle, &fwd );

    return fwd;
}

// Play normal footsteps instead of HL2DM ones.
void CZMPlayer::PlayStepSound( Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force )
{
    CBasePlayer::PlayStepSound( vecOrigin, psurface, fvol, force );
}

// Shared version of Weapon_GetWpnForAmmo.
CBaseCombatWeapon* CZMPlayer::GetWeaponForAmmo( int iAmmoType )
{
    for ( int i = 0; i < MAX_WEAPONS; i++ )
    {
        CBaseCombatWeapon* pWep = GetWeapon( i );

        if ( !pWep ) continue;


        if ( pWep->GetPrimaryAmmoType() == iAmmoType )
            return pWep;
    }
    
    return nullptr;
}

#ifndef CLIENT_DLL
void TE_PlayerAnimEvent( CBasePlayer* pPlayer, PlayerAnimEvent_t playerAnim, int nData );
#endif

void CZMPlayer::DoAnimationEvent( PlayerAnimEvent_t playerAnim, int nData )
{
#ifdef CLIENT_DLL
	if ( IsLocalPlayer() )
	{
		if ( ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() ) )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
#endif

    m_pPlayerAnimState->DoAnimationEvent( playerAnim, nData );

#ifndef CLIENT_DLL
    TE_PlayerAnimEvent( this, playerAnim, nData );
#endif
}

float CZMPlayer::GetAccuracyRatio() const
{
    return m_ZMLocal.m_flAccuracyRatio;
}

void CZMPlayer::UpdateAccuracyRatio()
{
    CZMBaseWeapon* pWep = static_cast<CZMBaseWeapon*>( GetActiveWeapon() );


    float cur = m_ZMLocal.m_flAccuracyRatio;

    float goalacc = 0.0f;

    if ( GetFlags() & FL_ONGROUND )
    {
        float maxspd = MAX( 1.0f, MaxSpeed() );

        float curspd = GetAbsVelocity().Length2D();


        if ( curspd < 0.1f )
            curspd = 0.0f;


        float spdratio = MIN( 1.0f, curspd / maxspd );

        goalacc = 1.0f - spdratio;
    }

    float min = 0.0f;
    float max = 1.0f;

    float add = 0.0f;
    if ( goalacc < cur )
    {
        float f = pWep ? pWep->GetAccuracyDecreaseRate() : 2.0f;

        add = -(f * gpGlobals->frametime);
        min = goalacc;
    }
    else if ( goalacc > cur )
    {
        float f = pWep ? pWep->GetAccuracyIncreaseRate() : 2.0f;

        add = (f * gpGlobals->frametime);
        max = goalacc;
    }

    m_ZMLocal.m_flAccuracyRatio = clamp( cur + add, min, max );
}

bool CZMPlayer::IsControllingZombie() const
{
    CZMBaseZombie* pZombie = ToZMBaseZombie( m_hObserverTarget.Get() );
    return pZombie != nullptr && pZombie->GetControllerIndex() == entindex();
}
