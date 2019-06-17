#include "cbase.h"

#include "c_zmr_player.h"
#include "zmr/zmr_shareddefs.h"

#include "c_zmr_colorcorrection.h"


#define ZMCC_SURVIVOR           "materials/colorcorrection/game_survivor.raw"


ConVar zm_cl_colorcorrection_survivor_weight( "zm_cl_colorcorrection_survivor_weight", "1", FCVAR_ARCHIVE );

class CZMSurvivorEffect : public CZMBaseCCEffect
{
public:
    CZMSurvivorEffect() : CZMBaseCCEffect( "zmgame_survivor", ZMCC_SURVIVOR )
    {
        m_bAlive = false;
    }

    virtual bool OnTeamChange( int iTeam ) OVERRIDE
    {
        if ( iTeam == ZMTEAM_HUMAN )
        {
            m_bAlive = true;

            return true;
        }

        return false;
    }

    virtual bool OnDeath() OVERRIDE
    {
        m_bAlive = false;
        return false;
    }

    virtual float GetWeight() const OVERRIDE
    {
        return zm_cl_colorcorrection_survivor_weight.GetFloat();
    }

    virtual bool IsDone() const OVERRIDE
    {
        return !m_bAlive;
    }

private:
    bool m_bAlive;
};

CZMSurvivorEffect g_ZMCCSurvivorEffect;

