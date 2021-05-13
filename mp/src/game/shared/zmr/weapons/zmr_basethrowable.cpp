#include "cbase.h"
#include "in_buttons.h"
#include "npcevent.h"
#include "basegrenade_shared.h"


#include "zmr_basethrowable.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_NETWORKCLASS_ALIASED( ZMBaseThrowableWeapon, DT_ZM_BaseThrowableWeapon )

BEGIN_NETWORK_TABLE( CZMBaseThrowableWeapon, DT_ZM_BaseThrowableWeapon )
#ifdef CLIENT_DLL
    RecvPropInt( RECVINFO( m_iThrowState ) ),
#else
    SendPropInt( SENDINFO( m_iThrowState ), Q_log2( MOLOTOVSTATE_MAX ) + 1, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMBaseThrowableWeapon )
END_PREDICTION_DATA()
#endif

CZMBaseThrowableWeapon::CZMBaseThrowableWeapon()
{
    m_iThrowState = THROWSTATE_IDLE;
}

CZMBaseThrowableWeapon::~CZMBaseThrowableWeapon()
{
}

void CZMBaseThrowableWeapon::Precache()
{
    BaseClass::Precache();

#ifndef CLIENT_DLL
    UTIL_PrecacheOther( GetProjectileClassname() );
#endif
}

void CZMBaseThrowableWeapon::ItemPostFrame()
{
    CZMPlayer* pPlayer = GetPlayerOwner();

    if ( !pPlayer ) return;


    // We've thrown the grenade, remove our weapon.
#ifndef CLIENT_DLL
    if ( GetThrowState() == THROWSTATE_THROWN && IsViewModelSequenceFinished() )
    {
        pPlayer->Weapon_Drop( this, nullptr, nullptr );

        UTIL_Remove( this );
        return;
    }
#endif


    if ( GetThrowState() == THROWSTATE_IDLE && pPlayer->m_nButtons & IN_ATTACK && m_flNextPrimaryAttack <= gpGlobals->curtime )
    {
        PrimaryAttack();
    }

    // We've finished our drawback animation, now we're ready to throw.
    if ( GetThrowState() == THROWSTATE_DRAW_BACK && IsViewModelSequenceFinished() )
    {
        SetThrowState( THROWSTATE_READYTOTHROW );
    }


    if ( GetThrowState() == THROWSTATE_READYTOTHROW && !(pPlayer->m_nButtons & IN_ATTACK) )
    {
        SendWeaponAnim( ACT_VM_THROW );
        SetThrowState( THROWSTATE_THROWN );

        auto HasAnimEvent = [&]( int iAnimEvent ) {
            return GetFirstInstanceOfAnimEventTime( GetSequence(), iAnimEvent, false ) != -1.0f;
        };

        // We don't have an animation event for throwing. Just throw now!
        if ( !HasAnimEvent( EVENT_WEAPON_THROW ) && !HasAnimEvent( EVENT_WEAPON_THROW2 ) && !HasAnimEvent( EVENT_WEAPON_THROW3 ) )
        {
            Throw( pPlayer );
        }
    }
}

bool CZMBaseThrowableWeapon::Deploy()
{
    bool ret = BaseClass::Deploy();

    if ( ret )
    {
        SetThrowState( THROWSTATE_IDLE );
    }

    return ret;
}

void CZMBaseThrowableWeapon::Equip( CBaseCombatCharacter* pCharacter )
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
void CZMBaseThrowableWeapon::Drop( const Vector& vecVelocity )
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( pPlayer )
    {
        pPlayer->RemoveAmmo( 1, GetPrimaryAmmoType() );
    }

    BaseClass::Drop( vecVelocity );
}

