#include "cbase.h"
#include "ammodef.h"
#ifdef CLIENT_DLL
#include "prediction.h"
#include "weapon_selection.h"
#include "view.h"
#endif
#include "datacache/imdlcache.h"
#include "eventlist.h"
#include "animation.h"
#include "in_buttons.h"

#include <vphysics/constraints.h>


#include "zmr_gamerules.h"
#include "zmr_viewmodel.h"
#include "zmr_ammodef.h"
#include "weapons/zmr_base.h"
#ifdef CLIENT_DLL
#include "zmr_thirdpersonmanager.h"
#endif
#include "zmr_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace ZMWeaponConfig;


#ifdef CLIENT_DLL
void UTIL_ClipPunchAngleOffset( QAngle &in, const QAngle &punch, const QAngle &clip )
{
	QAngle result = in + punch;

	//Clip each component
	for ( int i = 0; i < 3; i++ )
	{
		if ( result[i] > clip[i] )
		{
			result[i] = clip[i];
		}
		else if ( result[i] < -clip[i] )
		{
			result[i] = -clip[i];
		}

		// Return the result
		in[i] = result[i] - punch[i];
	}
}


ConVar zm_cl_glow_weapon( "zm_cl_glow_weapon", "1 .2 .2", FCVAR_ARCHIVE );
ConVar zm_cl_glow_weapon_enabled( "zm_cl_glow_weapon_enabled", "1", FCVAR_ARCHIVE );

extern ConVar zm_sv_glow_item_enabled;
#endif


#ifndef CLIENT_DLL
static ConVar zm_sv_weaponreserveammo( "zm_sv_weaponreserveammo", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE, "When player drops their weapon, their ammo gets dropped with the weapon as well." );
#endif

ConVar zm_sv_bulletsusemainview( "zm_sv_bulletsusemainview", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "When on, clients will use their current view's origin and angles instead of networked ones to determine the source of bullets." );

ConVar zm_sv_debug_weapons( "zm_sv_debug_weapons", "0", FCVAR_REPLICATED );




BEGIN_NETWORK_TABLE( CZMBaseWeapon, DT_ZM_BaseWeapon )
#ifdef CLIENT_DLL
    RecvPropInt( RECVINFO( m_nOverrideClip1 ) ),
    RecvPropTime( RECVINFO( m_flNextClipFillTime ) ),
    RecvPropBool( RECVINFO( m_bCanCancelReload ) ),
    RecvPropString( RECVINFO( m_sScriptFileName ) ),

    RecvPropBool( RECVINFO( m_bInReload2 ) ),
#else
    SendPropInt( SENDINFO( m_nOverrideClip1 ) ),
    SendPropTime( SENDINFO( m_flNextClipFillTime ) ),
    SendPropBool( SENDINFO( m_bCanCancelReload ) ),
    SendPropStringT( SENDINFO( m_sScriptFileName ) ),

    SendPropExclude( "DT_LocalWeaponData", "m_iClip2" ),
    SendPropExclude( "DT_LocalWeaponData", "m_iSecondaryAmmoType" ),

    SendPropBool( SENDINFO( m_bInReload2 ) ),
