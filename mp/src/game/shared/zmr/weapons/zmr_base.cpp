#include "cbase.h"

#include <vphysics/constraints.h>


#include "zmr/zmr_gamerules.h"
#include "zmr/weapons/zmr_base.h"
#include "zmr/zmr_shareddefs.h"


#ifdef CLIENT_DLL
void UTIL_ClipPunchAngleOffset( QAngle &in, const QAngle &punch, const QAngle &clip );
#endif


BEGIN_NETWORK_TABLE( CZMBaseWeapon, DT_ZM_BaseWeapon )
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( ZMBaseWeapon, DT_ZM_BaseWeapon )

BEGIN_PREDICTION_DATA( CZMBaseWeapon )
END_PREDICTION_DATA()

BEGIN_DATADESC( CZMBaseWeapon )
END_DATADESC()

LINK_ENTITY_TO_CLASS( weapon_zm_base, CZMBaseWeapon );


CZMBaseWeapon::CZMBaseWeapon()
{
	SetPredictionEligible( true );
	AddSolidFlags( FSOLID_TRIGGER ); // Nothing collides with these but it gets touches.

#ifndef CLIENT_DLL
    SetSlotFlag( ZMWEAPONSLOT_NONE );
#endif
}

CZMBaseWeapon::~CZMBaseWeapon()
{
#ifndef CLIENT_DLL
    FreeWeaponSlot();
#endif
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
#endif

const CZMWeaponInfo& CZMBaseWeapon::GetWpnData() const
{
    const FileWeaponInfo_t *pBase = &CBaseCombatWeapon::GetWpnData();
	const CZMWeaponInfo *pInfo;

#ifdef _DEBUG
    pInfo = dynamic_cast<const CZMWeaponInfo*>( pBase );
    Assert( pInfo );
#else
	pInfo = static_cast<const CZMWeaponInfo*>( pBase );
#endif

    return *pInfo;
}

void CZMBaseWeapon::FireBullets( const FireBulletsInfo_t &info )
{
    if ( !GetOwner() ) return;


	FireBulletsInfo_t modinfo = info;

    modinfo.m_flDamage = GetWpnData().m_flDamage;
	modinfo.m_iPlayerDamage = (int)modinfo.m_flDamage;
    
    

    GetOwner()->FireBullets( modinfo );
}

void CZMBaseWeapon::PrimaryAttack( void )
{
    // If my clip is empty (and I use clips) start reload
    if ( UsesClipsForAmmo1() && !m_iClip1 ) 
    {
        Reload();
        return;
    }


    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;


    pPlayer->DoMuzzleFlash();

    SendWeaponAnim( GetPrimaryAttackActivity() );
    pPlayer->SetAnimation( PLAYER_ATTACK1 );


    FireBulletsInfo_t info;
    info.m_vecSrc	 = pPlayer->Weapon_ShootPosition();
    
    info.m_vecDirShooting = pPlayer->CBasePlayer::GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );


    info.m_iShots = 1;

    WeaponSound( SINGLE, m_flNextPrimaryAttack );
    m_flNextPrimaryAttack = m_flNextPrimaryAttack + GetFireRate();

    // ZMRTODO: See if this has any truth to it.
    // To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
    // especially if the weapon we're firing has a really fast rate of fire.
    /*float fireRate = GetFireRate();

    while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
    {
        // MUST call sound before removing a round from the clip of a CMachineGun
        WeaponSound(SINGLE, m_flNextPrimaryAttack);
        m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
        info.m_iShots++;
        if ( !fireRate )
            break;
    }*/

    // Make sure we don't fire more than the amount in the clip
    if ( UsesClipsForAmmo1() )
    {
        info.m_iShots = MIN( info.m_iShots, m_iClip1 );
        m_iClip1 -= info.m_iShots;
    }
    else
    {
        info.m_iShots = MIN( info.m_iShots, pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );
        pPlayer->RemoveAmmo( info.m_iShots, m_iPrimaryAmmoType );
    }

    info.m_flDistance = MAX_TRACE_LENGTH;
    info.m_iAmmoType = m_iPrimaryAmmoType;
    info.m_iTracerFreq = 2;

#ifndef CLIENT_DLL
    info.m_vecSpread = pPlayer->GetAttackSpread( this );
#else
    //!!!HACKHACK - what does the client want this function for? 
    info.m_vecSpread = GetActiveWeapon()->GetBulletSpread();
#endif

    // Fire the bullets
    // Use our FireBullets to get the weapon damage from .txt file.
    FireBullets( info );


    if ( !m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
    }

    // Add our view kick in
    AddViewKick();
}

#ifdef CLIENT_DLL
void CZMBaseWeapon::OnDataChanged( DataUpdateType_t type )
{
    BaseClass::OnDataChanged( type );

    if ( GetPredictable() && !ShouldPredict() )
	    ShutdownPredictable();
}

