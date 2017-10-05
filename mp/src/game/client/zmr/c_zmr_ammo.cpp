#include "cbase.h"
#include "ammodef.h"
#include "glowstencil/c_glowbase.h"

#include "c_zmr_player.h"


ConVar zm_cl_glow_ammo( "zm_cl_glow_ammo", "1 .2 .2", FCVAR_ARCHIVE );
ConVar zm_cl_glow_ammo_enabled( "zm_cl_glow_ammo_enabled", "1", FCVAR_ARCHIVE );



class C_ZMAmmo : public C_BaseGlowEntity
{
public:
    DECLARE_CLASS( C_ZMAmmo, C_BaseGlowEntity );
    DECLARE_CLIENTCLASS();

    C_ZMAmmo();


    virtual void    Spawn() OVERRIDE;
    virtual void    ClientThink() OVERRIDE;


    void            UpdateGlow();
    virtual void    GetGlowEffectColor( float& r, float& g, float& b ) OVERRIDE;

protected:
    CNetworkVar( int, m_iAmmoType );
};

IMPLEMENT_CLIENTCLASS_DT( C_ZMAmmo, DT_ZM_Ammo, CZMAmmo )
    RecvPropInt( RECVINFO( m_iAmmoType ) ),
END_RECV_TABLE()


C_ZMAmmo::C_ZMAmmo()
{
    m_iAmmoType = -1;
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
    if ( !zm_cl_glow_ammo_enabled.GetBool() )
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
    &&  pPlayer->GetAmmoCount( m_iAmmoType ) < (GetAmmoDef()->MaxCarry( m_iAmmoType ) * 0.8f) )
    {
        if ( !IsClientSideGlowEnabled() )
            SetClientSideGlowEnabled( true );
    }
    else
    {
        SetClientSideGlowEnabled( false );
    }
}

void C_ZMAmmo::GetGlowEffectColor( float& r, float& g, float& b )
{
    CSplitString split( zm_cl_glow_ammo.GetString(), " " );

    if ( split.Count() > 0 ) r = atof( split[0] );
    if ( split.Count() > 1 ) g = atof( split[1] );
    if ( split.Count() > 2 ) b = atof( split[2] );
}
