#include "cbase.h"
#include "debugoverlay_shared.h"

#include "zmr_player_shared.h"
#include "zmr_thirdpersonmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CZMThirdpersonCameraTraceFilter : public CTraceFilter
{
public:
    CZMThirdpersonCameraTraceFilter();

    virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask ) OVERRIDE;
};

CZMThirdpersonCameraTraceFilter::CZMThirdpersonCameraTraceFilter()
{
}

bool CZMThirdpersonCameraTraceFilter::ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask )
{
    auto* pEnt = EntityFromEntityHandle( pHandleEntity );
    if ( !pEnt )
    {
        return true;
    }


    return !pEnt->IsPlayer();
}



#ifdef CLIENT_DLL
ConVar zm_cl_thirdperson_dist( "zm_cl_thirdperson_dist", "40", FCVAR_USERINFO, "Distance from player head.", true, 32.0f, true, 128.0f );
ConVar zm_cl_thirdperson_offset_pitch( "zm_cl_thirdperson_offset_pitch", "5", FCVAR_USERINFO, "How many degrees do we offset from the back the player.", true, -32.0f, true, 32.0f );
ConVar zm_cl_thirdperson_offset_yaw( "zm_cl_thirdperson_offset_yaw", "40", FCVAR_USERINFO, "How many degrees do we offset from the back the player.", true, -32.0f, true, 32.0f );
ConVar zm_cl_thirdperson_camera_hullsize( "zm_cl_thirdperson_camera_hullsize", "20", FCVAR_USERINFO, "Size of the camera trace hull.", true, 1.0f, true, 32.0f );
ConVar zm_cl_thirdperson_aim_hullsize( "zm_cl_thirdperson_aim_hullsize", "4", FCVAR_USERINFO, "Size of the aim trace hull.", true, 1.0f, true, 32.0f );
ConVar zm_cl_thirdperson_aim_max_distance( "zm_cl_thirdperson_aim_max_distance", "4096", FCVAR_USERINFO, "Size of the aim trace hull.", true, 1.0f, false, 0.0f );

ConVar zm_cl_thirdperson_debug( "zm_cl_thirdperson_debug", "0", FCVAR_CHEAT );
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

    // Invert pitch
    float pitch = angView.x;
    // Offset pitch
    pitch += zm_cl_thirdperson_offset_pitch.GetFloat();
    pitch = AngleNormalize( pitch );

    // Go to the back of the player
    float yaw  = angView.y - 180.0f;
    yaw = AngleNormalize( yaw );
    // Offset yaw
    yaw += zm_cl_thirdperson_offset_yaw.GetFloat();
    yaw = AngleNormalize( yaw );

    Vector vecDirCamera;
    vecDirCamera.x = cos( DEG2RAD( yaw ) );
    vecDirCamera.y = sin( DEG2RAD( yaw ) );
    vecDirCamera.z = 0.0f;
    vecDirCamera.NormalizeInPlace();

    vecDirCamera.z = sin( DEG2RAD( pitch ) );

    


    vecTraceEnd = vecTraceStart + vecDirCamera * zm_cl_thirdperson_dist.GetFloat();

    CZMThirdpersonCameraTraceFilter filter;
    trace_t tr;


    Vector mins, maxs;
    float hullHalf = zm_cl_thirdperson_camera_hullsize.GetFloat() * 0.5f;

    maxs.x = maxs.y = maxs.z = hullHalf;
    mins = -maxs;
    

    UTIL_TraceHull( vecTraceStart, vecTraceEnd, mins, maxs, CONTENTS_SOLID, &filter, &tr );

    m_vecLastThirdpersonSpot = tr.endpos;
    m_angLastThirdpersonAngles = angView;

    vecView = m_vecLastThirdpersonSpot;


    if ( IsDebugging() )
    {
        NDebugOverlay::SweptBox( tr.startpos, tr.endpos, mins, maxs, vec3_angle, 255, 255, 255, 255, gpGlobals->frametime );
    }
    
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
    Vector vecTraceEnd = vecTraceStart + vecLookDir * zm_cl_thirdperson_aim_max_distance.GetFloat();

    CZMThirdpersonCameraTraceFilter filter;
    trace_t tr;

    float hullHalf = zm_cl_thirdperson_aim_hullsize.GetFloat() * 0.5f;
    Vector mins, maxs;

    maxs.x = maxs.y = maxs.z = hullHalf;
    mins = -maxs;

    UTIL_TraceHull( vecTraceStart, vecTraceEnd, mins, maxs, CONTENTS_SOLID, &filter, &tr );


    Vector vecNewLook = (tr.endpos - pLocal->EyePosition()).Normalized();

    QAngle angNew;
    VectorAngles( vecNewLook, angNew );

    angNew.x = AngleNormalize( angNew.x );
    angNew.y = AngleNormalize( angNew.y );
    angNew.z = 0.0f;

    pCmd->aimangles = angNew;

    if ( IsDebugging() )
    {
        NDebugOverlay::SweptBox( tr.startpos, tr.endpos, mins, maxs, vec3_angle, 255, 255, 255, 255, gpGlobals->frametime );
    }

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

bool CZMThirdpersonManager::IsDebugging()
{
    return zm_cl_thirdperson_debug.GetBool();
}

CZMThirdpersonManager g_ZMThirdpersonManager;

#ifdef CLIENT_DLL
CON_COMMAND( zm_cl_thirdperson_toggle, "" )
{
    g_ZMThirdpersonManager.ToggleThirdperson();
}
#endif