bool CZMBaseWeapon::ShouldPredict()
{
    if ( CBaseCombatWeapon::GetOwner() && CBaseCombatWeapon::GetOwner() == C_BasePlayer::GetLocalPlayer() )
	    return true;

    return BaseClass::ShouldPredict();
}
#endif

CZMPlayer* CZMBaseWeapon::GetPlayerOwner()
{
    return ToZMPlayer( CBaseCombatWeapon::GetOwner() );
}

void CZMBaseWeapon::WeaponSound( WeaponSound_t sound_type, float soundtime /* = 0.0f */ )
{
#ifdef CLIENT_DLL
		// If we have some sounds from the weapon classname.txt file, play a random one of them
		const char *shootsound = GetWpnData().aShootSounds[ sound_type ]; 
		if ( !shootsound || !shootsound[0] )
			return;

		CBroadcastRecipientFilter filter; // this is client side only
		if ( !te->CanPredict() )
			return;
		
		CBaseEntity::EmitSound( filter, GetOwner()->entindex(), shootsound, &GetOwner()->GetAbsOrigin() ); 
#else
		BaseClass::WeaponSound( sound_type, soundtime );
#endif
}

#ifndef CLIENT_DLL
void CZMBaseWeapon::Materialize( void )
{
	if ( IsEffectActive( EF_NODRAW ) )
	{
		// changing from invisible state to visible.
		//EmitSound( "AlyxEmp.Charge" );
		
		RemoveEffects( EF_NODRAW );
		//DoMuzzleFlash();
	}

	if ( /*HasSpawnFlags( SF_NORESPAWN ) == false*/ 1 )
	{
		VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );
		SetMoveType( MOVETYPE_VPHYSICS );


		//ZMRules()->AddLevelDesignerPlacedObject( this );

	}

	/*if ( HasSpawnFlags( SF_NORESPAWN ) == false )
	{
		if ( GetOriginalSpawnOrigin() == vec3_origin )
		{
			m_vOriginalSpawnOrigin = GetAbsOrigin();
			m_vOriginalSpawnAngles = GetAbsAngles();
		}
	}*/

	SetPickupTouch();

	SetThink (NULL);
}
#endif

void CZMBaseWeapon::FallInit( void )
{
#ifndef CLIENT_DLL
	SetModel( GetWorldModel() );
	VPhysicsDestroyObject();

	if ( HasSpawnFlags( SF_NORESPAWN ) == false )
	{
		SetMoveType( MOVETYPE_NONE );
		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_TRIGGER );

		UTIL_DropToFloor( this, MASK_SOLID );
	}
	else
	{
		if ( !VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false ) )
		{
			SetMoveType( MOVETYPE_NONE );
			SetSolid( SOLID_BBOX );
			AddSolidFlags( FSOLID_TRIGGER );
		}
		else
		{
			// Constrained start?
			if ( HasSpawnFlags( SF_WEAPON_START_CONSTRAINED ) )
			{
				//Constrain the weapon in place
				IPhysicsObject *pReferenceObject, *pAttachedObject;
				
				pReferenceObject = g_PhysWorldObject;
				pAttachedObject = VPhysicsGetObject();

				if ( pReferenceObject && pAttachedObject )
				{
					constraint_fixedparams_t fixed;
					fixed.Defaults();
					fixed.InitWithCurrentObjectState( pReferenceObject, pAttachedObject );
					
					fixed.constraint.forceLimit	= lbs2kg( 10000 );
					fixed.constraint.torqueLimit = lbs2kg( 10000 );

					IPhysicsConstraint *pConstraint = GetConstraint();

					pConstraint = physenv->CreateFixedConstraint( pReferenceObject, pAttachedObject, NULL, fixed );

					pConstraint->SetGameData( (void *) this );
				}
			}
		}
	}

	SetPickupTouch();
	
	SetThink( &CBaseCombatWeapon::FallThink );

	SetNextThink( gpGlobals->curtime + 0.1f );

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
#endif

    BaseClass::Drop( vecVelocity );


    // ZMRTODO: See if this works as intended -> drop it so player changes weapon and then remove.
#ifndef CLIENT_DLL
    if ( !CanBeDropped() )
    {
        UTIL_Remove( this );
    }
#endif
}

void CZMBaseWeapon::Equip( CBaseCombatCharacter* pCharacter )
{
#ifndef CLIENT_DLL
    if ( pCharacter->IsPlayer() && GetSlotFlag() != ZMWEAPONSLOT_NOLIMIT )
    {
        CZMPlayer* pPlayer = ToZMPlayer( pCharacter );

        if ( pPlayer )
        {
            DevMsg( "Adding slot flag %i\n", GetSlotFlag() );
            pPlayer->AddWeaponSlotFlag( GetSlotFlag() );
        }
    }
#endif

    BaseClass::Equip( pCharacter );
}