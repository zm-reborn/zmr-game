#include "cbase.h"

#include <vphysics/constraints.h>


#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_viewmodel.h"
#include "zmr/weapons/zmr_base.h"
#include "zmr/zmr_shareddefs.h"


#ifdef CLIENT_DLL
void UTIL_ClipPunchAngleOffset( QAngle &in, const QAngle &punch, const QAngle &clip );


ConVar zm_cl_glow_weapon( "zm_cl_glow_weapon", "1 .2 .2", FCVAR_ARCHIVE );
ConVar zm_cl_glow_weapon_enabled( "zm_cl_glow_weapon_enabled", "1", FCVAR_ARCHIVE );

extern ConVar zm_sv_glow_item_enabled;
#endif


#ifndef CLIENT_DLL
static ConVar zm_sv_weaponreserveammo( "zm_sv_weaponreserveammo", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE, "When player drops their weapon, their ammo gets dropped with the weapon as well." );
#endif


BEGIN_NETWORK_TABLE( CZMBaseWeapon, DT_ZM_BaseWeapon )
#ifdef CLIENT_DLL
    RecvPropInt( RECVINFO( m_nOverrideClip1 ) )
#else
    SendPropInt( SENDINFO( m_nOverrideClip1 ) )
#endif
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( ZMBaseWeapon, DT_ZM_BaseWeapon )

BEGIN_PREDICTION_DATA( CZMBaseWeapon )
END_PREDICTION_DATA()

#ifndef CLIENT_DLL
BEGIN_DATADESC( CZMBaseWeapon )
    DEFINE_KEYFIELD( m_OverrideViewModel, FIELD_MODELNAME, "v_modeloverride" ),
    DEFINE_KEYFIELD( m_OverrideWorldModel, FIELD_MODELNAME, "w_modeloverride" ),

    DEFINE_KEYFIELD( m_nOverrideDamage, FIELD_INTEGER, "dmgoverride" ),
    DEFINE_KEYFIELD( m_nOverrideClip1, FIELD_INTEGER, "clip1override" ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( weapon_zm_base, CZMBaseWeapon );
#endif

CZMBaseWeapon::CZMBaseWeapon()
{
	SetPredictionEligible( true );
	AddSolidFlags( FSOLID_TRIGGER ); // Nothing collides with these but it gets touches.


    SetSlotFlag( ZMWEAPONSLOT_NONE );

#ifndef CLIENT_DLL
    m_OverrideViewModel = m_OverrideWorldModel = NULL_STRING;
    m_nOverrideDamage = -1;
    m_nOverrideClip1 = -1;
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


    bool ret = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );

    if ( ret )
    {
        GetPlayerOwner()->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
    }

    return ret;
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

#ifndef CLIENT_DLL
    modinfo.m_flDamage = ( m_nOverrideDamage > -1 ) ? (float)m_nOverrideDamage : GetWpnData().m_flDamage;
#else
    modinfo.m_flDamage = GetWpnData().m_flDamage;
#endif
    
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
    pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );


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

#ifndef CLIENT_DLL
    PlayAISound();
#endif
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

