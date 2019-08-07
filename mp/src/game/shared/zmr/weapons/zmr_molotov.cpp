#include "cbase.h"

#include "npcevent.h"
#include "eventlist.h"

#include "in_buttons.h"

#ifndef CLIENT_DLL
#include "fire.h"
#include "smoke_trail.h"
#include "gib.h"
#include "props.h"
#endif

#include "zmr/zmr_shareddefs.h"
#include "zmr_base.h"


#include "zmr/zmr_player_shared.h"




#ifndef CLIENT_DLL
#include "basegrenade_shared.h"

class CFlamingGib : public CGib
{
public:
    DECLARE_CLASS( CFlamingGib, CGib );
    
    void Spawn()
    {
        const char* gibname = g_PropDataSystem.GetRandomChunkModel( "MetalChunks" );

        BaseClass::Spawn( gibname );


        SetBloodColor( DONT_BLEED );
        m_takedamage = DAMAGE_NO;

        SetCollisionGroup( COLLISION_GROUP_DEBRIS );
        AddEffects( EF_NODRAW );
        




        SetNextThink( gpGlobals->curtime + m_lifeTime );
        SetThink( &CGib::DieThink );
        


        QAngle ang;
        Vector vel;
        ang.x = random->RandomFloat( -70.0f, 20.0f );
        ang.y = random->RandomFloat( 0.0f, 360.0f );
        ang.z = 0;
        AngleVectors( ang, &vel );

        vel *= random->RandomFloat( 10.0f, 20.0f );

        SetAbsVelocity( vel );


        IPhysicsObject* pPhys = VPhysicsInitNormal( SOLID_NONE, GetSolidFlags(), false );

        if ( pPhys )
        {
            AngularImpulse angimp = RandomAngularImpulse( -90.0f, 90.0f );



            pPhys->EnableMotion( true );
            pPhys->SetVelocity(
                &vel,
                &angimp );
        }

        CFireTrail* trail = CFireTrail::CreateFireTrail();

        if ( trail )
        {
            trail->FollowEntity( this, "" );
            trail->SetParent( this, 0 );
            trail->SetLocalOrigin( vec3_origin );
            trail->SetMoveType( MOVETYPE_NONE );
            trail->SetLifetime( this->m_lifeTime - 1.0f );
            //trail->m_StartSize = 15; //TGB: made the chunks smaller
            //trail->m_EndSize = 6;  
        }
    }
    void Think() OVERRIDE { SetNextThink( gpGlobals->curtime ); BaseClass::Think(); };
};

LINK_ENTITY_TO_CLASS( flaming_gib, CFlamingGib );


class CZMGrenadeMolotov : public CBaseGrenade
{
public:
    DECLARE_CLASS( CZMGrenadeMolotov, CBaseGrenade );
    DECLARE_DATADESC();


    void Spawn() OVERRIDE;
    void Precache() OVERRIDE;

    void MolotovTouch( CBaseEntity* pOther );
    void MolotovThink();
    void Detonate() OVERRIDE;

    void CreateFlyingChunk( const Vector& pos );

private:
    SmokeTrail* m_pFireTrail;
};

#ifndef CLIENT_DLL
BEGIN_DATADESC( CZMGrenadeMolotov )
    DEFINE_FIELD( m_pFireTrail, FIELD_CLASSPTR ),

    DEFINE_ENTITYFUNC( MolotovTouch ),
    DEFINE_THINKFUNC( MolotovThink ),
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( grenade_molotov, CZMGrenadeMolotov );

void CZMGrenadeMolotov::Precache()
{
    PrecacheModel( "models/weapons/molotov3rd_zm.mdl" );

    PrecacheScriptSound( "Grenade_Molotov.Detonate" );
    PrecacheScriptSound( "Grenade_Molotov.Detonate2" );
}

