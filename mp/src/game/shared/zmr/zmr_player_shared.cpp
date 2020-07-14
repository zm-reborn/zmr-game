#include "cbase.h"

#include "ammodef.h"
#include "decals.h"
#include "rumble_shared.h"
#include "shot_manipulator.h"
#include "takedamageinfo.h"
#include "debugoverlay_shared.h"

#ifdef CLIENT_DLL
#include "prediction.h"
#include "c_te_effect_dispatch.h"
#else
#include "ilagcompensationmanager.h"
#include "iservervehicle.h"
//#include "vehicle_baseserver.h"
#include "te_effect_dispatch.h"
#endif


#include "zmr_ammodef.h"
#include "zmr_resource_system.h"
#include "zmr_player_shared.h"


#ifdef CLIENT_DLL
#include "c_zmr_player.h"
#include "npcs/c_zmr_zombiebase.h"
#else
#include "zmr_te_firebullets.h"
#include "zmr_player.h"
#include "npcs/zmr_zombiebase.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef CLIENT_DLL
//#define CZMPlayer C_ZMPlayer
#define CZMBaseZombie C_ZMBaseZombie


extern ConVar zm_cl_participation;
#endif


ConVar zm_sv_bulletspassplayers( "zm_sv_bulletspassplayers", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED, "Do bullets players shoot pass through other players?" );

ConVar zm_sv_bulletpenetration( "zm_sv_bulletpenetration", "2", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED, "0 = No penetration, 1 = Only zombies, 2 = Zombies and world" );

ConVar zm_sv_debug_penetration( "zm_sv_debug_penetration", "0", FCVAR_REPLICATED | FCVAR_CHEAT );


//
CZMPlayerAttackTraceFilter::CZMPlayerAttackTraceFilter( CBaseEntity* pAttacker, CBaseEntity* pIgnore, int collisionGroup ) : CTraceFilter()
{
    Assert( pAttacker != nullptr && pAttacker->IsPlayer() );
    m_pAttacker = pAttacker;
    
    if ( pIgnore )
    {
        AddToIgnoreList( pIgnore );
    }

    m_nPenetrations = 0;
}

bool CZMPlayerAttackTraceFilter::ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask )
{
    CBaseEntity* pEntity = EntityFromEntityHandle( pHandleEntity );

    if ( !pEntity ) return false;
    if ( pEntity == m_pAttacker ) return false;
    
    FOR_EACH_VEC( m_vIgnore, i )
    {
        if ( m_vIgnore[i] == pEntity )
        {
            return false;
        }
    }


    // It's a bone follower of the entity to ignore (toml 8/3/2007)
    if ( pEntity->GetOwnerEntity() == m_pAttacker )
        return false;

    if ( m_pAttacker->IsPlayer() )
    {
#ifdef GAME_DLL
        CBasePlayer* pPlayer = static_cast<CBasePlayer*>( m_pAttacker );

        // Clientside hit reg will do this for us.
        if ( !pPlayer->IsBot() && g_ZMUserCmdSystem.UsesClientsideDetection( pEntity ) )
        {
            ++m_nPenetrations;
            return false;
        }
#endif
        if (pEntity->IsPlayer()
        &&  pEntity->GetTeamNumber() == m_pAttacker->GetTeamNumber()
        &&  zm_sv_bulletspassplayers.GetBool())
            return false;
    }

    return true;
}

int CZMPlayerAttackTraceFilter::AddToIgnoreList( CBaseEntity* pIgnore )
{
    int index = m_vIgnore.Find( pIgnore );
    return ( index == -1 ) ? m_vIgnore.AddToTail( pIgnore ) : -1;
}

bool CZMPlayerAttackTraceFilter::RemoveFromIgnoreList( CBaseEntity* pIgnore )
{
    return m_vIgnore.FindAndRemove( pIgnore );
}

void CZMPlayerAttackTraceFilter::ClearIgnoreList()
{
    m_vIgnore.RemoveAll();
}
//


//
struct ZMFireBulletsInfo_t
{
    ZMFireBulletsInfo_t( const FireBulletsInfo_t& bulletinfo, Vector vecDir, CZMPlayerAttackTraceFilter* filter ) : info( bulletinfo ), Manipulator( vecDir ), pFilter( filter )
    {
        bStartedInWater = false;
        bDoImpacts = false;
        bDoTracers = false;
        flCumulativeDamage = 0.0f;
        this->vecDir = vecDir;

        nTraceHullFreq = 0;
        bDoTraceHull = false;
    }

