#pragma once


#include "Multiplayer/multiplayer_animstate.h"


#ifdef CLIENT_DLL
#define CZMPlayer C_ZMPlayer
#endif

class CZMPlayer;


class CZMPlayerAnimState : public CMultiPlayerAnimState
{
public:
	DECLARE_CLASS( CZMPlayerAnimState, CMultiPlayerAnimState );

	CZMPlayerAnimState( CBasePlayer* pPlayer, MultiPlayerMovementData_t& movementData );
	~CZMPlayerAnimState();


	void InitZMAnimState( CZMPlayer* pPlayer );
	CZMPlayer* GetZMPlayer() const { return m_pZMPlayer; }

    virtual bool ShouldUpdateAnimState() OVERRIDE;
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
    void            ComputePoseParam_Head( CStudioHdr* pStudioHdr );

    void            UpdateLookAt();
#endif

	CZMPlayer*      m_pZMPlayer;
	bool            m_bInAirWalk;
	float           m_flHoldDeployedPoseUntilTime;


    Vector m_vLookAtTarget;
    float m_flLastLookAtUpdate;

    int	m_headYawPoseParam;
    int	m_headPitchPoseParam;
    float m_headYawMin;
    float m_headYawMax;
    float m_headPitchMin;
    float m_headPitchMax;
    float m_flLastBodyYaw;
    float m_flCurrentHeadYaw;
    float m_flCurrentHeadPitch;
    float m_flCurrentAimYaw;
    CountdownTimer m_blinkTimer;
};
