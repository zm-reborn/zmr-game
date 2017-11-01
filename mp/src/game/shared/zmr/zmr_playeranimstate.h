#pragma once


#include "Multiplayer/multiplayer_animstate.h"

#ifdef CLIENT_DLL
class C_ZMPlayer;
#else
class CZMPlayer;
#endif


class CZMPlayerAnimState : public CMultiPlayerAnimState
{
public:
	DECLARE_CLASS( CZMPlayerAnimState, CMultiPlayerAnimState );

	CZMPlayerAnimState();
	CZMPlayerAnimState( CBasePlayer* pPlayer, MultiPlayerMovementData_t& movementData );
	~CZMPlayerAnimState();

#ifdef CLIENT_DLL
	void InitZMAnimState( C_ZMPlayer* pPlayer );
	C_ZMPlayer* GetZMPlayer() { return m_pZMPlayer; };
#else
	void InitZMAnimState( CZMPlayer* pPlayer );
	CZMPlayer* GetZMPlayer() { return m_pZMPlayer; };
#endif


	virtual void ClearAnimationState() OVERRIDE;
	virtual Activity TranslateActivity( Activity actDesired ) OVERRIDE;
	virtual void Update( float eyeYaw, float eyePitch ) OVERRIDE;

	void DoAnimationEvent( PlayerAnimEvent_t playerAnim, int nData = 0 ) OVERRIDE;

	bool HandleMoving( Activity &idealActivity ) OVERRIDE;
	bool HandleJumping( Activity &idealActivity ) OVERRIDE;
	bool HandleDucking( Activity &idealActivity ) OVERRIDE;
	bool HandleSwimming( Activity &idealActivity ) OVERRIDE;

	virtual float GetCurrentMaxGroundSpeed() OVERRIDE;

private:

	bool            SetupPoseParameters( CStudioHdr* pStudioHdr );
	virtual void    EstimateYaw() OVERRIDE;
	virtual void    ComputePoseParam_MoveYaw( CStudioHdr* pStudioHdr ) OVERRIDE;
	virtual void    ComputePoseParam_AimPitch( CStudioHdr* pStudioHdr ) OVERRIDE;
	virtual void    ComputePoseParam_AimYaw( CStudioHdr* pStudioHdr ) OVERRIDE;

#ifdef CLIENT_DLL
    C_ZMPlayer*     m_pZMPlayer;
#else
	CZMPlayer*      m_pZMPlayer;
#endif
	bool            m_bInAirWalk;
	float           m_flHoldDeployedPoseUntilTime;
};
