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


        virtual void Update() OVERRIDE;


        virtual float GetHullWidth() const OVERRIDE { return 16.0f; }

        virtual bool UsePitch() const OVERRIDE { return true; }
        virtual float GetPitchRate( float delta ) const OVERRIDE;
        virtual float GetYawRate( float delta ) const OVERRIDE;
        virtual float GetStepHeight() const OVERRIDE;


        virtual CBaseEntity* GetGroundEntity() const OVERRIDE { return GetOuter()->GetGroundEntity(); }
        virtual bool IsOnGround() const OVERRIDE { return GetGroundEntity() != nullptr; }


        virtual bool ShouldDoFullMove() const;

    protected:
        void Move();

    private:
        float m_flMoveDist;
    };
}