#endif
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( ZMBaseWeapon, DT_ZM_BaseWeapon )

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMBaseWeapon )
    DEFINE_PRED_FIELD_TOL( m_flNextClipFillTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
    DEFINE_PRED_FIELD( m_bCanCancelReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
    DEFINE_PRED_FIELD( m_bInReload2, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

#ifndef CLIENT_DLL
BEGIN_DATADESC( CZMBaseWeapon )
    DEFINE_KEYFIELD( m_OverrideViewModel, FIELD_MODELNAME, "v_modeloverride" ),
    DEFINE_KEYFIELD( m_OverrideWorldModel, FIELD_MODELNAME, "w_modeloverride" ),

    DEFINE_KEYFIELD( m_nOverrideDamage, FIELD_INTEGER, "dmgoverride" ),
    DEFINE_KEYFIELD( m_nOverrideClip1, FIELD_INTEGER, "clip1override" ),

    DEFINE_KEYFIELD( m_sScriptFileName, FIELD_STRING, "customscriptfile" ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( weapon_zm_base, CZMBaseWeapon );
#endif

CZMBaseWeapon::CZMBaseWeapon()
{
    // Enable base class stuff by default since we'll be checking these ourselves.
    m_bAltFiresUnderwater = m_bFiresUnderwater = true;



	SetPredictionEligible( true );
	AddSolidFlags( FSOLID_TRIGGER ); // Nothing collides with these but it gets touches.


    SetConfigSlot( ZMCONFIGSLOT_INVALID );
    SetSlotFlag( ZMWEAPONSLOT_NONE );

#ifndef CLIENT_DLL
    m_OverrideViewModel = m_OverrideWorldModel = NULL_STRING;
    m_nOverrideDamage = -1;
    m_nOverrideClip1 = -1;

    m_pConstraint = nullptr;
#endif

    m_flNextClipFillTime = 0.0f;
    m_bCanCancelReload = true;
    m_bInReload2 = false;

#ifdef GAME_DLL
    m_sScriptFileName = MAKE_STRING( "" );
#else
    m_sScriptFileName[0] = NULL;
#endif
}

CZMBaseWeapon::~CZMBaseWeapon()
{
#ifndef CLIENT_DLL
    FreeWeaponSlot();

    SetReserveAmmo( 0 );

    ReleaseConstraint();
#endif
}

bool CZMBaseWeapon::IsDebugging()
{
    return zm_sv_debug_weapons.GetBool();
}

#ifndef CLIENT_DLL
void CZMBaseWeapon::FreeWeaponSlot()
{
    if ( GetSlotFlag() == ZMWEAPONSLOT_NOLIMIT )
        return;

    

    CZMPlayer* pPlayer = GetPlayerOwner();

    if ( !pPlayer ) return;


    pPlayer->RemoveWeaponSlotFlag( GetSlotFlag() );
}

const char* CZMBaseWeapon::GetDropAmmoName() const
{
    Assert( m_iPrimaryAmmoType != -1 );

    auto* pAmmoDef = ZMAmmoDef();

    return pAmmoDef->m_Additional[m_iPrimaryAmmoType].pszItemName;
}

int CZMBaseWeapon::GetDropAmmoAmount() const
{
    auto* pAmmoDef = ZMAmmoDef();

    return pAmmoDef->m_Additional[m_iPrimaryAmmoType].nDropAmount;
}
#endif

void CZMBaseWeapon::ItemPostFrame()
{
    //
    // Override CBaseCombatWeapon::ItemPostFrame
    //
    auto* pOwner = GetPlayerOwner();
    if ( !pOwner )
        return;


    // This sets the m_nButtons on player.
    UpdateAutoFire();


    const int buttons = pOwner->m_nButtons;
    bool bHoldingAttack = buttons & IN_ATTACK;
    bool bHoldingAttack2 = buttons & IN_ATTACK2;
    bool bHoldingReload = buttons & IN_RELOAD;

    

    // Track the duration of the fire
    m_fFireDuration = bHoldingAttack ? ( m_fFireDuration + gpGlobals->frametime ) : 0.0f;


    //
    // Handle reloading
    //
    if ( UsesClipsForAmmo1() )
    {
        CheckReload();
    }

    //
    // Secondary attack
    // has priority over primary
    //
    if ( bHoldingAttack2 && CanPerformSecondaryAttack() )
    {
        SecondaryAttack();
    }

    //
    // Primary attack
    //
    if ( bHoldingAttack && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
    {
        PrimaryAttack();
    }

    //
    // Reloading
    //
    bool bAutoReload = UsesClipsForAmmo1() && Clip1() == 0 && pOwner->GetAmmoCount( GetPrimaryAmmoType() ) > 0;

    if ( (bHoldingReload || bAutoReload) && m_flNextPrimaryAttack <= gpGlobals->curtime && UsesClipsForAmmo1() && !m_bInReload2 ) 
    {
        Reload();
        m_fFireDuration = 0.0f;
    }


    //
    // Check weapon idle anims.
    //
    WeaponIdle();
}

Activity CZMBaseWeapon::GetPrimaryAttackActivity()
{
    auto* pOwner = GetPlayerOwner();
    if ( !pOwner )
        return ACT_VM_PRIMARYATTACK;

    auto* pVM = pOwner->GetViewModel( m_nViewModelIndex );

    // If we have last bullet, play fitting animation if we have it.
    bool bUseLastAct =  !IsMeleeWeapon()
                    &&  m_iClip1 == 1
                    &&  ::SelectHeaviestSequence( pVM->GetModelPtr(), ACT_VM_PRIMARYATTACK_EMPTY ) != ACTIVITY_NOT_AVAILABLE;

    return bUseLastAct ? ACT_VM_PRIMARYATTACK_EMPTY : ACT_VM_PRIMARYATTACK;
}

bool CZMBaseWeapon::Deploy()
{
    MDLCACHE_CRITICAL_SECTION();
    bool bResult = DefaultDeploy();


    PoseParameterOverride( false );

    return bResult;
}

bool CZMBaseWeapon::DefaultDeploy()
{
    // Weapons that don't autoswitch away when they run out of ammo 
    // can still be deployed when they have no ammo.
    if ( !HasAnyAmmo() && AllowsAutoSwitchFrom() )
        return false;

    auto* pOwner = GetPlayerOwner();
    if ( pOwner )
    {
        if ( !pOwner->IsAlive() )
            return false;


        pOwner->SetAnimationExtension( GetAnimPrefix() );

        SetViewModel();
        SendWeaponAnim( GetDrawActivity() );

        pOwner->SetNextAttack( gpGlobals->curtime + SequenceDuration() );
    }


    // Can't shoot again until we've finished deploying
    m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
    m_flNextSecondaryAttack	= m_flNextPrimaryAttack;

    //m_flHudHintMinDisplayTime = 0;

    //m_bAltFireHudHintDisplayed = false;
    //m_bReloadHudHintDisplayed = false;
    //m_flHudHintPollTime = gpGlobals->curtime + 5.0f;
    
    WeaponSound( DEPLOY );

    SetWeaponVisible( true );

    SetContextThink( nullptr, 0, "BaseCombatWeapon_HideThink" );

    return true;
}

bool CZMBaseWeapon::Holster( CBaseCombatWeapon* pSwitchingTo )
{
    // Reset reloading
    m_bInReload2 = false;
    m_bFiringWholeClip = false;

    m_flNextClipFillTime = 0.0f;
    m_bCanCancelReload = false;


    SetThink( nullptr );

    SetWeaponVisible( false );

    PoseParameterOverride( true );

    return true;
}

//Activity CZMBaseWeapon::GetSecondaryAttackActivity()
//{
//
//}

Activity CZMBaseWeapon::GetDrawActivity()
{
    return UsesDryActivity( ACT_VM_DRAW_EMPTY ) ? ACT_VM_DRAW_EMPTY : ACT_VM_DRAW;
}

Activity CZMBaseWeapon::GetIdleActivity() const
{
    auto* pMe = const_cast<CZMBaseWeapon*>( this );
    return pMe->UsesDryActivity( ACT_VM_IDLE_EMPTY ) ? ACT_VM_IDLE_EMPTY : ACT_VM_IDLE;
}

bool CZMBaseWeapon::UsesDryActivity( Activity act )
{
    auto* pOwner = GetPlayerOwner();
    if ( !pOwner )
        return false;

    auto* pVM = pOwner->GetViewModel( m_nViewModelIndex );


    return  !IsMeleeWeapon()
        &&  m_iClip1 == 0
        &&  ::SelectHeaviestSequence( pVM->GetModelPtr(), act ) != ACTIVITY_NOT_AVAILABLE; // Do we have that activity?
}

void CZMBaseWeapon::WeaponIdle()
{
    if ( HasWeaponIdleTimeElapsed() )
    {
        // Don't keep updating our activity as the idle should always loop.
        // Resetting the idle animation manually can look bad when
        // taking into account pose parameter anims.
        auto curAct = GetActivity();
        auto wantedAct = GetIdleActivity();

        if ( curAct != wantedAct )
            SendWeaponAnim( wantedAct );
    }
}

bool CZMBaseWeapon::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
    auto* pOwner = GetPlayerOwner();
    if ( !pOwner )
        return false;

    // If I don't have any spare ammo, I can't reload
    if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
    {
        if ( Clip1() == 0 )
        {
            //
            // Play empty sound
            //
            if ( pOwner->m_nButtons & (IN_ATTACK|IN_ATTACK2) && m_flNextEmptySoundTime <= gpGlobals->curtime )
            {
                WeaponSound( EMPTY );
                m_flNextEmptySoundTime = gpGlobals->curtime + 1.0f;
            }
        }

        return false;
    }

    bool bReload = false;

    // If you don't have clips, then don't try to reload them.
    if ( UsesClipsForAmmo1() )
    {
        // need to reload primary clip?
        int primary	= MIN(iClipSize1 - m_iClip1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));
        if ( primary != 0 )
        {
            bReload = true;
        }
    }

    if ( UsesClipsForAmmo2() )
    {
        // need to reload secondary clip?
        int secondary = MIN(iClipSize2 - m_iClip2, pOwner->GetAmmoCount(m_iSecondaryAmmoType));
        if ( secondary != 0 )
        {
            bReload = true;
        }
    }

    if ( !bReload )
        return false;

#ifdef CLIENT_DLL
    // Play reload
    WeaponSound( RELOAD );
#endif
    SendWeaponAnim( iActivity );


    MDLCACHE_CRITICAL_SECTION();

    float flConfigReloadTime = GetReloadTime();
    //pOwner->SetNextAttack( flSequenceEndTime );

    float flReloadTime = GetFirstInstanceOfAnimEventTime( GetSequence(), (int)AE_WPN_INCREMENTAMMO );
    if ( flReloadTime == -1.0f )
        flReloadTime = flConfigReloadTime;


    float flReadyTime = GetFirstInstanceOfAnimEventTime( GetSequence(), (int)AE_WPN_PRIMARYATTACK );
    if ( flReadyTime == -1.0f )
        flReadyTime = flConfigReloadTime;
    

    m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flReadyTime;


    m_flNextClipFillTime = gpGlobals->curtime + flReloadTime;

    m_bInReload2 = true;
    m_bCanCancelReload = true;

    return true;
}

void CZMBaseWeapon::CheckReload()
{
    if ( m_bInReload2 )
    {
        //auto* pOwner = GetPlayerOwner();
        if ( ShouldIncrementClip() )
        {
            IncrementClip();
        }

        if ( ShouldCancelReload() )
        {
            CancelReload();

            return;
        }

        if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
        {
            if ( m_bReloadsSingly && m_iClip1 < GetMaxClip1() )
            {
                Reload();
            }
            else
            {
                StopReload();
            }
        }
    }
}

bool CZMBaseWeapon::ShouldCancelReload() const
{
    if ( !m_bCanCancelReload )
        return false;

    CZMPlayer* pOwner = GetPlayerOwner();
    if  ( !pOwner )
        return false;

    return pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0;
}

void CZMBaseWeapon::CancelReload()
{
    SendWeaponAnim( GetIdleActivity() );

    m_bInReload2 = false;

    // Make sure we don't attack instantly when stopping the reload.
    // Add a bit more time.
    m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
    m_flNextSecondaryAttack = m_flNextPrimaryAttack;
}

void CZMBaseWeapon::StopReload()
{
    m_bInReload2 = false;
}

bool CZMBaseWeapon::ShouldIncrementClip() const
{
    return ( m_flNextClipFillTime != 0.0f && m_flNextClipFillTime <= gpGlobals->curtime );
}

void CZMBaseWeapon::IncrementClip()
{
    // We don't want to increment the clip more than once per reload.
    m_flNextClipFillTime = 0.0f;
    m_bCanCancelReload = false;


    auto* pOwner = GetPlayerOwner();
    if ( !pOwner )
        return;
    

    int nPrimAmmo = pOwner->GetAmmoCount( m_iPrimaryAmmoType );
    int nSecAmmo = pOwner->GetAmmoCount( m_iSecondaryAmmoType );

    // If I use primary clips, reload primary
    if ( UsesClipsForAmmo1() && nPrimAmmo > 0 && GetMaxClip1() > m_iClip1 )
    {
        int primary	= m_bReloadsSingly ? 1 : MIN( GetMaxClip1() - m_iClip1, nPrimAmmo );	
        m_iClip1 += primary;
        pOwner->RemoveAmmo( primary, m_iPrimaryAmmoType);
    }

    // If I use secondary clips, reload secondary
    if ( UsesClipsForAmmo2() && nSecAmmo > 0 && GetMaxClip2() > m_iClip2 )
    {
        int secondary = m_bReloadsSingly ? 1 : MIN( GetMaxClip2() - m_iClip2, nSecAmmo );
        m_iClip2 += secondary;
        pOwner->RemoveAmmo( secondary, m_iSecondaryAmmoType );
    }
}

bool CZMBaseWeapon::Reload()
{
    if ( !CanAct( WEPACTION_RELOAD ) ) return false;


    bool ret = DefaultReload(
                    GetMaxClip1(),
                    GetMaxClip2(),
                    UsesDryActivity( ACT_VM_RELOAD_EMPTY ) ? ACT_VM_RELOAD_EMPTY : ACT_VM_RELOAD );

    if ( ret )
    {
        GetPlayerOwner()->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
    }

    return ret;
}

bool CZMBaseWeapon::IsInSecondaryAttack() const
{
    auto* pMe = const_cast<CZMBaseWeapon*>( this );
    return GetActivity() == pMe->GetSecondaryAttackActivity();
}

void CZMBaseWeapon::AddViewKick()
{
    auto* pPlayer = GetPlayerOwner();

    if ( !pPlayer ) return;


    Vector min, max;
    
    if ( !IsInSecondaryAttack() )
    {
        min = GetWeaponConfig()->primary.vecViewPunch_Min;
        max = GetWeaponConfig()->primary.vecViewPunch_Max;
    }
    else
    {
        min = GetWeaponConfig()->secondary.vecViewPunch_Min;
        max = GetWeaponConfig()->secondary.vecViewPunch_Max;
    }



	QAngle punch;
	punch.x = SharedRandomFloat( "gunx", min.x, max.x );
	punch.y = SharedRandomFloat( "guny", min.y, max.y );
	punch.z = SharedRandomFloat( "gunz", min.z, max.z );;

	pPlayer->ViewPunch( punch );
}

//
const CZMBaseWeaponConfig* CZMBaseWeapon::GetWeaponConfig() const
{
    return GetWeaponConfigSystem()->GetConfigBySlot( GetConfigSlot() );
}

CHudTexture const* CZMBaseWeapon::GetSpriteActive() const
{
#ifdef CLIENT_DLL
    return GetWeaponConfig()->pIconActive;
#else
    return nullptr;
#endif
}

CHudTexture const* CZMBaseWeapon::GetSpriteInactive() const
{
#ifdef CLIENT_DLL
    return GetWeaponConfig()->pIconInactive;
#else
    return nullptr;
#endif
}

CHudTexture const* CZMBaseWeapon::GetSpriteAmmo() const
{
#ifdef CLIENT_DLL
    return GetWeaponConfig()->pIconAmmo;
#else
    return nullptr;
#endif
}

const char* CZMBaseWeapon::GetViewModel( int vmIndex ) const
{
#ifndef CLIENT_DLL
    if ( m_OverrideViewModel != NULL_STRING )
    {
        return STRING( m_OverrideViewModel );
    }
#endif

#ifdef CLIENT_DLL
    if ( GetWeaponConfig()->bOverriddenViewmodel )
    {
        return modelinfo->GetModelName( modelinfo->GetModel( m_iViewModelIndex ) );
    }
#endif

    return GetWeaponConfig()->pszModel_View;
}

const char* CZMBaseWeapon::GetWorldModel() const
{
#ifndef CLIENT_DLL
    if ( m_OverrideWorldModel != NULL_STRING )
    {
        return STRING( m_OverrideWorldModel );
    }
#endif

    return GetWeaponConfig()->pszModel_World;
}

const char* CZMBaseWeapon::GetAnimPrefix() const
{
    return GetWeaponConfig()->pszAnimPrefix;
}

int CZMBaseWeapon::GetMaxClip1() const
{
    if ( m_nOverrideClip1 >= 0 )
    {
        return m_nOverrideClip1;
    }

    return GetWeaponConfig()->nClipSize;
}

int CZMBaseWeapon::GetMaxClip2() const
{
    return WEAPON_NOCLIP;
}

int CZMBaseWeapon::GetDefaultClip1() const
{
    return GetWeaponConfig()->nDefaultClip;
}

int CZMBaseWeapon::GetSlot() const
{
    return GetWeaponConfig()->iSlot;
}

int CZMBaseWeapon::GetPosition() const
{
    return GetWeaponConfig()->iPosition;
}

int CZMBaseWeapon::GetWeight() const
{
    return GetWeaponConfig()->nWeight;
}

char const* CZMBaseWeapon::GetName() const
{
    // GetClassname is bad for the client.
    return GetWeaponConfig()->pszWeaponName;
}

char const* CZMBaseWeapon::GetPrintName() const
{
    return GetWeaponConfig()->pszPrintName;
}

char const* CZMBaseWeapon::GetShootSound( int iIndex ) const
{
    return GetWeaponConfig()->pszSounds[iIndex];
}

float CZMBaseWeapon::GetPrimaryFireRate() const
{
    return GetWeaponConfig()->primary.flFireRate;
}

float CZMBaseWeapon::GetAccuracyIncreaseRate() const
{
    return GetWeaponConfig()->flAccuracyIncrease;
}

float CZMBaseWeapon::GetAccuracyDecreaseRate() const
{
    return GetWeaponConfig()->flAccuracyDecrease;
}

float CZMBaseWeapon::GetPenetrationDmgMult() const
{
    return GetWeaponConfig()->flPenetrationDmgMult;
}

int CZMBaseWeapon::GetMaxPenetrations() const
{
    return GetWeaponConfig()->nMaxPenetrations;
}

float CZMBaseWeapon::GetMaxPenetrationDist() const
{
    return GetWeaponConfig()->flMaxPenetrationDist;
}

Vector CZMBaseWeapon::GetBulletSpread() const
{
    return (!IsInSecondaryAttack())
                ? GetWeaponConfig()->primary.vecSpread
                : GetWeaponConfig()->secondary.vecSpread;
}

float CZMBaseWeapon::GetFireRate()
{
    float flConfigFireRate = (!IsInSecondaryAttack())
                                ? GetWeaponConfig()->primary.flFireRate
                                : GetWeaponConfig()->secondary.flFireRate;

    // If the fire rate is not set, use sequence duration
    if ( !CZMBaseWeaponConfig::IsValidFirerate( flConfigFireRate ) )
    {
        return SequenceDuration();
    }

    return flConfigFireRate;
}

float CZMBaseWeapon::GetReloadTime() const
{
    float flReloadTime = GetWeaponConfig()->flReloadTime;

    // If the fire rate is not set, use sequence duration
    if ( !CZMBaseWeaponConfig::IsValidFirerate( flReloadTime ) )
    {
        return const_cast<CZMBaseWeapon*>( this )->SequenceDuration();
    }

    return flReloadTime;
}

int CZMBaseWeapon::GetBulletsPerShot() const
{
    return (!IsInSecondaryAttack())
        ? GetWeaponConfig()->primary.nBulletsPerShot
        : GetWeaponConfig()->secondary.nBulletsPerShot;
}
//



void CZMBaseWeapon::FireBullets( const FireBulletsInfo_t &info )
{
    if ( !GetOwner() ) return;


	FireBulletsInfo_t modinfo = info;


    float flConfigDamage = (!IsInSecondaryAttack())
                                ? GetWeaponConfig()->primary.flDamage
                                : GetWeaponConfig()->primary.flDamage;

#ifndef CLIENT_DLL
    modinfo.m_flDamage = ( m_nOverrideDamage > -1 ) ? (float)m_nOverrideDamage : flConfigDamage;
#else
    modinfo.m_flDamage = flConfigDamage;
#endif
    
	modinfo.m_iPlayerDamage = (int)modinfo.m_flDamage;
    
    
#ifndef CLIENT_DLL
    GetPlayerOwner()->CopyWeaponDamage( this, modinfo );
#endif

    GetOwner()->FireBullets( modinfo );
}

void CZMBaseWeapon::FireBullets( int numShots, int iAmmoType, float flMaxDist )
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer )
        return;


    FireBulletsInfo_t info;

    // This fixes a problem of the client's shooting being off.
    // More noticeable with higher pings.
#ifdef CLIENT_DLL
    if ( zm_sv_bulletsusemainview.GetBool() && !g_ZMThirdpersonManager.IsInThirdperson() )
    {
        info.m_vecSrc           = MainViewOrigin();
        info.m_vecDirShooting   = MainViewForward();
    }
    else
#endif
    {
        info.m_vecSrc           = pPlayer->Weapon_ShootPosition();
        info.m_vecDirShooting   = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
    }

    info.m_iShots = numShots;

    // ZMRTODO: See if this has any truth to it.
    // To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
    // especially if the weapon we're firing has a really fast rate of fire.
    /*float fireRate = GetFireRate();

    while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
    {
        // MUST call sound before removing a round from the clip of a CMachineGun
        WeaponSound( SINGLE );
        m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
        info.m_iShots++;
        if ( !fireRate )
            break;
    }*/


    info.m_flDistance = flMaxDist;
    info.m_iAmmoType = iAmmoType;
    info.m_iTracerFreq = 2;
    info.m_vecSpread = GetBulletSpread();

    // Fire the bullets
    // Use our FireBullets to get the weapon damage from .txt file.
    FireBullets( info );
}

float CZMBaseWeapon::GetFirstInstanceOfAnimEventTime( int iSeq, int iAnimEvent, bool bReturnOption ) const
{
    CZMBaseWeapon* me = const_cast<CZMBaseWeapon*>( this );

    CStudioHdr* hdr = me->GetModelPtr();
    if ( !hdr )
        return -1.0f;

    mstudioseqdesc_t& seqdesc = hdr->pSeqdesc( iSeq );
    if ( !seqdesc.numevents )
        return -1.0f;


    mstudioevent_t* pevent = seqdesc.pEvent( 0 );

    if ( !pevent )
    {
        return -1.0f;
    }

    for ( int i = 0; i < (int)seqdesc.numevents; i++ )
    {
        if ( pevent[i].event == iAnimEvent )
        {
            if ( bReturnOption && pevent[i].pszOptions()[0] != NULL )
                return atof( pevent[i].pszOptions() );
            return pevent[i].cycle * me->SequenceDuration( hdr, iSeq );
        }
    }

    return -1.0f;
}


void CZMBaseWeapon::PrimaryAttack()
{
    if ( !CanAct( WEPACTION_ATTACK ) ) return;


    // If my clip is empty (and I use clips) start reload
    if ( UsesClipsForAmmo1() && !m_iClip1 ) 
    {
        Reload();
        return;
    }


    m_flNextPrimaryAttack = gpGlobals->curtime + GetPrimaryFireRate();
    Shoot();
}

void CZMBaseWeapon::Shoot( int iAmmoType, int nBullets, int nAmmo, float flMaxRange, bool bSecondary )
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;


    // IMPORTANT: We NEED to set the animation first and foremost!
    // Certain methods will check for secondary fire by
    // comparing the weapon activity.
    if ( !bSecondary )
        PrimaryAttackEffects( SINGLE );
    else
        SecondaryAttackEffects( SINGLE );


    if ( iAmmoType == -1 )
        iAmmoType = m_iPrimaryAmmoType;
    if ( nAmmo <= -1 )
        nAmmo = 1;
    if ( nBullets <= -1 )
        nBullets = GetBulletsPerShot() * MAX( 1, nAmmo );
    if ( flMaxRange < 0.0f )
        flMaxRange = GetWeaponConfig()->primary.flRange;

    int& iClip = (int&)m_iClip1;


    Assert( nBullets > 0 );



    // ZMRTODO: Add burst firing here.


    // Make sure we don't fire more than the amount in the clip
    if ( UsesClipsForAmmo1() )
    {
        nAmmo = MIN( nAmmo, iClip );
        iClip -= nAmmo;
    }
    else
    {
        nAmmo = MIN( nAmmo, pPlayer->GetAmmoCount( iAmmoType ) );
        pPlayer->RemoveAmmo( nAmmo, iAmmoType );
    }

    Assert( nAmmo > 0 );

    
    // I still barely understand how you're suppose to handle prediction.
    // You cannot put the ENTIRE method behind this, because then none of the predictable variables seem to change on the client.
    // If you don't check this, then this will get called multiple times, in this case firing bullets multiple times.
    // It seems like you NEED to get called multiple times to set the predictable variables again.
    // But you put everything non-predictable behind this condition.
    // Problem is, almost everything is a predictable on the client.
#ifdef CLIENT_DLL
    if ( prediction->IsFirstTimePredicted() )
#endif
    {
        FireBullets( nBullets, iAmmoType, flMaxRange );
    }

    if ( !iClip && pPlayer->GetAmmoCount( iAmmoType ) <= 0 )
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
    }

    // Add our view kick in
    AddViewKick();

