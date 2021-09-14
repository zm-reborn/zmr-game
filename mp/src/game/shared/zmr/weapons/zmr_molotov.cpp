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

#include "zmr_shareddefs.h"
#include "zmr_basethrowable.h"


#include "zmr_player_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define MOLOTOV_RADIUS      4.0f

#define MOLOTOV_FIRE_SPRITE         "sprites/fire_vm_grey.vmt"

#ifdef CLIENT_DLL
#define CZMWeaponMolotov C_ZMWeaponMolotov
#endif

class CZMWeaponMolotov : public CZMBaseThrowableWeapon
{
public:
	DECLARE_CLASS( CZMWeaponMolotov, CZMBaseThrowableWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMWeaponMolotov();
    ~CZMWeaponMolotov();

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
    bool Deploy() OVERRIDE;
    bool Holster( CBaseCombatWeapon* pSwitchTo = nullptr ) OVERRIDE;
    bool CanHolster() const OVERRIDE { return GetThrowState() < THROWSTATE_DRAW_BACK && GetThrowState() >= THROWSTATE_IDLE; }
    bool CanBeDropped() const OVERRIDE { return CanHolster(); }

    void HandleAnimEventLight();
    
    virtual const char* GetProjectileClassname() const OVERRIDE;

#ifdef CLIENT_DLL
    HPARTICLEFFECT m_hClothFlameParticle;

    void DestroyClothFlameParticle();
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponMolotov, DT_ZM_WeaponMolotov )

BEGIN_NETWORK_TABLE( CZMWeaponMolotov, DT_ZM_WeaponMolotov )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponMolotov )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_molotov, CZMWeaponMolotov );
PRECACHE_WEAPON_REGISTER( weapon_zm_molotov );

REGISTER_WEAPON_CONFIG( ZMWeaponConfig::ZMCONFIGSLOT_MOLOTOV, CZMBaseThrowableConfig );

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

    PrecacheMaterial( MOLOTOV_FIRE_SPRITE );
    PrecacheParticleSystem( "molotov_clothflame" );
}

bool CZMWeaponMolotov::Deploy()
{
    bool ret = BaseClass::Deploy();

    if ( ret )
    {
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
    if ( m_hClothFlameParticle != nullptr )
    {
        auto* pPlayer = GetPlayerOwner();
        auto* pVM = pPlayer ? pPlayer->GetViewModel() : nullptr;

        if ( pVM != nullptr )
        {
            Assert( pVM->ParticleProp()->FindEffect( m_hClothFlameParticle ) != -1 );
            pVM->ParticleProp()->StopEmission( m_hClothFlameParticle );
        }

        m_hClothFlameParticle = nullptr;
    }
}
#endif

void CZMWeaponMolotov::PrimaryAttack()
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;

    // No arming in water :(
    if ( pPlayer->GetWaterLevel() >= 2 ) return;

    
    SendWeaponAnim( ACT_VM_PRIMARYATTACK_1 );
    m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();

    SetThrowState( THROWSTATE_ARMING );
}

void CZMWeaponMolotov::HandleAnimEventLight()
{
    if ( GetThrowState() >= THROWSTATE_DRAW_BACK )
    {
        return;
    }


    CZMPlayer* pPlayer = GetPlayerOwner();

    if ( !pPlayer ) return;

    // HACK
    SetThrowState( (ZMThrowState_t)((int)GetThrowState() + 1) );
    switch ( (int)GetThrowState() )
    {
    case 1 :
    case 2 : SendWeaponAnim( ACT_VM_PRIMARYATTACK_2 ); break;
    case 3 : SendWeaponAnim( ACT_VM_PRIMARYATTACK_3 ); break;
    default : break;
    }


    if (GetThrowState() >= THROWSTATE_DRAW_BACK ||
        (GetThrowState() < THROWSTATE_DRAW_BACK
    &&  random->RandomInt( 0, 100 ) < MIN( pPlayer->GetHealth(), 100 )) )
    {
        SetThrowState( THROWSTATE_DRAW_BACK );

        SendWeaponAnim( ACT_VM_PRIMARYATTACK_4 );
    }

    DevMsg( "Set throw state to: %i\n", (int)GetThrowState() );
}

const char* CZMWeaponMolotov::GetProjectileClassname() const
{
    return "grenade_molotov";
}

#ifndef CLIENT_DLL
void CZMWeaponMolotov::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_SEQUENCE_FINISHED :
		HandleAnimEventLight();
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
    case AE_ZM_LIGHTERFLAME :
        return true;
    case AE_ZM_CLOTHFLAME :
        // Toggle cloth flame.
        if ( m_hClothFlameParticle != nullptr )
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
