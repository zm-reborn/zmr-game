#include "cbase.h"
#include "glowbase.h"


IMPLEMENT_SERVERCLASS_ST( CBaseGlowProp, DT_BaseGlowProp )
    SendPropBool( SENDINFO( m_bGlowEnabled ) ),
    SendPropInt( SENDINFO_STRUCTELEM( m_GlowColor ), 32, SPROP_UNSIGNED ),
END_SEND_TABLE()

BEGIN_DATADESC( CBaseGlowProp )
    DEFINE_KEYFIELD( m_GlowColor, FIELD_COLOR32, "glowcolor" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "EnableGlow", InputEnableGlow ),
    DEFINE_INPUTFUNC( FIELD_VOID, "DisableGlow", InputDisableGlow ),
    DEFINE_INPUTFUNC( FIELD_VOID, "ToggleGlow", InputToggleGlow ),
    DEFINE_INPUTFUNC( FIELD_COLOR32, "SetGlowColor", InputSetGlowColor ),
END_DATADESC()

CBaseGlowProp::CBaseGlowProp()
{
    m_GlowColor = { 255, 255, 255, 255 };
    m_bGlowEnabled = false;
}

void CBaseGlowProp::AddGlowEffect()
{
    SetTransmitState( FL_EDICT_ALWAYS );
    m_bGlowEnabled = true;
}

void CBaseGlowProp::RemoveGlowEffect()
{
    //SetTransmitState( FL_EDICT_PVSCHECK );
    m_bGlowEnabled = false;
}

bool CBaseGlowProp::KeyValue( const char* szKeyValue, const char* szKeyName )
{
    if ( Q_strcmp( szKeyValue, "glowenable" ) == 0 )
    {
        atoi( szKeyName ) ? AddGlowEffect() : RemoveGlowEffect();

        return true;
    }

    return BaseClass::KeyValue( szKeyValue, szKeyName );
}

void CBaseGlowProp::InputEnableGlow( inputdata_t& inputData )
{
    AddGlowEffect();
}

void CBaseGlowProp::InputDisableGlow( inputdata_t& inputData )
{
    RemoveGlowEffect();
}

void CBaseGlowProp::InputToggleGlow( inputdata_t& inputData )
{
    IsGlowEffectActive() ? RemoveGlowEffect() : AddGlowEffect();
}

void CBaseGlowProp::InputSetGlowColor( inputdata_t& inputData )
{
    m_GlowColor = inputData.value.Color32();
}
