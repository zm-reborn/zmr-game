#include "cbase.h"

#include <vphysics/constraints.h>


#include "zmr/zmr_gamerules.h"
#include "zmr/weapons/zmr_base.h"
#include "zmr/zmr_shareddefs.h"


#ifdef CLIENT_DLL
void UTIL_ClipPunchAngleOffset( QAngle &in, const QAngle &punch, const QAngle &clip );
#endif


#ifndef CLIENT_DLL
static ConVar zm_sv_weaponreserveammo( "zm_sv_weaponreserveammo", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE, "When player drops their weapon, their ammo gets dropped with the weapon as well." );
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

    SetReserveAmmo( 0 );
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

bool CZMBaseWeapon::Reload()
{
    if ( !CanAct() ) return false;


    return BaseClass::Reload();
}

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
    if ( !CanAct() ) return;


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

void CZMBaseWeapon::SecondaryAttack( void )
{
    if ( !CanAct() ) return;


    BaseClass::SecondaryAttack();
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

void CZMBaseWeapon::SetPickupTouch( void )
{
#ifndef CLIENT_DLL
    SetTouch( &CBaseCombatWeapon::DefaultTouch );
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

bool CZMBaseWeapon::CanBeSelected( void )
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

    TransferReserveAmmo( pCharacter );
#endif

    BaseClass::Equip( pCharacter );
}


// Viewmodel stuff from basehl2mpcombatweapon.
#ifdef CLIENT_DLL

#define	HL2_BOB_CYCLE_MIN	1.0f
#define	HL2_BOB_CYCLE_MAX	0.45f
#define	HL2_BOB			0.002f
#define	HL2_BOB_UP		0.5f

extern float	g_lateralBob;
extern float	g_verticalBob;

float CZMBaseWeapon::CalcViewmodelBob( void )
{
    static	float bobtime;
    static	float lastbobtime;
    float	cycle;
    
    CBasePlayer *player = ToBasePlayer( GetOwner() );
    //Assert( player );

    //NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

    if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
    {
        //NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
        return 0.0f;// just use old value
    }

    //Find the speed of the player
    float speed = player->GetLocalVelocity().Length2D();

    //FIXME: This maximum speed value must come from the server.
    //		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

    speed = clamp( speed, -320, 320 );

    float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );
    
    bobtime += ( gpGlobals->curtime - lastbobtime ) * bob_offset;
    lastbobtime = gpGlobals->curtime;

    //Calculate the vertical bob
    cycle = bobtime - (int)(bobtime/HL2_BOB_CYCLE_MAX)*HL2_BOB_CYCLE_MAX;
    cycle /= HL2_BOB_CYCLE_MAX;

    if ( cycle < HL2_BOB_UP )
    {
        cycle = M_PI * cycle / HL2_BOB_UP;
    }
    else
    {
        cycle = M_PI + M_PI*(cycle-HL2_BOB_UP)/(1.0 - HL2_BOB_UP);
    }
    
    g_verticalBob = speed*0.005f;
    g_verticalBob = g_verticalBob*0.3 + g_verticalBob*0.7*sin(cycle);

    g_verticalBob = clamp( g_verticalBob, -7.0f, 4.0f );

    //Calculate the lateral bob
    cycle = bobtime - (int)(bobtime/HL2_BOB_CYCLE_MAX*2)*HL2_BOB_CYCLE_MAX*2;
    cycle /= HL2_BOB_CYCLE_MAX*2;

    if ( cycle < HL2_BOB_UP )
    {
        cycle = M_PI * cycle / HL2_BOB_UP;
    }
    else
    {
        cycle = M_PI + M_PI*(cycle-HL2_BOB_UP)/(1.0 - HL2_BOB_UP);
    }

    g_lateralBob = speed*0.005f;
    g_lateralBob = g_lateralBob*0.3 + g_lateralBob*0.7*sin(cycle);
    g_lateralBob = clamp( g_lateralBob, -7.0f, 4.0f );
    
    //NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
    return 0.0f;
}

void CZMBaseWeapon::AddViewmodelBob( CBaseViewModel *viewmodel, Vector& origin, QAngle& angles )
{
    Vector	forward, right;
    AngleVectors( angles, &forward, &right, NULL );

    CalcViewmodelBob();

    // Apply bob, but scaled down to 40%
    VectorMA( origin, g_verticalBob * 0.1f, forward, origin );
    
    // Z bob a bit more
    origin[2] += g_verticalBob * 0.1f;
    
    // bob the angles
    angles[ ROLL ]	+= g_verticalBob * 0.5f;
    angles[ PITCH ]	-= g_verticalBob * 0.4f;

    angles[ YAW ]	-= g_lateralBob  * 0.3f;

    VectorMA( origin, g_lateralBob * 0.8f, right, origin );
}

Vector CZMBaseWeapon::GetBulletSpread( WeaponProficiency_t proficiency )
{
    return BaseClass::GetBulletSpread( proficiency );
}

float CZMBaseWeapon::GetSpreadBias( WeaponProficiency_t proficiency )
{
    return BaseClass::GetSpreadBias( proficiency );
}

const WeaponProficiencyInfo_t* CZMBaseWeapon::GetProficiencyValues()
{
    return nullptr;
}

#else

// Server stubs
float CZMBaseWeapon::CalcViewmodelBob( void )
{
    return 0.0f;
}

void CZMBaseWeapon::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
}

Vector CZMBaseWeapon::GetBulletSpread( WeaponProficiency_t proficiency )
{
    Vector baseSpread = BaseClass::GetBulletSpread( proficiency );

    const WeaponProficiencyInfo_t *pProficiencyValues = GetProficiencyValues();
    float flModifier = (pProficiencyValues)[ proficiency ].spreadscale;
    return ( baseSpread * flModifier );
}

float CZMBaseWeapon::GetSpreadBias( WeaponProficiency_t proficiency )
{
    const WeaponProficiencyInfo_t *pProficiencyValues = GetProficiencyValues();
    return (pProficiencyValues)[ proficiency ].bias;
}

const WeaponProficiencyInfo_t* CZMBaseWeapon::GetProficiencyValues()
{
    return GetDefaultProficiencyValues();
}

const WeaponProficiencyInfo_t* CZMBaseWeapon::GetDefaultProficiencyValues()
{
    // Weapon proficiency table. Keep this in sync with WeaponProficiency_t enum in the header!!
    static WeaponProficiencyInfo_t g_BaseWeaponProficiencyTable[] =
    {
        { 2.50, 1.0	},
        { 2.00, 1.0	},
        { 1.50, 1.0	},
        { 1.25, 1.0 },
        { 1.00, 1.0	},
    };

    COMPILE_TIME_ASSERT( ARRAYSIZE(g_BaseWeaponProficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

    return g_BaseWeaponProficiencyTable;
}

#endif




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
#endif

bool CZMBaseWeapon::CanAct()
{
    CBaseCombatCharacter* pOwner = GetOwner();

    if ( !pOwner )
        return false;

    if ( pOwner->GetMoveType() == MOVETYPE_LADDER )
        return false;


    return true;
}