void CZMGrenadeMolotov::Spawn()
{
    Precache();

    SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
    SetSolid( SOLID_BBOX ); 
    SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
    RemoveEffects( EF_NOINTERP );

    SetModel( "models/weapons/molotov3rd_zm.mdl" );


    SetTouch( &CZMGrenadeMolotov::MolotovTouch );
    SetThink( &CZMGrenadeMolotov::MolotovThink );


    CFireTrail* fire = CFireTrail::CreateFireTrail();

    if ( fire )
    {
        fire->FollowEntity( this, "flame" );
        fire->SetLocalOrigin( vec3_origin );
        fire->SetMoveType( MOVETYPE_NONE );
        fire->SetLifetime( 20.0f ); 
        //fire->m_StartSize = random->RandomFloat(1.0f, 2.0f);
        //fire->m_EndSize = random->RandomFloat(3.5f, 5.0f);
    }
}

void CZMGrenadeMolotov::MolotovTouch( CBaseEntity* pOther )
{
    if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER|FSOLID_VOLUME_CONTENTS ) )
        return;


    Detonate();
}

void CZMGrenadeMolotov::MolotovThink()
{
    if ( UTIL_PointContents( GetAbsOrigin() ) & CONTENTS_WATER )
    {
        UTIL_Remove( this );
        return;
    }

    SetNextThink( gpGlobals->curtime + 0.1f );
}

void CZMGrenadeMolotov::Detonate()
{
    AddEffects( EF_NODRAW );


    // Travel back to the touch position.
    trace_t trace;
    trace = GetTouchTrace();

    Vector normal( 0.0f, 0.0f, 1.0f );

    if ( trace.fraction != 1.0f )
    {
        normal = trace.plane.normal;

        // Go off the wall a bit.
        SetAbsOrigin( trace.endpos + normal * 2.0f );
    }
    

    if ( UTIL_PointContents( GetAbsOrigin() ) & CONTENTS_WATER )
    {
        UTIL_Remove( this );
        return;
    }


    EmitSound( "Grenade_Molotov.Detonate" );
    EmitSound( "Grenade_Molotov.Detonate2" );

    // ZMRTODO: Fix flying chunks.
    //for ( int i = 0; i < 5; i++ )
    //{
    //    CreateFlyingChunk( GetAbsOrigin() );
    //}

    CBaseCombatCharacter* pThrower = GetThrower();

    Vector start, end;
    Vector fwd;
    QAngle ang;

    trace_t firetrace;
    start = GetAbsOrigin() + normal * 48.0f;
    for ( int i = 0; i < 5; i++ )
    {
        ang.x = random->RandomFloat( 45.0f, 135.0f );
        ang.y = random->RandomFloat( 0.0f, 360.0f );
        ang.z = 0.0f;

        AngleVectors( ang, &fwd );

        end = start + fwd * 300.0f;


        UTIL_TraceLine( start, end, MASK_SOLID, this, COLLISION_GROUP_NONE, &firetrace );

        if ( firetrace.fraction != 1.0f )
        {
            FireSystem_StartFire( firetrace.endpos + ( firetrace.plane.normal * 12.0f ), 125.0f, 128.0f, 10.0f, (SF_FIRE_START_ON), pThrower, FIRE_NATURAL );
        }
        else
        {
            FireSystem_StartFire( firetrace.endpos, 125.0f, 128.0f, 20.0f, (SF_FIRE_START_ON|SF_FIRE_SMOKELESS), pThrower, FIRE_NATURAL );
        }
    }


    UTIL_ScreenShake( GetAbsOrigin(), 10.0f, 60.0f, 1.0f, 200.0f, SHAKE_START, true );


    if ( pThrower )
    {
        RadiusDamage( CTakeDamageInfo( this, pThrower, 40.0f, DMG_BURN ), GetAbsOrigin(), 128.0f, CLASS_NONE, NULL );
    }


    UTIL_Remove( this );
}