#ifndef CLIENT_DLL
    PlayAISound();
#endif
}

void CZMBaseWeapon::PrimaryAttackEffects( WeaponSound_t wpnsound )
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer )
        return;


    // IMPORTANT: Always needs to be called
    SendWeaponAnim( GetPrimaryAttackActivity() );

#ifdef CLIENT_DLL
    if ( prediction->IsFirstTimePredicted() )
#endif
    {
        pPlayer->DoMuzzleFlash();

        pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

        WeaponSound( wpnsound );
    }
}

void CZMBaseWeapon::SecondaryAttackEffects( WeaponSound_t wpnsound )
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer )
        return;


    // IMPORTANT: Always needs to be called
    SendWeaponAnim( GetSecondaryAttackActivity() );

#ifdef CLIENT_DLL
    if ( prediction->IsFirstTimePredicted() )
#endif
    {
        pPlayer->DoMuzzleFlash();

        pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );

        WeaponSound( wpnsound );
    }
}

#ifndef CLIENT_DLL
void CZMBaseWeapon::PlayAISound() const
{
    // Our owner is not guaranteed.
    // This may be called after we damage an explosive, owner dies and drops the weapon.
    CBaseEntity* pSrc = GetOwner();
    if ( !pSrc )
        pSrc = const_cast<CZMBaseWeapon*>( this );

    CSoundEnt::InsertSound( SOUND_COMBAT, pSrc->GetAbsOrigin(), GetAISoundVolume(), 0.25f, pSrc );
}
#endif

