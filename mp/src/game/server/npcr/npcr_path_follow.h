#pragma once


#include "npcr_path.h"
#include "npcr_basenpc.h"

namespace NPCR
{
    enum PathRes_t
    {
        PATH_FAILED = 0,
        PATH_OK,
        PATH_SUCCESS
    };

    enum JumpStatus_t
    {
        JUMP_BEGIN = 0,
        JUMP_IN_AIR,
        JUMP_DONE
    };

    class CFollowNavPath : public CBaseNavPath
    {
    public:
        DECLARE_CLASS( CFollowNavPath, CBaseNavPath );

        CFollowNavPath()
        {
            m_pGoal = nullptr;
            m_pCurLink = nullptr;
            m_flGoalTolerance = 36.0f;
            m_flMoveTolerance = 30.0f;
            m_flLastSide = 1.0f;
            m_vecAvoidDelta = vec3_origin;
        }


        virtual bool IsValid() const OVERRIDE
        {
            return CBaseNavPath::IsValid() && m_pGoal;
        }

        virtual void Invalidate() OVERRIDE;


        virtual void OnNewGoal( CBaseNPC* pNPC );


        virtual const NavLink_t* GetGoalLink() const { return m_pGoal; }
        virtual const NavLink_t* GetCurLink() const { return m_pCurLink; }

        // The minimum distance to goal that we stop trying to avoid.
        virtual float GetAvoidMinDistance() const { return 24.0f; }


        float   GetGoalTolerance() const { return m_flGoalTolerance; }
        void    SetGoalTolerance( float tolerance ) { m_flGoalTolerance = tolerance; }

        float   GetMoveTolerance() const { return m_flMoveTolerance; }
        void    SetMoveTolerance( float tolerance ) { m_flMoveTolerance = tolerance; }

        virtual void OnPathCreated() OVERRIDE
        {
            m_pGoal = FirstLink();
            m_pCurLink = nullptr;
        }

        virtual void Update( CBaseNPC* pNPC );


        virtual void Draw();


        virtual bool ShouldDecelerateToGoal() const OVERRIDE;

    protected:
        virtual void    UpdateMove( CBaseNPC* pNPC );
        void            GroundMove( CBaseNPC* pNPC );

        void                FollowPath( CBaseNPC* pNPC );
        virtual PathRes_t   CheckGoal( CBaseNPC* pNPC ) const;
        int                 CheckAvoid( CBaseNPC* pNPC, Vector& vecGoalPos );
        int                 CheckStuck( CBaseNPC* pNPC, Vector& vecGoalPos );

        void                StartCheckingStuck( CBaseNPC* pNPC );
        void                StopCheckingStuck();


        virtual bool        ShouldFailNavJump( CBaseNPC* pNPC ) const;

        void ClearAvoidState()
        {
            m_AvoidTimer.Invalidate();
        }


        const NavLink_t* m_pCurLink; // The link that we are currently using. (ie. goal-1)
        const NavLink_t* m_pGoal; // The link we are going towards.
        float m_flGoalTolerance; // The last link tolerance.
        float m_flMoveTolerance; // Everything else.

        JumpStatus_t m_JumpStatus;

    private:
        float m_flLastSide;
        CountdownTimer m_NextAvoidCheck;
        CountdownTimer m_AvoidTimer;
        Vector m_vecAvoidDelta;

        bool m_bStuckPosCheck;
        int m_nStuckTimes;
        Vector m_vecStuckPos;
        CountdownTimer m_StuckTimer;
        Vector m_vecStuckDelta;
        float m_flStuckAngle;
        CountdownTimer m_StuckCheckTimer;
    };
}
