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

	virtual void GetOuterAbsVelocity( Vector& vel ) override;

    virtual bool ShouldUpdateAnimState() override;
	virtual void ClearAnimationState() override;
	virtual Activity TranslateActivity( Activity actDesired ) override;
	virtual void Update( float eyeYaw, float eyePitch ) override;

	void DoAnimationEvent( PlayerAnimEvent_t playerAnim, int nData = 0 ) override;

	bool HandleMoving( Activity &idealActivity ) override;
	bool HandleJumping( Activity &idealActivity ) override;
	bool HandleDucking( Activity &idealActivity ) override;
	bool HandleSwimming( Activity &idealActivity ) override;

	virtual float GetCurrentMaxGroundSpeed() override;

	bool Uses9WayAnim() const { return m_bIs9Way; }

private:

	bool            SetupPoseParameters( CStudioHdr* pStudioHdr );
	virtual void    EstimateYaw() override;
	virtual void    ComputePoseParam_MoveYaw( CStudioHdr* pStudioHdr ) override;
	virtual void    ComputePoseParam_AimPitch( CStudioHdr* pStudioHdr ) override;
	virtual void    ComputePoseParam_AimYaw( CStudioHdr* pStudioHdr ) override;

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

	bool m_bIs9Way;
};