void CZMBaseWeapon::SecondaryAttack()
{
    if ( !CanAct( WEPACTION_ATTACK2 ) ) return;


    BaseClass::SecondaryAttack();
}

void CZMBaseWeapon::SetWeaponVisible( bool visible )
{
    CBaseViewModel* vm = nullptr;
    CZMViewModel* vmhands = nullptr;

    CZMPlayer* pOwner = GetPlayerOwner();
    if ( pOwner )
    {
        vm = pOwner->GetViewModel( VMINDEX_WEP );
        vmhands = static_cast<CZMViewModel*>( pOwner->GetViewModel( VMINDEX_HANDS ) );

#ifndef CLIENT_DLL
        Assert( vm == pOwner->GetViewModel( m_nViewModelIndex ) );
#endif
    }

    if ( visible )
    {
        RemoveEffects( EF_NODRAW );

        if ( vm ) vm->RemoveEffects( EF_NODRAW );
        if ( vmhands )
        {
            if ( GetWeaponConfig()->bUseHands )
            {
                vmhands->RemoveEffects( EF_NODRAW );
#ifdef CLIENT_DLL // Let client override this if they are using a custom viewmodel that doesn't use the new hands system.
                vmhands->SetDrawVM( true );
#endif
            }
            else
            {
#ifdef CLIENT_DLL
                vmhands->SetDrawVM( false );
#endif
            }

        }
    }
    else
    {
        AddEffects( EF_NODRAW );

        if ( vm ) vm->AddEffects( EF_NODRAW );
        if ( vmhands ) vmhands->AddEffects( EF_NODRAW );
    }
}

