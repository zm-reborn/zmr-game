#include "cbase.h"
#include "ammodef.h"
#include "glowstencil/c_glowbase.h"

#include "c_zmr_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_cl_glow_ammo( "zm_cl_glow_ammo", "1 .2 .2", FCVAR_ARCHIVE );
ConVar zm_cl_glow_ammo_enabled( "zm_cl_glow_ammo_enabled", "1", FCVAR_ARCHIVE );


extern ConVar zm_sv_glow_item_enabled;



class C_ZMAmmo : public C_BaseGlowEntity
{
public:
    DECLARE_CLASS( C_ZMAmmo, C_BaseGlowEntity );
    DECLARE_CLIENTCLASS();

    C_ZMAmmo();


    virtual void    Spawn() OVERRIDE;
    virtual void    ClientThink() OVERRIDE;


    void            UpdateGlow();
    virtual void    GetGlowEffectColor( float& r, float& g, float& b ) const OVERRIDE;
    
    virtual bool    GlowOccluded() const OVERRIDE { return false; };
    virtual bool    GlowUnoccluded() const OVERRIDE { return true; };

protected:
    CNetworkVar( int, m_iAmmoType );
    CNetworkVar( int, m_nAmmo );
    CNetworkVar( int, m_nMaxAmmo );
};

IMPLEMENT_CLIENTCLASS_DT( C_ZMAmmo, DT_ZM_Ammo, CZMAmmo )
    RecvPropInt( RECVINFO( m_iAmmoType ) ),
    RecvPropInt( RECVINFO( m_nAmmo ), SPROP_UNSIGNED ),
    RecvPropInt( RECVINFO( m_nMaxAmmo ), SPROP_UNSIGNED ),
END_RECV_TABLE()


C_ZMAmmo::C_ZMAmmo()
{
    m_iAmmoType = -1;
    m_nAmmo = 0;
    m_nMaxAmmo = 0;
}

void C_ZMAmmo::Spawn()
{
    BaseClass::Spawn();

    SetNextClientThink( gpGlobals->curtime + 0.1f );
}

void C_ZMAmmo::ClientThink()
{
    UpdateGlow();

    SetNextClientThink( gpGlobals->curtime + 0.1f );
}

void C_ZMAmmo::UpdateGlow()
{
    if ( !zm_cl_glow_ammo_enabled.GetBool() || !zm_sv_glow_item_enabled.GetBool() )
    {
        if ( IsClientSideGlowEnabled() )
            SetClientSideGlowEnabled( false );

        return;
    }


    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();

    if (pPlayer
    &&  pPlayer->IsHuman()
    &&  pPlayer->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) < ITEM_GLOW_DIST_SQR
    &&  pPlayer->GetWeaponForAmmo( m_iAmmoType ) != nullptr
    &&  pPlayer->GetAmmoCount( m_iAmmoType ) < GetAmmoDef()->MaxCarry( m_iAmmoType ) )
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

void C_ZMAmmo::GetGlowEffectColor( float& r, float& g, float& b ) const
{
    float clr[3];
    UTIL_ParseFloatColorFromString( zm_cl_glow_ammo.GetString(), clr, ARRAYSIZE( clr ) );

    r = clr[0];
    g = clr[1];
    b = clr[2];
}
