#pragma once


#include "gamemovement.h"
#include "hl2/hl_gamemovement.h"

#include "func_ladder.h"

#ifndef CLIENT_DLL
#include "hl2/hl2_player.h"
#else
#include "hl2/c_basehlplayer.h"
#endif

#include "zmr_player_shared.h"

/*
    NOTE:
    You have to set m_HL2Local to public in hl2/hl2_player.h ...


    Remove hl2/hl_gamemovement.cpp from project.
*/


class CZMGameMovement : public CHL2GameMovement
{
public:
    typedef CHL2GameMovement BaseClass;

    virtual void PlayerMove() OVERRIDE;


    virtual Vector GetPlayerMins() const OVERRIDE;
    virtual Vector GetPlayerMins( bool ducked ) const OVERRIDE;
    virtual Vector GetPlayerMaxs() const OVERRIDE;
    virtual Vector GetPlayerMaxs( bool ducked ) const OVERRIDE;
    virtual Vector GetPlayerViewOffset( bool ducked ) const OVERRIDE;
    virtual unsigned int PlayerSolidMask( bool brushOnly = false ) OVERRIDE;

    virtual void CategorizePosition() OVERRIDE;

    void FullZMMove( float factor, float maxacceleration );
    virtual void Duck() OVERRIDE;

    virtual bool CheckJumpButton() OVERRIDE;

    // Lets dead players accelerate or otherwise spectators can't move in roaming mode. Thanks Valve.
    virtual bool CanAccelerate() OVERRIDE { return !(player->GetWaterJumpTime()); }
    virtual void Accelerate( Vector& wishdir, float wishspeed, float accel ) OVERRIDE;


    virtual void PlayerRoughLandingEffects( float fvol ) OVERRIDE;

    inline CZMPlayer* GetZMPlayer() const { return static_cast<CZMPlayer*>( player ); }
};