void CZMBaseWeapon::AssignWeaponConfigSlot()
{
#ifdef CLIENT_DLL
    if ( ZMWeaponConfig::CZMWeaponConfigSystem::IsDebugging() )
    {
        int index = entindex();
        Msg( "Assigning weapon config slot to %i on client!\n", index );
    }
#endif

    if ( GetConfigSlot() == ZMCONFIGSLOT_INVALID )
    {
        WeaponConfigSlot_t slot = GetWeaponConfigSystem()->RegisterBareBonesWeapon( GetClassname() );

        SetConfigSlot( slot );
    }


    const char* customscript =
#ifdef CLIENT_DLL
        m_sScriptFileName;
#else
        STRING( m_sScriptFileName.Get() );
#endif

    if ( customscript[0] != NULL )
    {
        WeaponConfigSlot_t slot = GetWeaponConfigSystem()->RegisterCustomWeapon( GetConfigSlot(), customscript );

        SetConfigSlot( slot );
    }
}

#ifdef CLIENT_DLL
int UTIL_CreateClientModel( const char* pszModel );
#endif

void CZMBaseWeapon::Precache()
{
    AssignWeaponConfigSlot();

	m_iPrimaryAmmoType = m_iSecondaryAmmoType = -1;


	// Get the ammo indexes for the ammo's specified in the data file
    const char* pszPrimAmmo = GetWeaponConfig()->pszAmmoName;
	if ( pszPrimAmmo && pszPrimAmmo[0] != NULL )
	{
		m_iPrimaryAmmoType = GetWeaponConfig()->GetPrimaryAmmoIndex();
		if (m_iPrimaryAmmoType == -1)
		{
			Msg("ERROR: Weapon (%s) using undefined primary ammo type (%s)\n", GetClassname(), GetWeaponConfig()->pszAmmoName );
		}
 	}

#if defined( CLIENT_DLL )
	gWR.LoadWeaponSprites( GetWeaponFileInfoHandle() );
#endif


	// Precache sounds, too
	for ( int i = 0; i < NUM_SHOOT_SOUND_TYPES; ++i )
	{
		auto* pszSound = GetShootSound( i );
		if ( pszSound && pszSound[0] )
		{
			if ( !CBaseEntity::PrecacheScriptSound( pszSound ) && !PrecacheSound( pszSound ) )
            {
                Warning( "Failed to precache weapon sound called '%s' from '%s'!\n", GetWeaponConfig()->pszConfigFilePath, pszSound );
            }
		}
	}



    //
    // Only set m_iViewModelIndex/m_iWorldModelIndex
    // as the server unless we're overriding the model.
    //
#ifdef GAME_DLL
	// Precache models (preload to avoid hitch)
	m_iViewModelIndex = 0;
	m_iWorldModelIndex = 0;
#endif


    // Make sure we precache all models.
    // It's possible to only precache the override model and the default one is left out.
    if ( GetWeaponConfig()->pszModel_View[0] != NULL )
    {
#ifdef CLIENT_DLL
        if ( GetWeaponConfig()->bOverriddenViewmodel )
        {
            m_iViewModelIndex = UTIL_CreateClientModel( GetWeaponConfig()->pszModel_View );
        }
#endif
#ifdef GAME_DLL
        m_iViewModelIndex = PrecacheModel( GetWeaponConfig()->pszModel_View );
#endif
    }

#ifdef GAME_DLL
    if ( GetWeaponConfig()->pszModel_World[0] != NULL )
    {
        m_iWorldModelIndex = PrecacheModel( GetWeaponConfig()->pszModel_World );
    }


    if ( m_OverrideViewModel != NULL_STRING )
    {
        m_iViewModelIndex = PrecacheModel( STRING( m_OverrideViewModel ) );
    }

    if ( m_OverrideWorldModel != NULL_STRING )
    {
        m_iWorldModelIndex = PrecacheModel( STRING( m_OverrideWorldModel ) );
    }
#endif
}

