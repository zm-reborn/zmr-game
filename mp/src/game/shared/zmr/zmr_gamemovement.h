#pragma once


#include "gamemovement.h"

#include "func_ladder.h"

#include "zmr_player_shared.h"


enum LadderMoveRet_t
{
    LADDERMOVERET_NO_LADDER = 0,

    LADDERMOVERET_ONLADDER,

    LADDERMOVERET_DISMOUNTED
};

class CZMGameMovement : public CGameMovement
{
public:
    typedef CGameMovement BaseClass;

    virtual void PlayerMove() OVERRIDE;


    virtual void TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm ) OVERRIDE;
    virtual CBaseHandle TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm ) OVERRIDE;
    virtual void TryTouchGround( const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int fMask, int collisionGroup, trace_t& pm ) OVERRIDE;

    virtual Vector GetPlayerMins() const OVERRIDE;
    virtual Vector GetPlayerMins( bool ducked ) const OVERRIDE;
    virtual Vector GetPlayerMaxs() const OVERRIDE;
    virtual Vector GetPlayerMaxs( bool ducked ) const OVERRIDE;
    virtual Vector GetPlayerViewOffset( bool ducked ) const OVERRIDE;
    virtual unsigned int PlayerSolidMask( bool brushOnly = false ) OVERRIDE;

    virtual void CategorizePosition() OVERRIDE;

    void FullZMMove();
    virtual void Duck() OVERRIDE;
    virtual void FinishDuck() OVERRIDE;

    virtual bool CheckJumpButton() OVERRIDE;

    // Lets dead players accelerate or otherwise spectators can't move in roaming mode. Thanks Valve.
    virtual bool CanAccelerate() OVERRIDE { return !(player->GetWaterJumpTime()); }
    virtual void Accelerate( Vector& wishdir, float wishspeed, float accel ) OVERRIDE;

    virtual float ClimbSpeed() const OVERRIDE;
    virtual bool LadderMove() OVERRIDE;


    virtual void PlayerRoughLandingEffects( float fvol ) OVERRIDE;

    inline CZMPlayer* GetZMPlayer() const { return static_cast<CZMPlayer*>( player ); }

protected:
    //
    // HL2 ladder specific stuff
    //
    CFuncLadder*    FindLadder( Vector& ladderOrigin, const CFuncLadder* skipLadder );
    bool            CheckLadderMount( CFuncLadder* pLadder, Vector& vecBestPos );

    bool            IsForceMoveActive();
    // Start forcing player position
    void            StartForcedMove( bool mounting, float transit_speed, const Vector& goalpos, CFuncLadder *ladder );
    // Returns false when finished
    bool            ContinueForcedMove();
    bool            ExitLadderViaDismountNode( CFuncLadder* pLadder, bool strict );


    LadderMove_t*   GetLadderMove() const;
    CFuncLadder*    GetLadder() const;

    void            SetLadder( CFuncLadder* pLadder );
    void            ExitLadder();


    LadderMoveRet_t LadderMove_Brush();
    LadderMoveRet_t LadderMove_HL2();
};
