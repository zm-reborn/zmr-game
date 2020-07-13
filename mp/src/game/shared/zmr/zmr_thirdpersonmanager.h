#pragma once

#include "igamesystem.h"

class CZMThirdpersonManager : public CAutoGameSystem
{
public:
    CZMThirdpersonManager();
    ~CZMThirdpersonManager();

    bool ComputeViewToThirdperson( Vector& vecView, QAngle& angView );
    bool ComputeThirdpersonToUserCmd( CUserCmd* pCmd );

    bool IsInThirdperson() const;
    void ToggleThirdperson();

private:
    bool m_bWantThirdperson;
    Vector m_vecLastThirdpersonSpot;
    QAngle m_angLastThirdpersonAngles;
};

extern CZMThirdpersonManager g_ZMThirdpersonManager;