#ifdef CLIENT_DLL
void CZMBaseWeapon::Spawn()
{
    BaseClass::Spawn();

    SetNextClientThink( gpGlobals->curtime + 0.1f );
}

void CZMBaseWeapon::PostDataUpdate( DataUpdateType_t updateType )
{
    BaseClass::PostDataUpdate( updateType );

    if ( updateType == DATA_UPDATE_CREATED )
    {
        Precache();
    }
}

void CZMBaseWeapon::ClientThink()
{
    UpdateGlow();

    SetNextClientThink( gpGlobals->curtime + 0.1f );
}

void CZMBaseWeapon::UpdateGlow()
{
    if ( !zm_cl_glow_weapon_enabled.GetBool() || !zm_sv_glow_item_enabled.GetBool() )
    {
        if ( IsClientSideGlowEnabled() )
            SetClientSideGlowEnabled( false );

        return;
    }


    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();

    if (pPlayer
    &&  GetOwner() == nullptr
    &&  pPlayer->IsHuman()
    &&  pPlayer->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) < ITEM_GLOW_DIST_SQR
    &&  !(pPlayer->GetWeaponSlotFlags() & GetSlotFlag()) )
    {
        if ( !IsClientSideGlowEnabled() )
            SetClientSideGlowEnabled( true );
    }
    else
    {
        SetClientSideGlowEnabled( false );
    }
}

void UTIL_ParseFloatColorFromString( const char* str, float clr[], int nColors );

void CZMBaseWeapon::GetGlowEffectColor( float& r, float& g, float& b )
{
    float clr[3];
    UTIL_ParseFloatColorFromString( zm_cl_glow_weapon.GetString(), clr, ARRAYSIZE( clr ) );

    r = clr[0];
    g = clr[1];
    b = clr[2];
}

ShadowType_t CZMBaseWeapon::ShadowCastType()
{
    if ( IsEffectActive( EF_NODRAW | EF_NOSHADOW ) )
        return SHADOWS_NONE;


    auto* pOwner = GetPlayerOwner();
    if ( !pOwner ) // Not carried
        return SHADOWS_RENDER_TO_TEXTURE;


    // Player's shadow isn't drawn either.
    if ( pOwner->ShadowCastType() == SHADOWS_NONE )
        return SHADOWS_NONE;


    // In firstperson?
    if ( pOwner->IsLocalPlayer() && !C_BasePlayer::ShouldDrawLocalPlayer() )
        return SHADOWS_NONE;


    // In thirdperson
    // Draw if active
    return pOwner->GetActiveWeapon() == this ? SHADOWS_RENDER_TO_TEXTURE : SHADOWS_NONE;
}

void CZMBaseWeapon::OnDataChanged( DataUpdateType_t type )
{
    CBaseAnimating::OnDataChanged( type );

    if ( GetPredictable() && !ShouldPredict() )
	    ShutdownPredictable();

    //
    // Override BaseCombatWeapon::OnDataChanged
    //
    

    // If it's being carried by the *local* player, on the first update,
    // find the registered weapon for this ID
    
    auto* pOwner = GetPlayerOwner();

    if ( pOwner && pOwner->IsLocalPlayer() && !C_ZMPlayer::ShouldDrawLocalPlayer() )
    {
        // If I was just picked up, or created & immediately carried, add myself to this client's list of weapons
        if ( (m_iState != WEAPON_NOT_CARRIED) && (m_iOldState == WEAPON_NOT_CARRIED) )
        {
            // Tell the HUD this weapon's been picked up
            if ( ShouldDrawPickup() )
            {
                auto* pHudSelection = GetHudWeaponSelection();
                if ( pHudSelection )
                {
                    pHudSelection->OnWeaponPickup( this );
                }

                //pPlayer->EmitSound( "Player.PickupWeapon" );
            }
        }
    }
    else // weapon carried by other player or not at all
    {
        int overrideModelIndex = CalcOverrideModelIndex();
        if( overrideModelIndex != -1 && overrideModelIndex != GetModelIndex() )
        {
            SetModelIndex( overrideModelIndex );
        }
    }

    if ( type == DATA_UPDATE_CREATED )
    {
        UpdateVisibility();
    }

    m_iOldState = m_iState;

    //m_bJustRestored = false;
}

