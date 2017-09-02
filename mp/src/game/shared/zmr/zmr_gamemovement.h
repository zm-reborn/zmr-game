#pragma once


#include "gamemovement.h"
#include "hl2/hl_gamemovement.h"

#include "func_ladder.h"

#ifndef CLIENT_DLL
#include "hl2/hl2_player.h"
#else
#include "hl2/c_basehlplayer.h"
#endif


/*
    NOTE:
    You have to set m_HL2Local to public in hl2/hl2_player.h ...


    Remove hl2/hl_gamemovement.cpp from project.
*/


class CZMGameMovement : public CGameMovement
{
protected:
    virtual void PlayerMove( void ) OVERRIDE;
    virtual bool LadderMove( void ) OVERRIDE;

    virtual void FullLadderMove() OVERRIDE;
    void FullZMMove( float factor, float maxacceleration );

    // Lets dead players accelerate or otherwise spectators can't move in roaming mode. Thanks Valve.
    virtual bool CanAccelerate() OVERRIDE { return !(player->GetWaterJumpTime()); };


    /*
        Ladder stuff copied from hl2/hl_gamemovement

        I really don't give a shit, so I'll just inline everything in here.
    */

    //-----------------------------------------------------------------------------
    // Purpose: Ends your life by making you do stupid shit.
    // Output : CFuncLadder
    //-----------------------------------------------------------------------------
    inline CFuncLadder* CZMGameMovement::GetLadder()
    {
	    return static_cast<CFuncLadder*>( static_cast<CBaseEntity *>( GetHL2Player()->m_HL2Local.m_hLadder.Get() ) );
    }

    inline void SetLadder( CFuncLadder* ladder )
    {
	    CFuncLadder* oldLadder = GetLadder();

	    if ( !ladder && oldLadder )
	    {
		    oldLadder->PlayerGotOff( GetHL2Player() );
	    }


	    GetHL2Player()->m_HL2Local.m_hLadder.Set( ladder );
    }

	inline LadderMove_t* GetLadderMove()
    {
        CZMPlayer *p = GetZMPlayer();
        if ( !p )
        {
	        return NULL;
        }

        return p->GetLadderMove();
    }

    inline CZMPlayer* GetZMPlayer() { return static_cast<CZMPlayer*>( player ); };

    inline CHL2_Player* GetHL2Player() { return static_cast<CHL2_Player*>( player ); };




    bool CheckLadderAutoMount( CFuncLadder*, const Vector& );
	bool CheckLadderAutoMountCone( CFuncLadder*, const Vector&, float, float );
	bool CheckLadderAutoMountEndPoint(CFuncLadder*, const Vector& );


    bool LookingAtLadder( CFuncLadder* );

    void Findladder( float, CFuncLadder**, Vector&, const CFuncLadder* );

	void SwallowUseKey();


	// Are we forcing the user's position to a new spot
	bool IsForceMoveActive()
    {
        LadderMove_t *lm = GetLadderMove();
	    return lm->m_bForceLadderMove;
    }

	void StartForcedMove( bool, float, const Vector&, CFuncLadder* );
	bool ContinueForcedMove();

	bool ExitLadderViaDismountNode( CFuncLadder*, bool, bool useAlternate = false );
	void GetSortedDismountNodeList( const Vector&, float, CFuncLadder*, CUtlRBTree< NearbyDismount_t, int >& );
};