void CZMGrenadeMolotov::CreateFlyingChunk( const Vector& pos )
{
    CFlamingGib* pGib = CREATE_ENTITY( CFlamingGib, "flaming_gib" );

    if ( !pGib )
    {
        Warning( "Couldn't create molotov gib!\n" );
        return;
    }

    pGib->m_lifeTime = 15.0f;
    pGib->Spawn();

    pGib->SetAbsOrigin( pos );
    pGib->SetOwnerEntity( this );
}
#endif

#define MOLOTOV_RADIUS      4.0f


#define MOLOTOVSTATE_THROWN         -1
#define MOLOTOVSTATE_IDLE           0
#define MOLOTOVSTATE_DRAWBACK       4
#define MOLOTOVSTATE_READYTOTHROW   5

#define MOLOTOV_FIRE_SPRITE         "sprites/fire_vm_grey.vmt"

#ifdef CLIENT_DLL
#define CZMWeaponMolotov C_ZMWeaponMolotov
#endif

class CZMWeaponMolotov : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMWeaponMolotov, CZMBaseWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

    CZMWeaponMolotov();
    ~CZMWeaponMolotov();

    CNetworkVar( int, m_iThrowState );

#ifndef CLIENT_DLL
    void Operator_HandleAnimEvent( animevent_t*, CBaseCombatCharacter* ) OVERRIDE;
    void HandleAnimEvent( animevent_t *pEvent ) OVERRIDE;
#else
    bool OnFireEvent( C_BaseViewModel*, const Vector&, const QAngle&, int event, const char* ) OVERRIDE;


    //void ViewModelDrawn( C_BaseViewModel* ) OVERRIDE;

    //CMaterialReference m_matClothFlame;


    //bool m_bClothFlame;
#endif


    void Precache() OVERRIDE;
    void PrimaryAttack() OVERRIDE;
    void Equip( CBaseCombatCharacter* pCharacter ) OVERRIDE;
    bool Deploy() OVERRIDE;
    bool Holster( CBaseCombatWeapon* pSwitchTo = nullptr ) OVERRIDE;
    bool CanHolster() const OVERRIDE { return m_iThrowState < MOLOTOVSTATE_DRAWBACK && m_iThrowState >= MOLOTOVSTATE_IDLE; }
    bool CanBeDropped() const OVERRIDE { return CanHolster(); }
    
#ifndef CLIENT_DLL
    void Drop( const Vector& vecVelocity ) OVERRIDE;
#endif
    
    void ItemPostFrame() OVERRIDE;

    void HandleAnimEventLight();
    void HandleAnimEventThrow();
    

    void Throw( CZMPlayer* pPlayer );
    void GetThrowPos( CZMPlayer* pPlayer, Vector& outpos );

#ifdef CLIENT_DLL
    HPARTICLEFFECT m_hClothFlameParticle;

    void DestroyClothFlameParticle();
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponMolotov, DT_ZM_WeaponMolotov )

BEGIN_NETWORK_TABLE( CZMWeaponMolotov, DT_ZM_WeaponMolotov )
#ifdef CLIENT_DLL
    RecvPropInt( RECVINFO( m_iThrowState ) ),
#else
    SendPropInt( SENDINFO( m_iThrowState ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponMolotov )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_molotov, CZMWeaponMolotov );
PRECACHE_WEAPON_REGISTER( weapon_zm_molotov );

acttable_t	CZMWeaponMolotov::m_acttable[] = 
{
    /*
    { ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_GRENADE,					false },
    { ACT_HL2MP_RUN,					ACT_HL2MP_RUN_GRENADE,					false },
    { ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },
    { ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_GRENADE,			false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },
    { ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },
    { ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_GRENADE,					false },
    */
    { ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_GRENADE,					false },
    { ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },
    { ACT_MP_RUN,					    ACT_HL2MP_RUN_GRENADE,					false },
    { ACT_MP_CROUCHWALK,			    ACT_HL2MP_WALK_CROUCH_GRENADE,			false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },
    { ACT_MP_RELOAD_STAND,			    ACT_HL2MP_GESTURE_RELOAD_GRENADE,       false },
    { ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_GRENADE,       false },
    { ACT_MP_JUMP,					    ACT_HL2MP_JUMP_GRENADE,					false },
};
IMPLEMENT_ACTTABLE( CZMWeaponMolotov );