void CZMBaseWeapon::SecondaryAttack( void )
{
    if ( !CanAct() ) return;


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
            if ( GetWpnData().m_bUseHands )
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

#ifndef CLIENT_DLL
void CZMBaseWeapon::Precache()
{
    BaseClass::Precache();


    // Make sure we precache all models.
    // It's possible to only precache the override model and the default one is left out.
    if ( GetWpnData().szViewModel[0] != NULL )
    {
        PrecacheModel( GetWpnData().szViewModel );
    }

    if ( GetWpnData().szWorldModel[0] != NULL )
    {
        PrecacheModel( GetWpnData().szWorldModel );
    }

    if ( m_OverrideViewModel != NULL_STRING )
    {
        PrecacheModel( STRING( m_OverrideViewModel ) );
    }

    if ( m_OverrideWorldModel != NULL_STRING )
    {
        PrecacheModel( STRING( m_OverrideWorldModel ) );
    }
}
#else
void CZMBaseWeapon::Spawn()
{
    BaseClass::Spawn();

    SetNextClientThink( gpGlobals->curtime + 0.1f );
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

void CZMBaseWeapon::GetGlowEffectColor( float& r, float& g, float& b )
{
    CSplitString split( zm_cl_glow_weapon.GetString(), " " );

    if ( split.Count() > 0 ) r = atof( split[0] );
    if ( split.Count() > 1 ) g = atof( split[1] );
    if ( split.Count() > 2 ) b = atof( split[2] );
}

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

CZMPlayer* CZMBaseWeapon::GetPlayerOwner() const
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

int CZMBaseWeapon::GetMaxClip1() const
{
    if ( m_nOverrideClip1 >= 0 )
    {
        return m_nOverrideClip1;
    }

    return BaseClass::GetMaxClip1();
}

const char* CZMBaseWeapon::GetViewModel( int vmIndex ) const
{
#ifndef CLIENT_DLL
    if ( m_OverrideViewModel != NULL_STRING )
    {
        return STRING( m_OverrideViewModel );
    }
#endif

    return BaseClass::GetViewModel( vmIndex );
}

const char* CZMBaseWeapon::GetWorldModel() const
{
#ifndef CLIENT_DLL
    if ( m_OverrideWorldModel != NULL_STRING )
    {
        return STRING( m_OverrideWorldModel );
    }
#endif

    return BaseClass::GetWorldModel();
}

void CZMBaseWeapon::SetViewModel()
{
    CZMPlayer* pOwner = GetPlayerOwner();
    if ( !pOwner ) return;

    CBaseViewModel* vm = pOwner->GetViewModel( m_nViewModelIndex, false );
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

    vm->SetWeaponModel( pszModel, this );
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
            // Fixes weapon still being constrained when dropped.
            // The constraint is destroyed when picked up but this re-creates it.

            // I'm leaving this piece of code here to remind Valve how shameful it is.
            // Yeah, let's just create a constraint and NOT USE m_pConstraint to reference it, I bet the constraint isn't even freed anywhere.
            // I know, people make mistakes and shit happens, but I spent way too much fucking time debugging this shit.
            // Thanks, Valve.

            /*
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
            */
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
// Version of cl_bob* cvars that are actually useful...
ConVar cl_bobcycle( "cl_bobcycle", "0.45", 0 , "How fast the bob cycles", true, 0.01f, false, 0.0f );
ConVar cl_bobup( "cl_bobup", "0.5", 0 , "Don't change...", true, 0.01f, true, 0.99f );
ConVar cl_bobvertscale( "cl_bobvertscale", "0.6", 0, "Vertical scale" ); // Def. is 0.1
ConVar cl_boblatscale( "cl_boblatscale", "0.8", 0, "Lateral scale" );

extern float g_lateralBob;
extern float g_verticalBob;

float CZMBaseWeapon::CalcViewmodelBob( void )
{
    static float    bobtime;
    static float    lastbobtime;
    float           cycle;

    float           bobup = cl_bobup.GetFloat();
    float           bobcycle = cl_bobcycle.GetFloat();
    

    C_BasePlayer* player = GetPlayerOwner();

    //NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

    if (!player ||
        !gpGlobals->frametime ||
        bobcycle <= 0.0f ||
        bobup <= 0.0f || bobup >= 1.0f)
    {
        return 0.0f;
    }


    float speed = player->GetLocalVelocity().Length2D();

    speed = clamp( speed, -320, 320 );

    float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );
    
    bobtime += ( gpGlobals->curtime - lastbobtime ) * bob_offset;
    lastbobtime = gpGlobals->curtime;

    //Calculate the vertical bob
    cycle = bobtime - (int)(bobtime/bobcycle)*bobcycle;
    cycle /= bobcycle;

    if ( cycle < bobup )
    {
        cycle = M_PI * cycle / bobup;
    }
    else
    {
        cycle = M_PI + M_PI*(cycle-bobup)/(1.0 - bobup);
    }
    
    g_verticalBob = speed*0.005f;
    g_verticalBob = g_verticalBob*0.3 + g_verticalBob*0.7*sin(cycle);

    g_verticalBob = clamp( g_verticalBob, -7.0f, 4.0f );

    //Calculate the lateral bob
    cycle = bobtime - (int)(bobtime/bobcycle*2)*bobcycle*2;
    cycle /= bobcycle*2;

    if ( cycle < bobup )
    {
        cycle = M_PI * cycle / bobup;
    }
    else
    {
        cycle = M_PI + M_PI*(cycle-bobup)/(1.0 - bobup);
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
    VectorMA( origin, g_verticalBob * cl_bobvertscale.GetFloat(), forward, origin );
    
    // Z bob a bit more
    origin[2] += g_verticalBob * 0.1f;
    
    // bob the angles
    angles[ ROLL ]	+= g_verticalBob * 0.5f;
    angles[ PITCH ]	-= g_verticalBob * 0.4f;

    angles[ YAW ]	-= g_lateralBob  * 0.3f;

    VectorMA( origin, g_lateralBob * cl_boblatscale.GetFloat(), right, origin );
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

bool CZMBaseWeapon::CanAct() const
{
    CBaseCombatCharacter* pOwner = GetOwner();

    if ( !pOwner )
        return false;

    if ( pOwner->GetMoveType() == MOVETYPE_LADDER )
        return false;


    return true;
}
