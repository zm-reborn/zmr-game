#pragma once

#include "zmr_basemelee.h"
#include "zmr_weaponconfig.h"



#ifdef CLIENT_DLL
#define CZMWeaponSledge C_ZMWeaponSledge
#endif


class CZMSledgeConfig : public ZMWeaponConfig::CZMBaseWeaponConfig
{
public:
    CZMSledgeConfig( const char* wepname, const char* configpath ) : CZMBaseWeaponConfig( wepname, configpath )
    {
        flPrimaryRandomMultMin = 1.0f;
        flPrimaryRandomMultMax = 1.0f;

        flSecondaryRandomMultMin = 1.0f;
        flSecondaryRandomMultMax = 1.0f;
    }

    virtual void LoadFromConfig( KeyValues* kv ) OVERRIDE
    {
        CZMBaseWeaponConfig::LoadFromConfig( kv );

        KeyValues* inner;

        inner = kv->FindKey( "PrimaryAttack" );
        if ( inner )
        {
            flPrimaryRandomMultMin = inner->GetFloat( "sledge_min_mult" );
            flPrimaryRandomMultMax = inner->GetFloat( "sledge_max_mult" );
        }

        inner = kv->FindKey( "SecondaryAttack" );
        if ( inner )
        {
            flSecondaryRandomMultMin = inner->GetFloat( "sledge_min_mult" );
            flSecondaryRandomMultMax = inner->GetFloat( "sledge_max_mult" );
        }
    }

    virtual bool OverrideFromConfig( KeyValues* kv ) OVERRIDE
    {
        auto ret = CZMBaseWeaponConfig::OverrideFromConfig( kv );

        KeyValues* inner;

        inner = kv->FindKey( "PrimaryAttack" );
        if ( inner )
        {
            OVERRIDE_FROM_WEPCONFIG_F( inner, sledge_min_mult, flPrimaryRandomMultMin );
            OVERRIDE_FROM_WEPCONFIG_F( inner, sledge_max_mult, flPrimaryRandomMultMax );
        }

        inner = kv->FindKey( "SecondaryAttack" );
        if ( inner )
        {
            OVERRIDE_FROM_WEPCONFIG_F( inner, sledge_min_mult, flSecondaryRandomMultMin );
            OVERRIDE_FROM_WEPCONFIG_F( inner, sledge_max_mult, flSecondaryRandomMultMax );
        }

        return ret;
    }

    virtual KeyValues* ToKeyValues() const OVERRIDE
    {
        auto* kv = CZMBaseWeaponConfig::ToKeyValues();

        KeyValues* inner;

        inner = kv->FindKey( "PrimaryAttack" );
        if ( inner )
        {
            inner->SetFloat( "sledge_min_mult", flPrimaryRandomMultMin );
            inner->SetFloat( "sledge_max_mult", flPrimaryRandomMultMax );
        }

        inner = kv->FindKey( "SecondaryAttack" );
        if ( inner )
        {
            inner->SetFloat( "sledge_min_mult", flSecondaryRandomMultMin );
            inner->SetFloat( "sledge_max_mult", flSecondaryRandomMultMax );
        }

        return kv;
    }

    
    float flPrimaryRandomMultMin;
    float flPrimaryRandomMultMax;

    float flSecondaryRandomMultMin;
    float flSecondaryRandomMultMax;
};


class CZMWeaponSledge : public CZMBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CZMWeaponSledge, CZMBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMWeaponSledge();


    virtual void PrimaryAttack() OVERRIDE;
    virtual void SecondaryAttack() OVERRIDE;


    const CZMSledgeConfig* GetSledgeConfig() const;


    virtual bool CanSecondaryAttack() const OVERRIDE;
    virtual WeaponSound_t GetSecondaryAttackSound() const;

    virtual bool UsesAnimEvent( bool bSecondary ) const OVERRIDE;


    virtual float GetDamageForActivity( Activity act ) const OVERRIDE;

    virtual void Hit( trace_t& traceHit, Activity iHitActivity ) OVERRIDE;
};