bool CZMBaseWeapon::ShouldPredict()
{
    if ( CBaseCombatWeapon::GetOwner() && CBaseCombatWeapon::GetOwner() == C_BasePlayer::GetLocalPlayer() )
	    return true;

    return BaseClass::ShouldPredict();
}

CZMBaseCrosshair* CZMBaseWeapon::GetWeaponCrosshair() const
{
    return g_ZMCrosshairs.GetCrosshairByIndex( GetWeaponConfig()->iCrosshair );
}
#endif

CZMPlayer* CZMBaseWeapon::GetPlayerOwner() const
{
    return ToZMPlayer( CBaseCombatWeapon::GetOwner() );
}

void CZMBaseWeapon::WeaponSound( WeaponSound_t sound_type, float soundtime /* = 0.0f */ )
{
	const char* pszSound = GetShootSound( sound_type ); 
	if ( !pszSound || !pszSound[0] )
		return;


    auto* pOwner = GetOwner();
    Assert( pOwner );

    Vector origin = pOwner->WorldSpaceCenter();

	CSoundParameters params;
	bool bIsSoundScript = GetParametersForSound( pszSound, params, nullptr );


#ifdef CLIENT_DLL
	CBroadcastRecipientFilter filter;
	if ( !te->CanPredict() )
		return;
#else


    CPASAttenuationFilter filter( pOwner, params.soundlevel );
    if ( IsPredicted() && CBaseEntity::GetPredictionPlayer() )
    {
        filter.UsePredictionRules();
    }
#endif

    if ( bIsSoundScript )
    {
        EmitSound( filter, pOwner->entindex(), pszSound, &origin, soundtime ); 
    }
    else
    {
        EmitSound_t snd;
        snd.m_pSoundName = pszSound;
        snd.m_pOrigin = &origin;

        EmitSound( filter, pOwner->entindex(), snd );
    }
    
}

void CZMBaseWeapon::SetViewModel()
{
    CZMPlayer* pOwner = GetPlayerOwner();
    if ( !pOwner ) return;

    auto* vm = static_cast<CZMViewModel*>( pOwner->GetViewModel( m_nViewModelIndex, false ) );
    if ( !vm ) return;

    Assert( vm->ViewModelIndex() == m_nViewModelIndex );
    

    // Use the model index instead of GetViewModel()...
    // This makes sure we don't have to network the model path to the client.
    const char* pszModel = nullptr;


    const model_t* pModel = modelinfo->GetModel( m_iViewModelIndex );
    if ( pModel )
    {
        pszModel = modelinfo->GetModelName( pModel );
    }

    // Our networked index isn't valid, use the shared version.
    if ( !pszModel || !(*pszModel) )
    {
        pszModel = GetViewModel( m_nViewModelIndex );
    }

    bool bOverridden = false;
#ifdef CLIENT_DLL
    bOverridden = GetWeaponConfig()->bOverriddenViewmodel;
#endif

    vm->SetWeaponModelEx( pszModel, this, bOverridden );
}

#ifndef CLIENT_DLL
bool CZMBaseWeapon::IsTemplate()
{
    if ( HasSpawnFlags( SF_ZMWEAPON_TEMPLATE ) )
    {
        return true;
    }

    return BaseClass::IsTemplate();
}

void CZMBaseWeapon::Materialize()
{
	if ( IsEffectActive( EF_NODRAW ) )
	{
		RemoveEffects( EF_NODRAW );
	}

    Assert( VPhysicsGetObject() );

	SetPickupTouch();

	SetThink( nullptr );
}
#endif

void CZMBaseWeapon::FallInit()
{
#ifndef CLIENT_DLL
	SetModel( GetWorldModel() );
	VPhysicsDestroyObject();

	if ( VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false ) )
	{
		// Constrain the weapon in place
		if ( HasSpawnFlags( SF_WEAPON_START_CONSTRAINED ) )
		{
			auto* pReferenceObject = g_PhysWorldObject;
			auto* pAttachedObject = VPhysicsGetObject();

			if ( pReferenceObject && pAttachedObject )
			{
				constraint_fixedparams_t fixed;
				fixed.Defaults();
				fixed.InitWithCurrentObjectState( pReferenceObject, pAttachedObject );
					
				fixed.constraint.forceLimit	= lbs2kg( 1000 );
				fixed.constraint.torqueLimit = lbs2kg( 1000 );

				ReleaseConstraint();

				m_pConstraint = physenv->CreateFixedConstraint( pReferenceObject, pAttachedObject, nullptr, fixed );

				m_pConstraint->SetGameData( (void*)this );
			}
		}
	}
    else
    {
        Assert( 0 );
    }

	SetPickupTouch();
	
	SetThink( &CZMBaseWeapon::FallThink );

	SetNextThink( gpGlobals->curtime + 0.1f );
#endif
}

#ifdef GAME_DLL
void CZMBaseWeapon::ReleaseConstraint()
{
    if ( m_pConstraint )
    {
        physenv->DestroyConstraint( m_pConstraint );
        m_pConstraint = nullptr;
    }
}

void CZMBaseWeapon::FallThink()
{
	SetNextThink( gpGlobals->curtime + 0.1f );


    auto* pPhys = VPhysicsGetObject();

	bool shouldMaterialize = false;

	if ( pPhys )
	{
		shouldMaterialize = pPhys->IsAsleep();
	}
	else
	{
        Assert( 0 );
		shouldMaterialize = true;
	}

	if ( shouldMaterialize )
	{
		Materialize(); 
	}
}
#endif

void CZMBaseWeapon::SetPickupTouch()
{
#ifndef CLIENT_DLL
    SetTouch( &CBaseCombatWeapon::DefaultTouch );
#endif
}

void CZMBaseWeapon::OnPickedUp( CBaseCombatCharacter* pNewOwner )
{
#ifndef CLIENT_DLL
    RemoveEffects( EF_ITEM_BLINK );


    auto* pZMPlayer = ToZMPlayer( pNewOwner );
    if ( pZMPlayer )
    {
        // HACK: Don't play sound if we can't be dropped
        if ( CanBeDropped() )
        {
            // Play pickup sound
            CRecipientFilter filter;
            pZMPlayer->GetMyRecipientFilter( filter );

            CBaseEntity::EmitSound( filter, pNewOwner->entindex(), "ZMPlayer.PickupWeapon" );
        }


        m_OnPlayerPickup.FireOutput( pNewOwner, this );

        // Robin: We don't want to delete weapons the player has picked up, so
        // clear the name of the weapon. This prevents wildcards that are meant
        // to find NPCs finding weapons dropped by the NPCs as well.
        SetName( NULL_STRING );
    }
    else
    {
        m_OnNPCPickup.FireOutput( pNewOwner, this );
    }

    // Someone picked me up, so make it so that I can't be removed.
    SetRemoveable( false );
#else


    //{
    //    // Tell the HUD this weapon's been picked up
    //    if ( ShouldDrawPickup() )
    //    {
    //        CBaseHudWeaponSelection* pHudSelection = GetHudWeaponSelection();
    //        if ( pHudSelection )
    //        {
    //            pHudSelection->OnWeaponPickup( this );
    //        }
    //    }
    //}

    //m_iOldState = m_iState;

    //m_bJustRestored = false;
#endif
}