CZMWeaponMolotov::CZMWeaponMolotov()
{
    SetSlotFlag( ZMWEAPONSLOT_EQUIPMENT );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_MOLOTOV );


#ifdef CLIENT_DLL
    //m_bClothFlame = false;
    m_hClothFlameParticle = nullptr;
#endif
}

CZMWeaponMolotov::~CZMWeaponMolotov()
{
#ifdef CLIENT_DLL
    DestroyClothFlameParticle();
#endif
}

void CZMWeaponMolotov::Precache()
{
    BaseClass::Precache();
    
#ifndef CLIENT_DLL
    UTIL_PrecacheOther( "grenade_molotov" );
#endif

    PrecacheMaterial( MOLOTOV_FIRE_SPRITE );
    PrecacheParticleSystem( "molotov_clothflame" );
}

bool CZMWeaponMolotov::Deploy()
{
    bool ret = BaseClass::Deploy();

    if ( ret )
    {
        m_iThrowState = MOLOTOVSTATE_IDLE;


        // Remove lighter flame.
        CZMPlayer* pPlayer = GetPlayerOwner();
        if ( pPlayer && pPlayer->GetViewModel() )
        {
            pPlayer->GetViewModel()->SetBodygroup( 1, 0 );
        }
    }

    return ret;
}

bool CZMWeaponMolotov::Holster( CBaseCombatWeapon* pSwitchTo )
{
    bool ret = BaseClass::Holster( pSwitchTo );

    if ( ret )
    {
#ifdef CLIENT_DLL
        DestroyClothFlameParticle();
#endif
    }

    return ret;
}

#ifdef CLIENT_DLL
void CZMWeaponMolotov::DestroyClothFlameParticle()
{
    if ( m_hClothFlameParticle )
    {
        m_hClothFlameParticle->StopEmission();
        m_hClothFlameParticle = nullptr;
    }
}
#endif

void CZMWeaponMolotov::Equip( CBaseCombatCharacter* pCharacter )
{
    BaseClass::Equip( pCharacter );

#ifndef CLIENT_DLL
    if ( pCharacter && GetOwner() == pCharacter && pCharacter->GetAmmoCount( GetPrimaryAmmoType() ) < 1 )
    {
        pCharacter->GiveAmmo( 1, GetPrimaryAmmoType(), true );
    }
#endif
}

#ifndef CLIENT_DLL
void CZMWeaponMolotov::Drop( const Vector& vecVelocity )
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( pPlayer )
    {
        pPlayer->RemoveAmmo( 1, GetPrimaryAmmoType() );
    }

    BaseClass::Drop( vecVelocity );
}
#endif

void CZMWeaponMolotov::ItemPostFrame()
{
    CZMPlayer* pPlayer = GetPlayerOwner();

    if ( !pPlayer ) return;


    // We've thrown the grenade, remove our weapon.
#ifndef CLIENT_DLL
    if ( m_iThrowState == MOLOTOVSTATE_THROWN && IsViewModelSequenceFinished() )
    {
        pPlayer->Weapon_Drop( this, nullptr, nullptr );

        UTIL_Remove( this );
        return;
    }
#endif


    if ( pPlayer->m_nButtons & IN_ATTACK && m_flNextPrimaryAttack <= gpGlobals->curtime && m_iThrowState == MOLOTOVSTATE_IDLE )
    {
        PrimaryAttack();
    }


    if ( m_iThrowState >= MOLOTOVSTATE_READYTOTHROW && !(pPlayer->m_nButtons & IN_ATTACK) )
    {
        SendWeaponAnim( ACT_VM_THROW );
        m_iThrowState = MOLOTOVSTATE_THROWN; // Reset our state so we don't keep trying to send the throw animation each frame.
    }
}