    void ClearPerBulletData()
    {
        pFilter->ClearIgnoreList();
        pFilter->ClearPenetrations();

        bDoTracers = false;
        bDoTraceHull = false;
    }


    int iPlayerDamage;
    CZMBaseWeapon* pWeapon;

    bool bStartedInWater;
    bool bDoImpacts;
    bool bDoTracers;
    float flCumulativeDamage;

    const FireBulletsInfo_t& info;
    CShotManipulator Manipulator;
    CZMPlayerAttackTraceFilter* pFilter;
    Vector vecDir;

    int nTraceHullFreq;
    bool bDoTraceHull;
};
//


bool CZMPlayer::HasEnoughResToSpawn( ZombieClass_t zclass ) const
{
    return GetResources() >= CZMBaseZombie::GetCost( zclass );
}

bool CZMPlayer::HasEnoughRes( int cost ) const
{
    return GetResources() >= cost;
}

int CZMPlayer::GetWeaponSlotFlags() const
{
    return m_ZMLocal.m_fWeaponSlotFlags;
}

int CZMPlayer::GetResources() const
{
    return m_ZMLocal.m_nResources;
}

void CZMPlayer::IncResources( int res, bool bLimit )
{
    int oldres = GetResources();
    int newres = oldres + res;
    int max = g_ZMResourceSystem.GetResourceLimit();


    if ( bLimit && newres > max )
    {
        if ( oldres < max )
            newres = max;
        else
            return;
    }

    SetResources( newres );
}

void CZMPlayer::SetResources( int res )
{
    if ( res < 0 ) res = 0;

    m_ZMLocal.m_nResources = res;
}

float CZMPlayer::GetFlashlightBattery() const
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

Participation_t CZMPlayer::GetParticipation() const
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
    QAngle angBase;
    auto* pCmd = GetCurrentUserCommand();

    if ( pCmd )
    {
        angBase = pCmd->aimangles;
    }
    else
    {
        angBase = GetAbsAngles();
    }

    AngleVectors( angBase + m_Local.m_vecPunchAngle, &fwd );

    return fwd;
}

// Play normal footsteps instead of HL2DM ones.
void CZMPlayer::PlayStepSound( Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force )
{
    CBasePlayer::PlayStepSound( vecOrigin, psurface, fvol, force );
}

// Shared version of Weapon_GetWpnForAmmo.
CBaseCombatWeapon* CZMPlayer::GetWeaponForAmmo( int iAmmoType ) const
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

#ifdef CLIENT_DLL
extern ConVar zm_cl_zmmovespeed;
extern ConVar zm_cl_zmmoveaccelerate;
extern ConVar zm_cl_zmmovedecelerate;
#endif

void CZMPlayer::GetZMMovementVars( float& maxspd, float& accel, float& decel ) const
{
#ifdef CLIENT_DLL
    //if ( !IsLocalPlayer() )
    //{
    //    return;
    //}


    maxspd = zm_cl_zmmovespeed.GetFloat();
    accel = zm_cl_zmmoveaccelerate.GetFloat();
    decel = zm_cl_zmmovedecelerate.GetFloat();
#else
    maxspd = m_flZMMoveSpeed;
    accel = m_flZMMoveAccel;
    decel = m_flZMMoveDecel;
#endif
}


#ifdef _DEBUG
static ConVar zm_sv_debugbullets( "zm_sv_debugbullets", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "" );
static ConVar zm_sv_debugbullets_time( "zm_sv_debugbullets_time", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "" );
#endif // _DEBUG



