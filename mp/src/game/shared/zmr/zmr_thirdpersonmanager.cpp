#include "cbase.h"

#include "zmr_player_shared.h"
#include "zmr_thirdpersonmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CZMThirdpersonCameraTraceFilter : public CTraceFilterSimple
{
public:
    CZMThirdpersonCameraTraceFilter();
};

CZMThirdpersonCameraTraceFilter::CZMThirdpersonCameraTraceFilter() :
    CTraceFilterSimple( nullptr, COLLISION_GROUP_NONE, nullptr )
{

}

#ifdef CLIENT_DLL
ConVar zm_cl_thirdperson_dist( "zm_cl_thirdperson_dist", "100", FCVAR_USERINFO, "Distance from player head.", true, 32.0f, true, 128.0f );
ConVar zm_cl_thirdperson_offset_z( "zm_cl_thirdperson_offset_z", "8", FCVAR_USERINFO, "Z-Axis offset from the player's head.", true, -32.0f, true, 32.0f );
ConVar zm_cl_thirdperson_offset_yaw( "zm_cl_thirdperson_offset_yaw", "20", FCVAR_USERINFO, "How many degrees do we offset from the back the player.", true, -32.0f, true, 32.0f );
#else

#endif

CZMThirdpersonManager::CZMThirdpersonManager()
{
    m_bWantThirdperson = false;
    m_vecLastThirdpersonSpot = vec3_origin;
    m_angLastThirdpersonAngles = vec3_angle;
}

CZMThirdpersonManager::~CZMThirdpersonManager()
{
}

bool CZMThirdpersonManager::ComputeViewToThirdperson( Vector& vecView, QAngle& angView )
{
    if ( !IsInThirdperson() )
    {
        return false;
    }


    auto* pLocal = CZMPlayer::GetLocalPlayer();
    if ( !pLocal )
    {
        return false;
    }


    Vector vecTraceStart = pLocal->EyePosition();
    Vector vecTraceEnd;


    // Build the unit vector to the camera position.
    float yaw = angView.y;
    yaw -= 180.0f;
    yaw = AngleNormalize( yaw );

    yaw += zm_cl_thirdperson_offset_yaw.GetFloat();
    yaw = AngleNormalize( yaw );

    Vector vecDirCamera;
    vecDirCamera.x = cos( DEG2RAD( yaw ) );
    vecDirCamera.y = sin( DEG2RAD( yaw ) );
    vecDirCamera.z = 0.0f;


    vecTraceEnd = vecTraceStart + vecDirCamera * zm_cl_thirdperson_dist.GetFloat();
    vecTraceEnd.z += zm_cl_thirdperson_offset_z.GetFloat();


    CZMThirdpersonCameraTraceFilter filter;
    trace_t tr;

    UTIL_TraceLine( vecTraceStart, vecTraceEnd, CONTENTS_SOLID, &filter, &tr );

    m_vecLastThirdpersonSpot = tr.endpos;
    m_angLastThirdpersonAngles = angView;

    vecView = m_vecLastThirdpersonSpot;
    
    return true;
}

bool CZMThirdpersonManager::ComputeThirdpersonToUserCmd( CUserCmd* pCmd )
{
    if ( !IsInThirdperson() )
    {
        return false;
    }


    auto* pLocal = CZMPlayer::GetLocalPlayer();
    if ( !pLocal )
    {
        return false;
    }


    //
    // The usercmd angles need to be relative to the "real" player
    // eye location.
    //
    Vector vecLookDir;
    AngleVectors( m_angLastThirdpersonAngles, &vecLookDir );


    Vector vecTraceStart = m_vecLastThirdpersonSpot;
    Vector vecTraceEnd = vecTraceStart + vecLookDir * MAX_TRACE_LENGTH;

    CZMThirdpersonCameraTraceFilter filter;
    trace_t tr;

    UTIL_TraceLine( vecTraceStart, vecTraceEnd, CONTENTS_SOLID, &filter, &tr );


    Vector vecNewLook = (tr.endpos - pLocal->EyePosition()).Normalized();

    QAngle angNew;
    VectorAngles( vecNewLook, angNew );

    angNew.x = AngleNormalize( angNew.x );
    angNew.y = AngleNormalize( angNew.y );
    angNew.z = pCmd->viewangles.z;

    pCmd->viewangles = angNew;

    return true;
}

bool CZMThirdpersonManager::IsInThirdperson() const
{
    if ( !m_bWantThirdperson )
    {
        return false;
    }

    auto* pLocal = CZMPlayer::GetLocalPlayer();
    if ( !pLocal )
    {
        return false;
    }

    if ( !pLocal->IsHuman() || !pLocal->IsAlive() )
    {
        return false;
    }

    return true;
}

void CZMThirdpersonManager::ToggleThirdperson()
{
    m_bWantThirdperson = !m_bWantThirdperson;
}

CZMThirdpersonManager g_ZMThirdpersonManager;

#ifdef CLIENT_DLL
CON_COMMAND( zm_cl_thirdperson_toggle, "" )
{
    g_ZMThirdpersonManager.ToggleThirdperson();
}
#endif
