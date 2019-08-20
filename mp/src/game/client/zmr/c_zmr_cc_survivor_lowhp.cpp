#include "cbase.h"

#include "c_zmr_player.h"
#include "zmr/zmr_shareddefs.h"

#include "c_zmr_colorcorrection.h"


#define ZMCC_SURVIVOR_LOWHP         "materials/colorcorrection/game_survivor_lowhp.raw"


ConVar zm_cl_colorcorrection_survivor_lowhp_start( "zm_cl_colorcorrection_survivor_lowhp_start", "20", FCVAR_ARCHIVE );


class CZMSurvivorLowHpEffect : public CZMBaseCCEffect
{
public:
    CZMSurvivorLowHpEffect() : CZMBaseCCEffect( "zmgame_survivor_lowhp", ZMCC_SURVIVOR_LOWHP )
    {
        m_bAlive = false;
    }


    static float GetHealthCap()
    {
        return zm_cl_colorcorrection_survivor_lowhp_start.GetFloat();
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
        auto* pPlayer = C_ZMPlayer::GetLocalPlayer();

        if ( !pPlayer )
            return 0.0f;


        int health = pPlayer->GetHealth();

        if ( health >= GetHealthCap() )
            return 0.0f;


        health = MAX( 0, health );

        float weight = (1.0f - health / GetHealthCap());

        return weight;
    }

    virtual bool IsDone() const OVERRIDE
    {
        return !m_bAlive;
    }

private:
    bool m_bAlive;
};

CZMSurvivorLowHpEffect g_ZMCCSurvivorLowHpEffect;

