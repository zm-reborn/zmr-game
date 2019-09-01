#include "cbase.h"
#include "kbutton.h"
#include "input.h"

#include "c_zmr_in_main.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



ConVar zm_cl_border_pitchspeed( "zm_cl_border_pitchspeed", "100", FCVAR_ARCHIVE, "ZM screen scrolling pitch speed" );
ConVar zm_cl_border_yawspeed( "zm_cl_border_yawspeed", "160", FCVAR_ARCHIVE, "ZM screen scrolling yaw speed" );

#define SCROLL_GRACETIME                0.1f


CZMInput::CZMInput()
{
    m_StateYaw = m_StatePitch = { SCROLL_NONE, 0.0f };
}

CZMInput::~CZMInput()
{
}

void CZMInput::SetScreenScrollStatePitch( ScrollState_t state )
{
    m_StatePitch.iState = state;
    m_StatePitch.flLastStateChange = gpGlobals->curtime;
}

void CZMInput::SetScreenScrollStateYaw( ScrollState_t state )
{
    m_StateYaw.iState = state;
    m_StateYaw.flLastStateChange = gpGlobals->curtime;
}

void CZMInput::AdjustYaw( float speed, QAngle& viewangles )
{
    CInput::AdjustYaw( speed, viewangles );


    //
    // ZM screen scroll
    //
    // Adjust pitch and yaw (because AdjustPitch isn't virtual...)
    // Always check the time of the last change, since we don't want to be spinning around for ever.
    //
    if ( m_StateYaw.iState != SCROLL_NONE )
    {
	    viewangles[YAW] += speed * zm_cl_border_yawspeed.GetFloat() * (m_StateYaw.iState == SCROLL_POSITIVE ? 1 : -1);

        if ( (gpGlobals->curtime - m_StateYaw.flLastStateChange) > SCROLL_GRACETIME )
        {
            m_StateYaw.iState = SCROLL_NONE;
        }
    }

    if ( m_StatePitch.iState != SCROLL_NONE )
    {
	    viewangles[PITCH] += speed * zm_cl_border_pitchspeed.GetFloat() * (m_StatePitch.iState == SCROLL_POSITIVE ? 1 : -1);

        if ( (gpGlobals->curtime - m_StatePitch.flLastStateChange) > SCROLL_GRACETIME )
        {
            m_StatePitch.iState = SCROLL_NONE;
        }
    }
}


static CZMInput g_Input;

// Expose this interface
IInput* input = (IInput*)&g_Input;

CZMInput* ZMInput()
{
    return &g_Input;
}