void CZMBaseWeapon::DoMachineGunKick( float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime )
{
    //Get the view kick
    CZMPlayer *pPlayer = GetPlayerOwner();

    if ( pPlayer == NULL ) return;


    #define	KICK_MIN_X			0.2f // Degrees
    #define	KICK_MIN_Y			0.2f // Degrees
    #define	KICK_MIN_Z			0.1f // Degrees

    QAngle vecScratch;
    int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
	
    //Find how far into our accuracy degradation we are
    float duration	= ( fireDurationTime > slideLimitTime ) ? slideLimitTime : fireDurationTime;
    float kickPerc = duration / slideLimitTime;

    // do this to get a hard discontinuity, clear out anything under 10 degrees punch
    pPlayer->ViewPunchReset( 10 );

    //Apply this to the view angles as well
    vecScratch.x = -( KICK_MIN_X + ( maxVerticleKickAngle * kickPerc ) );
    vecScratch.y = -( KICK_MIN_Y + ( maxVerticleKickAngle * kickPerc ) ) / 3;
    vecScratch.z = KICK_MIN_Z + ( maxVerticleKickAngle * kickPerc ) / 8;

    RandomSeed( iSeed );

    //Wibble left and right
    if ( RandomInt( -1, 1 ) >= 0 )
	    vecScratch.y *= -1;

    iSeed++;

    //Wobble up and down
    if ( RandomInt( -1, 1 ) >= 0 )
	    vecScratch.z *= -1;

    //Clip this to our desired min/max
    UTIL_ClipPunchAngleOffset( vecScratch, pPlayer->m_Local.m_vecPunchAngle, QAngle( 24.0f, 3.0f, 1.0f ) );

    //Add it to the view punch
    // NOTE: 0.5 is just tuned to match the old effect before the punch became simulated
    pPlayer->ViewPunch( vecScratch * 0.5 );
}

bool CZMBaseWeapon::CanBeSelected()
{
	if ( !VisibleInWeaponSelection() )
		return false;

    return true;
}

void CZMBaseWeapon::Drop( const Vector& vecVelocity )
{
#ifndef CLIENT_DLL
    FreeWeaponSlot();


    SaveReserveAmmo( GetOwner() );
#endif

    BaseClass::Drop( vecVelocity );


    // ZMRTODO: See if this works as intended -> drop it so player changes weapon and then remove.
#ifndef CLIENT_DLL
    if ( !CanBeDropped() )
    {
        UTIL_Remove( this );
    }
#endif


    // Reset reloading
    m_bInReload2 = false;
    m_bFiringWholeClip = false;

    m_flNextClipFillTime = 0.0f;
    m_bCanCancelReload = false;
}

void CZMBaseWeapon::Equip( CBaseCombatCharacter* pCharacter )
{
#ifndef CLIENT_DLL
    if ( pCharacter->IsPlayer() && GetSlotFlag() != ZMWEAPONSLOT_NOLIMIT )
    {
        CZMPlayer* pPlayer = ToZMPlayer( pCharacter );

        if ( pPlayer )
        {
            if ( IsDebugging() )
            {
                Msg( "Adding slot flag %i\n", GetSlotFlag() );
            }
            
            pPlayer->AddWeaponSlotFlag( GetSlotFlag() );
        }
    }

    TransferReserveAmmo( pCharacter );

    ReleaseConstraint();
    RemoveSpawnFlags( SF_WEAPON_START_CONSTRAINED );
#endif

    BaseClass::Equip( pCharacter );
}

float CZMBaseWeapon::CalcViewmodelBob()
{
    return 0.0f;
}

void CZMBaseWeapon::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
}

#ifndef CLIENT_DLL
void CZMBaseWeapon::SaveReserveAmmo( CBaseCombatCharacter* pOwner )
{
    if ( !pOwner ) return;

    if ( !zm_sv_weaponreserveammo.GetBool() ) return;


    // Add player's ammo to me.
    int type = GetPrimaryAmmoType();

    int ammo = pOwner->GetAmmoCount( type );

    if ( ammo > 0 )
    {
        pOwner->SetAmmoCount( 0, type );
        SetReserveAmmo( ammo );
    }
    else
    {
        SetReserveAmmo( 0 );
    }
}

void CZMBaseWeapon::TransferReserveAmmo( CBaseCombatCharacter* pOwner )
{
    if ( !pOwner ) return;

    if ( !zm_sv_weaponreserveammo.GetBool() ) return;


    // Give player our reserve ammo.
    if ( GetReserveAmmo() > 0 )
    {
        int type = GetPrimaryAmmoType();

        pOwner->SetAmmoCount( pOwner->GetAmmoCount( type ) + GetReserveAmmo(), type );
        SetReserveAmmo( 0 );
    }
}

float CZMBaseWeapon::GetMaxDamageDist( ZMUserCmdValidData_t& data ) const
{
    return 8192.0f;
}

int CZMBaseWeapon::GetMaxUserCmdBullets( ZMUserCmdValidData_t& data ) const
{
    return GetBulletsPerShot();
}

int CZMBaseWeapon::GetMaxNumPenetrate( ZMUserCmdValidData_t& data ) const
{
    return GetMaxPenetrations();
}
#endif

bool CZMBaseWeapon::CanAct( ZMWepActionType_t type ) const
{
    // We're not using this right now, remove this if we do.
    Assert( type != WEPACTION_GENERIC );


    auto* pOwner = GetOwner();

    if ( !pOwner )
        return false;


    int flags = GetWeaponConfig()->fFlags;


    // Ladder
    if ( pOwner->GetMoveType() == MOVETYPE_LADDER )
    {
        switch ( type )
        {
        case WEPACTION_RELOAD :
            return (flags & ZMWeaponConfig::WEPFLAG_RELOAD_ONLADDER) ? true : false;
        case WEPACTION_ATTACK :
        case WEPACTION_ATTACK2 :
        case WEPACTION_ATTACK3 :
            return (flags & ZMWeaponConfig::WEPFLAG_ATTACK_ONLADDER) ? true : false;
        default :
            break;
        }
    }
    // Underwater
    else if ( pOwner->GetWaterLevel() >= WL_Eyes )
    {
        switch ( type )
        {
        case WEPACTION_ATTACK :
        case WEPACTION_ATTACK2 :
        case WEPACTION_ATTACK3 :
            return (flags & ZMWeaponConfig::WEPFLAG_ATTACK_INWATER) ? true : false;
        default :
            break;
        }
    }

    return true;
}
