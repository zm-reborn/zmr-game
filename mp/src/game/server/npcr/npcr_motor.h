#pragma once

#include "npcr_basenpc.h"
#include "npcr_component.h"


namespace NPCR
{
    class CBaseMotor : public CEventListener
    {
    public:
        CBaseMotor( CBaseNPC* pNPC );
        ~CBaseMotor();
        

        virtual const char* GetComponentName() const OVERRIDE { return "BaseMotor"; }

        virtual void Update() OVERRIDE;

        virtual void Approach( const Vector& vecDesiredGoal );

        virtual void FaceTowards( const Vector& vecPos );
        virtual void FaceTowards( const QAngle& angGoal );
        virtual void FaceTowards( float yaw );
        virtual bool IsFacing( const Vector& vecPos, float grace = 1.0f ) const;
        virtual bool IsFacing( const QAngle& angGoal, float grace = 1.0f ) const;

        virtual bool IsMoving() const { return m_MoveTimer.HasStarted() && !m_MoveTimer.IsElapsed(); }
        bool IsAttemptingToMove() const { return m_bDoMove; }


        const Vector& GetMoveDir() const { return m_vecMoveDir; }
        virtual const Vector& GetVelocity() const { return GetNPC()->GetVel(); }
        virtual void SetVelocity( const Vector& vecVel ) { GetNPC()->SetVel( vecVel ); }
        
        virtual float GetHullHeight() const { return 72.0f; }
        virtual float GetHullWidth() const { return 26.0f; }
        virtual float GetMovementSpeed() const { return 100.0f; }
        virtual float GetMaxAcceleration() const { return 470.0f; }
        virtual float GetMaxDeceleration() const { return 700.0f; }
        virtual float GetSlopeLimit() const { return 0.45f; }
        virtual float GetGravity() const { return 800.0f; }
        virtual float GetStepHeight() const { return 16.0f; }
        virtual float GetPitchRate( float delta ) const { return 0.0f; }
        virtual bool UsePitch() const { return false; }
        virtual float GetYawRate( float delta ) const { return 300.0f; }


        virtual CBaseEntity*    GetGroundEntity() const { return m_hGroundEnt.Get(); }
        virtual bool            IsOnGround() const { return m_bOnGround; }

        virtual void OnLandedGround( CBaseEntity* pGround ) OVERRIDE;
        virtual void OnLeftGround( CBaseEntity* pOldGround ) OVERRIDE;

        virtual void Jump();
        virtual void NavJump( const Vector& vecGoal, float flOverrideHeight = 0.0f );
        virtual void Climb();

        static Vector CalcJumpLaunchVelocity( const Vector& startPos, const Vector& endPos, float flGravity, float* pminHeight, float maxHorzVelocity, Vector* pvecApex );
        
    protected:
        virtual bool ShouldAdjustVelocity() const { return m_bAdjustVel; }

        void UpdateMoving() { m_MoveTimer.Start( 0.2f ); }
        const Vector& GetGroundNormal() const { return m_vecGroundNormal; }


        CBaseEntity*    UpdateGround();
        Vector          HandleCollisions( const Vector& vecGoal );
        void            MoveTowards( const Vector& vecNewPos );


        Vector m_vecMoveDir;
        Vector m_vecDesiredMoveDir;
        bool m_bForceGravity;
        float m_flGroundZOffset;
        
    private:
        Vector m_vecGroundNormal;
        CountdownTimer m_MoveTimer;
        bool m_bDoStepDownTrace;
        bool m_bAdjustVel;
        bool m_bDoMove;
        Vector m_vecLastValidPos;

        bool m_bOnGround;
        CHandle<CBaseEntity> m_hGroundEnt;
    };
}