void CZMWeaponMolotov::PrimaryAttack()
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;

    if ( pPlayer->GetWaterLevel() >= 2 ) return;


    m_iThrowState = 1;

    SendWeaponAnim( ACT_VM_PRIMARYATTACK_1 );


    m_flNextPrimaryAttack = gpGlobals->curtime + 3.0f;
}

void CZMWeaponMolotov::HandleAnimEventLight()
{
    if ( m_iThrowState >= MOLOTOVSTATE_DRAWBACK )
    {
        // We've finished our drawback animation, now we're ready to throw.
        if ( m_iThrowState == MOLOTOVSTATE_DRAWBACK )
        {
            m_iThrowState = MOLOTOVSTATE_READYTOTHROW;
        }

        return;
    }


    CZMPlayer* pPlayer = GetPlayerOwner();

    if ( !pPlayer ) return;

    switch ( ++m_iThrowState )
    {
    case 1 :
    case 2 : SendWeaponAnim( ACT_VM_PRIMARYATTACK_2 ); break;
    case 3 : SendWeaponAnim( ACT_VM_PRIMARYATTACK_3 ); break;
    default : break;
    }


    if (m_iThrowState >= MOLOTOVSTATE_DRAWBACK ||
        (m_iThrowState < MOLOTOVSTATE_DRAWBACK
    &&  random->RandomInt( 0, 100 ) < min( pPlayer->GetHealth(), 100 )) )
    {
        m_iThrowState = MOLOTOVSTATE_DRAWBACK;

        SendWeaponAnim( ACT_VM_PRIMARYATTACK_4 );
    }

    DevMsg( "Set throw state to: %i\n", m_iThrowState );
}

void CZMWeaponMolotov::HandleAnimEventThrow()
{
    Throw( GetPlayerOwner() );
}

void CZMWeaponMolotov::Throw( CZMPlayer* pPlayer )
{
    if ( !pPlayer ) return;


#ifndef CLIENT_DLL
    Vector pos;
    GetThrowPos( pPlayer, pos );


    Vector fwd;
    AngleVectors( pPlayer->EyeAngles(), &fwd );
    fwd[2] += 0.1f;


    Vector vel = pPlayer->GetAbsVelocity() + fwd * 1200.0f;

    CZMGrenadeMolotov* pMolotov = (CZMGrenadeMolotov*)CBaseEntity::Create( "grenade_molotov", pos, vec3_angle, pPlayer );

    if ( !pMolotov )
    {
        Warning( "Couldn't create molotov entity!\n" );
        return;
    }


    pMolotov->SetThrower( pPlayer );
    pMolotov->SetOwnerEntity( pPlayer );
    pMolotov->SetAbsVelocity( vel );
    pMolotov->m_takedamage = DAMAGE_EVENTS_ONLY;
    pMolotov->SetLocalAngularVelocity( QAngle( 0, 0, random->RandomFloat( -100, -500 ) ) );
#endif

    pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

    //WeaponSound( SINGLE );
}

void CZMWeaponMolotov::GetThrowPos( CZMPlayer* pPlayer, Vector& outpos )
{
    if ( !pPlayer ) return;


    trace_t trace;
    Vector fwd;

    AngleVectors( pPlayer->EyeAngles(), &fwd );
    fwd[2] += 0.1f;

    Vector maxs = Vector( MOLOTOV_RADIUS, MOLOTOV_RADIUS, MOLOTOV_RADIUS );

    UTIL_TraceHull( pPlayer->EyePosition(), pPlayer->EyePosition() + fwd * 18.0f, -maxs, maxs, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &trace );


    outpos = trace.endpos;
}

