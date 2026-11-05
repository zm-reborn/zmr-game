#pragma once


#include "npcr_motor.h"
#include "npcr_player.h"


namespace NPCR
{
    class CPlayerMotor : public CBaseMotor
    {
    public:
        typedef CPlayerMotor ThisClass;
        typedef CBaseMotor BaseClass;

        CPlayerMotor( CPlayerCmdHandler* pNPC );
        ~CPlayerMotor();


        virtual void Update() override;


        virtual float GetHullWidth() const override { return 16.0f; }

        virtual bool UsePitch() const override { return true; }
        virtual float GetPitchRate( float delta ) const override;
        virtual float GetYawRate( float delta ) const override;
        virtual float GetStepHeight() const override;


        virtual CBaseEntity* GetGroundEntity() const override { return GetOuter()->GetGroundEntity(); }
        virtual bool IsOnGround() const override { return GetGroundEntity() != nullptr; }


        virtual bool ShouldDoFullMove() const;

    protected:
        void Move();

    private:
        float m_flMoveDist;
    };
}