// Override FireBullets
// This is no longer called recursively.
void CZMPlayer::FireBullets( const FireBulletsInfo_t& info )
{
    Assert( info.m_flDamage > 0.0f || info.m_iPlayerDamage > 0 );

    AssertMsg( info.m_iAmmoType != -1, "Can't shoot invalid ammo type!" );



#ifdef GAME_DLL
    lagcompensation->StartLagCompensation( this, GetCurrentCommand() );
    NoteWeaponFired();
#endif

    static int  tracerCount = 0;
    CAmmoDef*   pAmmoDef    = GetAmmoDef();
    int         nDamageType = pAmmoDef->DamageType( info.m_iAmmoType );
    int         nAmmoFlags  = pAmmoDef->Flags( info.m_iAmmoType );
    
    auto*       pWeapon = static_cast<CZMBaseWeapon*>( GetActiveWeapon() );


#if defined( GAME_DLL )
    if ( IsPlayer() )
    {
        CBasePlayer* pPlayer = this;

        int rumbleEffect = pPlayer->GetActiveWeapon()->GetRumbleEffect();

        if ( rumbleEffect != RUMBLE_INVALID )
        {
            pPlayer->RumbleEffect( rumbleEffect, 0, RUMBLE_FLAG_RESTART );
        }
    }
#endif// GAME_DLL

    int iPlayerDamage = info.m_iPlayerDamage;
    if ( iPlayerDamage == 0 )
    {
        if ( nAmmoFlags & AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER )
        {
            iPlayerDamage = pAmmoDef->PlrDamage( info.m_iAmmoType );
        }
    }


    // Make sure we don't have a dangling damage target from a recursive call
    if ( g_MultiDamage.GetTarget() )
    {
        Warning( "Clearing dangling damage target %i!\n", g_MultiDamage.GetTarget()->entindex() );
        ApplyMultiDamage();
    }
      
    ClearMultiDamage();
    g_MultiDamage.SetDamageType( nDamageType | DMG_NEVERGIB );


    

    // ZMR
    CZMPlayerAttackTraceFilter traceFilter( this, info.m_pAdditionalIgnoreEnt, COLLISION_GROUP_NONE );

    ZMFireBulletsInfo_t bulletinfo( info, info.m_vecDirShooting, &traceFilter );
    bulletinfo.iPlayerDamage = iPlayerDamage;
    bulletinfo.pWeapon = pWeapon;


    bulletinfo.bStartedInWater = ( enginetrace->GetPointContents( info.m_vecSrc ) & (CONTENTS_WATER|CONTENTS_SLIME) ) != 0;

    //bool bUnderwaterBullets = ShouldDrawUnderwaterBulletBubbles();


    

    

    // Prediction is only usable on players
    int iSeed = CBaseEntity::GetPredictionRandomSeed( info.m_bUseServerRandomSeed ) & 255;


#if defined( GAME_DLL )
    int iEffectSeed = iSeed;
#endif

    // If we have more than a few shots, do hull traces.
    // You know, it's a shotgun, that shoots pellets, right.
    // Don't allow hull tracing when using penetrations.
    bulletinfo.nTraceHullFreq = (bulletinfo.pWeapon->GetMaxPenetrations() == 0 && info.m_iShots > 1) ? 2 : 0;


    for ( int iShot = 0; iShot < info.m_iShots; iShot++ )
    {
        RandomSeed( iSeed ); // Init random system with this seed


        // If we're firing multiple shots, and the first shot has to be bang on target, ignore spread
        if ( !iShot && info.m_iShots > 1 && (info.m_nFlags & FIRE_BULLETS_FIRST_SHOT_ACCURATE) )
        {
            bulletinfo.vecDir = bulletinfo.Manipulator.GetShotDirection();
        }
        else
        {
            // Don't run the biasing code for the player at the moment.
            bulletinfo.vecDir = bulletinfo.Manipulator.ApplySpread( info.m_vecSpread );
        }


        if ( info.m_iTracerFreq && ( tracerCount++ % info.m_iTracerFreq ) == 0 /*&& !bHitGlass*/ )
        {
            bulletinfo.bDoTracers = true;
        }

        if ( bulletinfo.nTraceHullFreq && (iShot % bulletinfo.nTraceHullFreq) == 0 )
        {
            bulletinfo.bDoTraceHull = true;
        }

        // Do the actual shooting
        SimulateBullet( bulletinfo );

        bulletinfo.ClearPerBulletData();

        iSeed++;
    }

#ifdef GAME_DLL
    TE_ZMFireBullets( entindex(), info.m_vecSrc, info.m_vecDirShooting, info.m_iAmmoType, iEffectSeed, info.m_iShots, info.m_vecSpread.x, bulletinfo.bDoTracers, bulletinfo.bDoImpacts );
#endif

#ifdef GAME_DLL
    ApplyMultiDamage();

#if 0 // ZMR: This was causing crashes. Commenting it out since we're not using stats.
    if ( IsPlayer() && flCumulativeDamage > 0.0f )
    {
        CBasePlayer *pPlayer = static_cast< CBasePlayer * >( this );
        CTakeDamageInfo dmgInfo( this, pAttacker, flCumulativeDamage, nDamageType );
        gamestats->Event_WeaponHit( pPlayer, info.m_bPrimaryAttack, pPlayer->GetActiveWeapon()->GetClassname(), dmgInfo );
    }
#endif

    // Move ents back to their original positions.
    lagcompensation->FinishLagCompensation( this );
#endif
}

