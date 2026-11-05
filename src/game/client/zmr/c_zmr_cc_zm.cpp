#include "cbase.h"

#include "c_zmr_player.h"
#include "zmr_shareddefs.h"

#include "c_zmr_colorcorrection.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define ZMCC_ZM         "materials/colorcorrection/game_zm.raw"


ConVar zm_cl_colorcorrection_zm_weight( "zm_cl_colorcorrection_zm_weight", "0.2", FCVAR_ARCHIVE );


class CZMZMEffect : public CZMBaseCCEffect
{
public:
    CZMZMEffect() : CZMBaseCCEffect( "zmgame_zm", ZMCC_ZM )
    {
        m_bAlive = false;
    }

    virtual bool OnTeamChange( int iTeam ) override
    {
        if ( iTeam == ZMTEAM_ZM )
        {
            m_bAlive = true;

            return true;
        }


        m_bAlive = false;

        return false;
    }

    virtual bool OnDeath() override
    {
        m_bAlive = false;
        return false;
    }

    virtual float GetWeight() const override
    {
        return zm_cl_colorcorrection_zm_weight.GetFloat();
    }

    virtual bool IsDone() const override
    {
        return !m_bAlive;
    }

private:
    bool m_bAlive;
};

CZMZMEffect g_ZMCCZMEffect;
