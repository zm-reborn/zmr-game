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

    virtual void PlayerMove() override;


    virtual void TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm ) override;
    virtual CBaseHandle TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm ) override;
    virtual void TryTouchGround( const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int fMask, int collisionGroup, trace_t& pm ) override;

    virtual Vector GetPlayerMins() const override;
    virtual Vector GetPlayerMins( bool ducked ) const override;
    virtual Vector GetPlayerMaxs() const override;
    virtual Vector GetPlayerMaxs( bool ducked ) const override;
    virtual Vector GetPlayerViewOffset( bool ducked ) const override;
    virtual unsigned int PlayerSolidMask( bool brushOnly = false ) override;

    virtual void CategorizePosition() override;

    void FullZMMove();
    virtual void Duck() override;
    virtual void FinishDuck() override;

    virtual bool CheckJumpButton() override;

    // Lets dead players accelerate or otherwise spectators can't move in roaming mode. Thanks Valve.
    virtual bool CanAccelerate() override { return !(player->GetWaterJumpTime()); }
    virtual void Accelerate( Vector& wishdir, float wishspeed, float accel ) override;

    virtual float ClimbSpeed() const override;
    virtual bool LadderMove() override;


    virtual void PlayerRoughLandingEffects( float fvol ) override;

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
