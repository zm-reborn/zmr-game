#pragma once

#include "cbase.h"

#include "glow_outline_effect.h"


// Client-only version.
abstract_class C_BaseGlowEntity : public C_BaseAnimating
{
public:
    DECLARE_CLASS( C_BaseGlowEntity, C_BaseAnimating )

    C_BaseGlowEntity();
    ~C_BaseGlowEntity();


    CGlowObject*    GetGlowObject() const { return m_pGlowEffect; };
    virtual bool    ShouldCreateGlow() const { return m_bClientSideGlowEnabled; };
    virtual void    GetGlowEffectColor( float& r, float& g, float& b ) const { r = g = b = 1.0f; };
    void            SetClientSideGlowEnabled( bool bEnabled ) { m_bClientSideGlowEnabled = bEnabled; UpdateGlowEffect(); };
    inline bool     IsClientSideGlowEnabled() const { return m_bClientSideGlowEnabled; };

    virtual bool    GlowOccluded() const { return true; };
    virtual bool    GlowUnoccluded() const { return true; };

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

    virtual bool    ShouldCreateGlow() const OVERRIDE;
    virtual void    GetGlowEffectColor( float& r, float& g, float& b ) const OVERRIDE;

    virtual bool    GlowOccluded() const OVERRIDE;
    virtual bool    GlowUnoccluded() const OVERRIDE;

protected:
    CNetworkVar( bool, m_bGlowEnabled );
    CNetworkColor32( m_GlowColor );
    CNetworkVar( int, m_fGlowType );

    bool            m_bOldGlowEnabled;
    color32         m_GlowOldColor;
    int             m_fOldGlowType;
};