void CZMBaseThrowableWeapon::Operator_HandleAnimEvent( animevent_t* pEvent, CBaseCombatCharacter* pOperator )
{
	switch( pEvent->event )
	{
    case EVENT_WEAPON_THROW :
    case EVENT_WEAPON_THROW2 :
    case EVENT_WEAPON_THROW3 :
        Throw( GetPlayerOwner() );
        break;
	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#endif

const CZMBaseThrowableConfig* CZMBaseThrowableWeapon::GetBaseThrowableConfig() const
{
    return static_cast<const CZMBaseThrowableConfig*>( GetWeaponConfig() );
}

const char* CZMBaseThrowableWeapon::GetProjectileClassname() const
{
    return "";
}

Vector CZMBaseThrowableWeapon::GetThrowPos( CZMPlayer* pPlayer )
{
    Assert( pPlayer );


    trace_t trace;
    Vector fwd;

    AngleVectors( pPlayer->EyeAngles(), &fwd );
    fwd[2] += 0.1f;

    Vector maxs = Vector( 6, 6, 6 );

    UTIL_TraceHull( pPlayer->EyePosition(), pPlayer->EyePosition() + fwd * 18.0f, -maxs, maxs, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &trace );


    return trace.endpos;
}

Vector CZMBaseThrowableWeapon::GetThrowDirection( CZMPlayer* pPlayer )
{
    Assert( pPlayer );

    Vector fwd;
    AngleVectors( pPlayer->EyeAngles(), &fwd );
    fwd[2] += 0.1f;

    return fwd;
}

float CZMBaseThrowableWeapon::GetThrowVelocity() const
{
    return GetBaseThrowableConfig()->flThrowVelocity;
}

QAngle CZMBaseThrowableWeapon::GetThrowAngularVelocity() const
{
    auto& min = GetBaseThrowableConfig()->vecAngularVel_Min;
    auto& max = GetBaseThrowableConfig()->vecAngularVel_Max;

    return QAngle( random->RandomFloat( min.x, max.x ), random->RandomFloat( min.y, max.y ), random->RandomFloat( min.z, max.z ) );
}

void CZMBaseThrowableWeapon::Throw( CZMPlayer* pPlayer )
{
    SetThrowState( THROWSTATE_THROWN );


    if ( !pPlayer ) return;


#ifndef CLIENT_DLL
    Vector pos = GetThrowPos( pPlayer );
    Vector vel = pPlayer->GetAbsVelocity() + GetThrowDirection( pPlayer ) * GetThrowVelocity();

    auto* pszProjectileClassname = GetProjectileClassname();

    auto* pProjectile = CBaseEntity::Create( pszProjectileClassname, pos, vec3_angle, pPlayer );

    if ( !pProjectile )
    {
        Warning( "Couldn't create grenade entity '%s'!\n", pszProjectileClassname );
        return;
    }

    AngularImpulse impulse;
    QAngleToAngularImpulse( GetThrowAngularVelocity(), impulse );

    //pProjectile->SetThrower( pPlayer );
    pProjectile->SetOwnerEntity( pPlayer );
    pProjectile->ApplyAbsVelocityImpulse( vel );
    pProjectile->m_takedamage = DAMAGE_EVENTS_ONLY;
    pProjectile->ApplyLocalAngularVelocityImpulse( impulse );

    PostThrow( pPlayer, pProjectile );
#endif

    pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

    //WeaponSound( SINGLE );
}

void CZMBaseThrowableWeapon::PostThrow( CZMPlayer* pPlayer, CBaseEntity* pProjectile )
{
    auto* pGrenade = dynamic_cast<CBaseGrenade*>( pProjectile );
    if ( pGrenade )
    {
        pGrenade->SetDamage( GetBaseThrowableConfig()->flProjectileDamage );
        pGrenade->SetDamageRadius( GetBaseThrowableConfig()->flProjectileRadius );

        pGrenade->SetThrower( pPlayer );
    }
}

//
//
//
CZMBaseThrowableConfig::CZMBaseThrowableConfig( const char* wepname, const char* configpath ) : CZMBaseWeaponConfig( wepname, configpath )
{
    flThrowVelocity = 100.0f;
    flProjectileDamage = 100.0f;
    flProjectileRadius = 100.0f;

    vecAngularVel_Min = vec3_origin;
    vecAngularVel_Max = vec3_origin;
}

void CZMBaseThrowableConfig::LoadFromConfig( KeyValues* kv )
{
    CZMBaseWeaponConfig::LoadFromConfig( kv );

    flThrowVelocity = kv->GetFloat( "throw_velocity", 100.0f );
    flProjectileDamage = kv->GetFloat( "projectile_damage", 100.0f );
    flProjectileRadius = kv->GetFloat( "projectile_radius", 100.0f );

    CopyVector( kv->GetString( "angvel_min" ), vecAngularVel_Min );
    CopyVector( kv->GetString( "angvel_max" ), vecAngularVel_Max );
}

KeyValues* CZMBaseThrowableConfig::ToKeyValues() const
{
    auto* kv = CZMBaseWeaponConfig::ToKeyValues();

    kv->SetFloat( "throw_velocity", flThrowVelocity );
    kv->SetFloat( "projectile_damage", flProjectileDamage );
    kv->SetFloat( "projectile_radius", flProjectileRadius );

    VectorToKv( kv, "angvel_min", vecAngularVel_Min );
    VectorToKv( kv, "angvel_max", vecAngularVel_Max );

    return kv;
}
