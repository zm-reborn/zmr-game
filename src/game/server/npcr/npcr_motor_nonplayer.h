#pragma once


#include "npcr_motor.h"
#include "npcr_nonplayer.h"


namespace NPCR
{
    class CNonPlayerMotor : public CBaseMotor
    {
    public:
        typedef CNonPlayerMotor ThisClass;
        typedef CBaseMotor BaseClass;

        CNonPlayerMotor( CNPCRNonPlayer* pNPC );
        ~CNonPlayerMotor();


        virtual CNPCRNonPlayer* GetOuter() const override;


        virtual void Update() override;


        virtual const Vector& GetVelocity() const override { return m_vecVelocity; }
        virtual void SetVelocity( const Vector& vecVel ) override { m_vecVelocity = vecVel; BaseClass::SetVelocity( vecVel ); }


        virtual void Approach( const Vector& vecDesiredGoal ) override;

        // Applied when moving
        virtual float GetFrictionSideways() const { return 5.0f; }

        // Applied when not doing anything
        virtual float GetFriction() const { return 6.0f; }


        virtual float GetMovementSpeed() const override;

    protected:
        void Move();
        bool GroundMove();

        bool ShouldApplyGroundMove() const;

        bool m_bDoDecelerate;
        Vector m_vecAcceleration;
        Vector m_vecVelocity;
        Vector m_vecLastBaseVelocity;
    };
}