#ifndef CLIENT_DLL
void CZMWeaponMolotov::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_SEQUENCE_FINISHED :
		HandleAnimEventLight();
		break;
    case EVENT_WEAPON_THROW :
    case EVENT_WEAPON_THROW2 :
    case EVENT_WEAPON_THROW3 :
        HandleAnimEventThrow();
        break;
	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

void CZMWeaponMolotov::HandleAnimEvent( animevent_t* pEvent )
{
    if ( pEvent->event == AE_ZM_LIGHTERFLAME )
    {
        CZMPlayer* pPlayer = GetPlayerOwner();

        if ( pPlayer )
        {
            CBaseViewModel* pVM = pPlayer->GetViewModel();

            if ( pVM )
            {
                // Toggle lighter flame on/off.
                int prevvalue = pVM->GetBodygroup( 1 );
                pVM->SetBodygroup( 1, prevvalue != 1 ? 1 : 0 );
            }
        }
    }
    else if ( pEvent->event == AE_ZM_CLOTHFLAME )
    {
#ifdef CLIENT_DLL
        //m_bClothFlame = true;
#endif
    }

    BaseClass::HandleAnimEvent( pEvent );
}
#else
bool CZMWeaponMolotov::OnFireEvent( C_BaseViewModel* pViewModel, const Vector& origin, const QAngle& angles, int event, const char* options )
{
	switch( event )
	{
	case EVENT_WEAPON_SEQUENCE_FINISHED :
		HandleAnimEventLight();
		return true;
    case EVENT_WEAPON_THROW :
    case EVENT_WEAPON_THROW2 :
    case EVENT_WEAPON_THROW3 :
        HandleAnimEventThrow();
        return true;
    case AE_ZM_LIGHTERFLAME :
        return true;
    case AE_ZM_CLOTHFLAME :
        // Toggle cloth flame.
        if ( m_hClothFlameParticle )
        {
            DestroyClothFlameParticle();
        }
        else
        {
            auto* pPlayer = GetPlayerOwner();
            CBaseViewModel* pVM = pPlayer ? pPlayer->GetViewModel() : nullptr;
            if ( pVM )
            {
                 m_hClothFlameParticle = pVM->ParticleProp()->Create( "molotov_clothflame", ParticleAttachment_t::PATTACH_POINT_FOLLOW, "clothflame" );
            }
        }


        return true;
	}

    return false;
}

/*bool UTIL_GetWeaponAttachment( C_BaseCombatWeapon *pWeapon, int attachmentID, Vector &absOrigin, QAngle &absAngles );

void CZMWeaponMolotov::ViewModelDrawn( C_BaseViewModel* pVM )
{
    if ( !IsWeaponVisible() || !m_bClothFlame )
    {
        BaseClass::ViewModelDrawn( pVM );
        return;
    }
    

    if ( !m_matClothFlame )
    {
        m_matClothFlame.Init( MOLOTOV_FIRE_SPRITE, TEXTURE_GROUP_CLIENT_EFFECTS );
    }

    if ( !m_matClothFlame ) return;


    int attachment = LookupAttachment( "clothflame" );

    Vector pos;
    QAngle ang;

    if ( attachment > 0 && UTIL_GetWeaponAttachment( this, attachment, pos, ang ) )
    {
        static float lastflamechange = 0.0f;

        int green = 100 - random->RandomInt( 0, 64 ); //The green channel deals with the yellow-redness
        color32 flamecolor = { 255, green, 0, 255 };

        //Resize...
        float w = 2.0f;
        float h = 4.0f;
        if (gpGlobals->curtime >= lastflamechange)
        {
            w = random->RandomFloat( 1.0f, 2.0f);
            h = random->RandomFloat( 3.8f, 4.2f);
            lastflamechange = gpGlobals->curtime + random->RandomFloat( 0.2f, 1.0f ); // Reflicker randomly
        }
        
        DrawSprite( pos, w, h, flamecolor );
    }

    BaseClass::ViewModelDrawn( pVM );
}*/
#endif
