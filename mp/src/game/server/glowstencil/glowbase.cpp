#include "cbase.h"
#include "glowbase.h"

#include "zmr_shareddefs.h"


#define GLOWTYPE_ALWAYS             0
#define GLOWTYPE_OCCLUDED           1
#define GLOWTYPE_UNOCCLUDED         2


IMPLEMENT_SERVERCLASS_ST( CBaseGlowProp, DT_BaseGlowProp )
    SendPropBool( SENDINFO( m_bGlowEnabled ) ),
    SendPropInt( SENDINFO_STRUCTELEM( m_GlowColor ), 32, SPROP_UNSIGNED ),
    SendPropInt( SENDINFO( m_fGlowType ), 2, SPROP_UNSIGNED ),
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
    m_fGlowType = GLOWFLAG_OCCLUDED | GLOWFLAG_UNOCCLUDED;
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
    else if ( Q_strcmp( szKeyValue, "glowtype" ) == 0 )
    {
        switch ( atoi( szKeyName ) )
        {
        case GLOWTYPE_OCCLUDED : m_fGlowType = GLOWFLAG_OCCLUDED; break;
        case GLOWTYPE_UNOCCLUDED : m_fGlowType = GLOWFLAG_UNOCCLUDED; break;
        default : m_fGlowType = GLOWFLAG_ALWAYS; break;
        }

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
