#pragma once

#include "cbase.h"

abstract_class CBaseGlowProp : public CBaseAnimating
{
public:
    DECLARE_CLASS( CBaseGlowProp, CBaseAnimating )
    DECLARE_SERVERCLASS()
    DECLARE_DATADESC()

    CBaseGlowProp();


    void            AddGlowEffect();
    void            RemoveGlowEffect();
    inline bool     IsGlowEffectActive() { return m_bGlowEnabled; };

    virtual bool    KeyValue( const char* szKeyValue, const char* szKeyName ) OVERRIDE;


    void            InputEnableGlow( inputdata_t& inputData );
    void            InputDisableGlow( inputdata_t& inputData );
    void            InputToggleGlow( inputdata_t& inputData );
    void            InputSetGlowColor( inputdata_t& inputData );

protected:
    CNetworkVar( bool, m_bGlowEnabled );
    CNetworkColor32( m_GlowColor );
    CNetworkVar( int, m_fGlowType );
};
