#pragma once

#include "cbase.h"

#include "glow_outline_effect.h"


abstract_class C_BaseGlowEntity : public C_BaseAnimating // Client-only version.
{
public:
    DECLARE_CLASS( C_BaseGlowEntity, C_BaseAnimating )

    C_BaseGlowEntity();
    ~C_BaseGlowEntity();


    CGlowObject*    GetGlowObject() { return m_pGlowEffect; };
    virtual bool    ShouldCreateGlow() { return m_bClientSideGlowEnabled; };
    virtual void    GetGlowEffectColor( float& r, float& g, float& b ) { r = g = b = 1.0f; };
    void            SetClientSideGlowEnabled( bool bEnabled ) { m_bClientSideGlowEnabled = bEnabled; UpdateGlowEffect(); };
    inline bool     IsClientSideGlowEnabled() { return m_bClientSideGlowEnabled; };

    virtual bool    GlowOccluded() { return true; };
    virtual bool    GlowUnoccluded() { return true; };

protected:
    virtual void    UpdateGlowEffect();
    virtual void    DestroyGlowEffect();


    bool            m_bClientSideGlowEnabled;
    CGlowObject*    m_pGlowEffect;
};

abstract_class C_BaseGlowProp : public C_BaseGlowEntity // Shared version.
{
public:
    DECLARE_CLASS( C_BaseGlowProp, C_BaseGlowEntity )
    DECLARE_CLIENTCLASS()


    virtual void    OnPreDataChanged( DataUpdateType_t updateType ) OVERRIDE;
    virtual void    OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;

    virtual bool    ShouldCreateGlow() OVERRIDE;
    virtual void    GetGlowEffectColor( float& r, float& g, float& b ) OVERRIDE;

protected:
    CNetworkVar( bool, m_bGlowEnabled );
    CNetworkColor32( m_GlowColor );

    bool            m_bOldGlowEnabled;
    color32         m_GlowOldColor;
};