void CZMPlayer::SimulateBullet( ZMFireBulletsInfo_t& bulletinfo )
{
    trace_t tr;
    Vector vecEnd;

    auto&   info = bulletinfo.info;
    int     iPlayerDamage = bulletinfo.iPlayerDamage;
    bool    bStartedInWater = bulletinfo.bStartedInWater;
    auto*   pFilter = bulletinfo.pFilter;


    CBaseEntity*    pAttacker = info.m_pAttacker ? info.m_pAttacker : this;
    float           flDistanceLeft = 8192.0f;
    float           flDamageFallOffStart = MAX( 1.0f, info.m_flDistance );
    Vector          vecSrc = info.m_vecSrc;
    Vector          vecDir = bulletinfo.vecDir;
    Vector          vecFirstStart = vec3_origin;
    Vector          vecFirstEnd = vecSrc + vecDir * flDistanceLeft;
    int             nPenetrations = 0;
    const Vector    vecShotHull = Vector( 3, 3, 3 );


    CAmmoDef*       pAmmoDef = GetAmmoDef();
    int             nDamageType = pAmmoDef->DamageType( info.m_iAmmoType );
#ifdef GAME_DLL
    int             nAmmoFlags = pAmmoDef->Flags( info.m_iAmmoType );
#endif

#ifdef CLIENT_DLL
    bool bDoEffects = IsLocalPlayer();
#endif

    bool bHitWater = bStartedInWater;
    //bool bHitGlass = false;

    bool bFirstValidShot = true;


    int iSanityCheck = 0;

    //
    // Keep going till we can't penetrate anymore.
    // The reason why we don't do it the old way where we recursively call FireBullets,
    // is because we want to keep track of how many times we penetrated.
    //
    while ( flDistanceLeft > 0.0f )
    {
        if ( ++iSanityCheck > 16 )
        {
            Warning( "CZMPlayer::SimulateBullet failed sanity check!!\n" );
            break;
        }


        vecEnd = vecSrc + vecDir * flDistanceLeft;

        //
        // Do the trace
        //
        if ( bulletinfo.bDoTraceHull )
        {
            UTIL_TraceHull( vecSrc,
                            vecEnd,
                            -vecShotHull,
                            vecShotHull,
                            MASK_SHOT,
                            pFilter,
                            &tr );
        }
        else
        {
            UTIL_TraceLine( vecSrc, vecEnd, MASK_SHOT, pFilter, &tr );
        }

        if ( !tr.startsolid )
        {
            flDistanceLeft *= (1.0f - tr.fraction);
        }


        //
        // Build the damage info
        //
        float flActualDamage = info.m_flDamage;
        int nActualDamageType = nDamageType;


        if ( tr.m_pEnt )
        {
            // If we hit a player, and we have player damage specified, use that instead
            // Adrian: Make sure to use the currect value if we hit a vehicle the player is currently driving.
            if ( iPlayerDamage && tr.m_pEnt->IsPlayer() )
            {
                flActualDamage = iPlayerDamage;
            }

            if ( flActualDamage == 0.0f )
            {
                flActualDamage = g_pGameRules->GetAmmoDamage( pAttacker, tr.m_pEnt, info.m_iAmmoType );
            }
#if 0 // ZMR: Never gib us from bullets...
            else
            {
                nActualDamageType = nDamageType | ((flActualDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB );
            }
#endif
        }


        CTakeDamageInfo dmgInfo( this, pAttacker, flActualDamage, nActualDamageType );

        // Scale damage based on the distance
        // ZMRTODO: Add a param to config to change fall-off behavior.
        float flDistFromSrc = vecSrc.DistTo( tr.endpos );
        if ( flDistFromSrc > flDamageFallOffStart )
        {
            // The distance we'll consider
            float falloffDist = flDamageFallOffStart * 2.0f;
            
            float dist = (flDistFromSrc - flDamageFallOffStart);
            dist = MIN( dist, falloffDist );

            float mult = 1.0f - (dist / falloffDist);
            mult = clamp( mult, 0.0f, 1.0f );
            mult *= mult;

            DevMsg( "Bullet wen't over weapon range of %.1f! Scaling damage by %.1f!\n",
                flDamageFallOffStart,
                mult );

            dmgInfo.ScaleDamage( mult );
            if ( dmgInfo.GetDamage() < 1.0f )
                dmgInfo.SetDamage( 1.0f );
        }

        ModifyFireBulletsDamage( &dmgInfo );
        CalculateBulletDamageForce( &dmgInfo, info.m_iAmmoType, vecDir, tr.endpos );
        dmgInfo.ScaleDamageForce( info.m_flDamageForceScale );
        dmgInfo.SetAmmoType( info.m_iAmmoType );

        
#ifdef GAME_DLL
        // Hit all triggers along the ray
        TraceAttackToTriggers( dmgInfo, tr.startpos, tr.endpos, vecDir );
#endif


        //
        // Alright, we're done with the bullet if we didn't actually hit anything.
        //
        if ( tr.fraction == 1.0f )
        {
            break;
        }

        // Is this even possible?
        if ( !tr.m_pEnt )
        {
            Assert( 0 );
            break;
        }


        //
        // Do damage, paint decals
        //


        if ( bFirstValidShot && !tr.startsolid )
        {
            vecFirstStart = tr.startpos;
            vecFirstEnd = tr.endpos;

            bFirstValidShot = false;
        }


        // No point in alerting zombies from bullet impact sounds.
//#ifdef GAME_DLL
//        //UpdateShotStatistics( tr );
//
//        // For shots that don't need persistance
//        int soundEntChannel = ( info.m_nFlags & FIRE_BULLETS_TEMPORARY_DANGER_SOUND ) ? SOUNDENT_CHANNEL_BULLET_IMPACT : SOUNDENT_CHANNEL_UNSPECIFIED;
//
//        CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, tr.endpos, 200, 0.5, this, soundEntChannel );
//#endif



        // See if the bullet ended up underwater + started out of the water
        if ( !bHitWater && ( enginetrace->GetPointContents( tr.endpos ) & (CONTENTS_WATER|CONTENTS_SLIME) ) )
        {
            bHitWater = HandleShotImpactingWater( info, vecEnd, pFilter );
        }


        if ( !bHitWater || ((info.m_nFlags & FIRE_BULLETS_DONT_HIT_UNDERWATER) == 0) )
        {
            // Damage specified by function parameter
            tr.m_pEnt->DispatchTraceAttack( dmgInfo, vecDir, &tr );
            
            if ( ToBaseCombatCharacter( tr.m_pEnt ) )
            {
                bulletinfo.flCumulativeDamage += dmgInfo.GetDamage();
            }

            if ( bStartedInWater || !bHitWater || (info.m_nFlags & FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS) )
            {
                bulletinfo.bDoImpacts = true;
            }
            else
            {
                // We may not impact, but we DO need to affect ragdolls on the client
                CEffectData data;
                data.m_vStart = tr.startpos;
                data.m_vOrigin = tr.endpos;
                data.m_nDamageType = nDamageType;
                    
                DispatchEffect( "RagdollImpact", data );
            }
    
#ifdef GAME_DLL
            if ( nAmmoFlags & AMMO_FORCE_DROP_IF_CARRIED )
            {
                // Make sure if the player is holding this, he drops it
                Pickup_ForcePlayerToDropThisObject( tr.m_pEnt );		
            }
#endif
        }

        
        //
        // We hit something, attempt penetration.
        //
        if (nPenetrations < bulletinfo.pWeapon->GetMaxPenetrations()
        &&  HandleBulletPenetration( tr, bulletinfo, vecSrc, flDistanceLeft ) )
        {
            ++nPenetrations;
        }
        else
        {
            // Can't go any further than this.
            flDistanceLeft = 0.0f;
        }


#ifdef CLIENT_DLL
        if ( bDoEffects )
        {
            DoImpactEffect( tr, nDamageType );
        }
#endif


        //
        // Draw debug shit
        //
#if defined( _DEBUG )
        if ( zm_sv_debugbullets.GetBool() )
        {
#ifdef CLIENT_DLL
            const int dbgclr[4] = { 255, 0, 0, 255 };
#else
            const int dbgclr[4] = { 0, 0, 255, 255 };
#endif // CLIENT_DLL
            float t = abs( zm_sv_debugbullets_time.GetFloat() );

            if ( bulletinfo.bDoTraceHull )
            {
                NDebugOverlay::SweptBox(
                    tr.startpos, tr.endpos,
                    -vecShotHull, vecShotHull,
                    vec3_angle,
                    dbgclr[0], dbgclr[1], dbgclr[2], dbgclr[3],
                    t );
            }
            else
            {
                DebugDrawLine(
                    tr.startpos, tr.endpos,
                    dbgclr[0], dbgclr[1], dbgclr[2],
                    false, t );
            }
            
        }
#endif // ZMR - DEBUG
    }


#ifdef CLIENT_DLL
    if ( bDoEffects )
    {
        tr.endpos = vecFirstEnd;
        MakeTracer( vecFirstStart, tr, pAmmoDef->TracerType( info.m_iAmmoType ) );
    }
#endif
}

bool CZMPlayer::HandleBulletPenetration( trace_t& tr, const ZMFireBulletsInfo_t& bulletinfo, Vector& vecNextSrc, float& flDistance )
{
    if ( !zm_sv_bulletpenetration.GetBool() )
        return false;


    // Make sure we are inside/outside the volume by nudging the trace forward/back.
    const float epsilon = 0.1f;


    if ( tr.fraction == 1.0f )
        return false;

    if ( tr.allsolid )
        return false;


    float flSurfaceMult = 1.0f;

    bool bPenetrate = tr.startsolid;



    bool bIsCharacter = ToBaseCombatCharacter( tr.m_pEnt ) != nullptr;

    if ( !bPenetrate && tr.m_pEnt )
    {
        //
        // Only penetrate through the world
        // There may be maps that rely on things NOT being penetrable
        //
        if ( tr.m_pEnt->IsWorld() && zm_sv_bulletpenetration.GetInt() >= 2 )
        {
            if ( tr.surface.flags & (SURF_SKY|SURF_SKY2D|SURF_NODRAW) )
            {
                return false;
            }
        
            auto* pSurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
            if ( pSurf )
            {
                switch ( pSurf->game.material )
                {
                case CHAR_TEX_CLIP : flSurfaceMult = 0.0f; break;
                case CHAR_TEX_VENT :
                case CHAR_TEX_METAL : flSurfaceMult = 0.25f; break;
                case CHAR_TEX_DIRT :
                case CHAR_TEX_CONCRETE : flSurfaceMult = 0.3f; break;
                case CHAR_TEX_WOOD : flSurfaceMult = 0.9f; break;
                default : break;
                }
                // Query the func_breakable for whether it wants to allow for bullet penetration
                //if ( !tr.m_pEnt->HasSpawnFlags( SF_BREAK_NO_BULLET_PENETRATION ) )
            }

            bPenetrate = true;
        }
        else
        {
            bPenetrate = bIsCharacter;
        }
    }



    const float flMaxPenetration =
        bulletinfo.pWeapon->GetMaxPenetrationDist() * flSurfaceMult;



    if ( !bPenetrate )
    {
        return false;
    }


    //
    // If they're a character, we can't use fractionleftsolid
    // due to it not working with hitboxes.
    // Instead, we'll just have to simply ignore it.
    //
    if ( bIsCharacter )
    {
        auto* pZombie = ToZMBaseZombie( tr.m_pEnt );

        if ( pZombie && pZombie->CanBePenetrated() )
        {
            auto dir = tr.endpos - tr.startpos;
            dir.NormalizeInPlace();

            vecNextSrc = tr.endpos + dir * epsilon;

            bulletinfo.pFilter->AddToIgnoreList( tr.m_pEnt );
            return true;
        }

        return false;
    }



    trace_t peneTrace;


    auto dir = tr.endpos - tr.startpos;
    dir.NormalizeInPlace();


    float distToTrace = MIN( flDistance, flMaxPenetration );
    
    auto start = tr.endpos + dir * epsilon;


    if ( tr.startsolid )
    {
        peneTrace = tr;
    }
    else
    {
        auto end = start + dir * distToTrace;

        UTIL_TraceLine( start, end, MASK_SHOT, bulletinfo.pFilter, &peneTrace );
    }


    if ( !peneTrace.startsolid )
    {
        Assert( 0 );
        return false;
    }



    float frac = peneTrace.fractionleftsolid;

    if ( frac == 0.0f )
    {
        Assert( peneTrace.allsolid );
        return false;
    }

    // We never left solid!
    if ( frac == 1.0f || tr.allsolid )
    {
        return false;
    }



    float distInSolid = distToTrace * frac;

    bool bPasses = distInSolid < flMaxPenetration;

    if ( zm_sv_debug_penetration.GetFloat() > 0.0f )
    {
        DebugDrawLine(
            peneTrace.startpos,
            peneTrace.startpos + dir * distInSolid,
            bPasses ? 0 : 255,
            (!bPasses) ? 0 : 255,
            0,
            true,
            zm_sv_debug_penetration.GetFloat() );
    }

    if ( !bPasses )
    {
        return false;
    }


    vecNextSrc = start + (dir * distInSolid) + (dir * epsilon);

    flDistance -= distInSolid;


    return true;
}

bool CZMPlayer::HandleShotImpactingWater( const FireBulletsInfo_t& info, const Vector& vecEnd, CTraceFilter* pFilter )
{
    trace_t	tr;

    // Trace again with water enabled
    UTIL_TraceLine( info.m_vecSrc, vecEnd, (MASK_SHOT|CONTENTS_WATER|CONTENTS_SLIME), pFilter, &tr );
    

    // See if this is the point we entered
    if ( ( enginetrace->GetPointContents( tr.endpos - Vector( 0, 0, 0.1f ) ) & (CONTENTS_WATER|CONTENTS_SLIME) ) == 0 )
        return false;


    int	nMinSplashSize = GetAmmoDef()->MinSplashSize(info.m_iAmmoType);
    int	nMaxSplashSize = GetAmmoDef()->MaxSplashSize(info.m_iAmmoType);

    CEffectData	data;
    data.m_vOrigin = tr.endpos;
    data.m_vNormal = tr.plane.normal;
    data.m_flScale = random->RandomFloat( nMinSplashSize, nMaxSplashSize );
    if ( tr.contents & CONTENTS_SLIME )
    {
        data.m_fFlags |= FX_WATER_IN_SLIME;
    }

    DispatchEffect( "gunshotsplash", data );

    return true;
}

void CZMPlayer::DoMuzzleFlash()
{
    // By default, the player muzzleflash will call into active weapon muzzleflash.
    // We don't want that, because it might not be networked.
    CBaseAnimating::DoMuzzleFlash();
}

int CZMPlayer::GetTotalAmmoAmount( int iValidAmmoIndex ) const
{
    Assert( iValidAmmoIndex >= 0 && iValidAmmoIndex <= MAX_AMMO_TYPES );
    
    int total = GetAmmoCount( iValidAmmoIndex );

    for ( int i = 0; i < MAX_WEAPONS; i++ )
    {
        auto* pWep = GetWeapon( i );
        if ( pWep && pWep->m_iPrimaryAmmoType == iValidAmmoIndex )
        {
            total += pWep->Clip1();
        }
    }

    return total;
}

int CZMPlayer::GetAmmoRoom( int iValidAmmoIndex ) const
{
    Assert( iValidAmmoIndex >= 0 && iValidAmmoIndex <= MAX_AMMO_TYPES );

    auto* pAmmoDef = ZMAmmoDef();
    return pAmmoDef->MaxCarry( iValidAmmoIndex ) + pAmmoDef->m_Additional[iValidAmmoIndex].nDropAmount;
}

bool CZMPlayer::IsFlashlightOn() const
{
    return IsEffectActive( EF_DIMLIGHT );
}
