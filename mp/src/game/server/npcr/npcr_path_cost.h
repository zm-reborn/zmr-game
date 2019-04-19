#pragma once


#include "nav_mesh.h"
#include "nav_pathfind.h"

namespace NPCR
{
#define PATHCOST_INVALID        -1.0f
    abstract_class CBasePathCost
    {
    public:
        CBasePathCost()
        {
            m_flStepHeight = 16.0f;
            m_flMaxPathLength = 0.0f;
        }

        float           GetStepHeight() const { return m_flStepHeight; }
        virtual void    SetStepHeight( float h ) { m_flStepHeight = h; }

        float           GetMaxPathLength() const { return m_flMaxPathLength; }
        void            SetMaxPathLength( float l ) { m_flMaxPathLength = l; }


        virtual float   GetJumpCostMultiplier() const { return 2.0f; }


        // NAV
        virtual float operator()( CNavArea* area, CNavArea* fromArea, const CNavLadder* ladder, const CFuncElevator* elevator, float length ) const = 0;

        // AI Graph
        virtual float operator()( const Vector& vecNodePos, const Vector& vecTestPos, int fCapBitMask ) const = 0;


        // Can we build a straight path.
        virtual bool CanBuildSimpleRoute( const Vector& vecStart, const Vector& vecGoal ) const;
    
    private:
        float m_flStepHeight;
        float m_flMaxPathLength;
    };

    class CPathCostGroundOnly : public CBasePathCost
    {
    public:
        CPathCostGroundOnly()
        {
            m_flMaxHeightChange = GetStepHeight(); // Just use default step height.

            //m_flHullHeight = 72.0f;

            m_pStartArea = nullptr;
            m_pGoalArea = nullptr;
        }

        //int     GetHullHeight() const { return m_flHullHeight; }
        //void    SetHullHeight( int h ) { m_flHullHeight = h; }

        
        virtual void SetStepHeight( float h ) OVERRIDE
        {
            // Make sure height change is at least step height.
            if ( GetMaxHeightChange() < h )
                SetMaxHeightChange( h );

            CBasePathCost::SetStepHeight( h );
        }

        int     GetMaxHeightChange() const { return m_flMaxHeightChange; }
        void    SetMaxHeightChange( float h ) { m_flMaxHeightChange = h; }

        CNavArea* GetStartArea() const { return m_pStartArea; }
        void SetStartPos( const Vector& vecPos, CNavArea* area )
        {
            m_vecStart = vecPos;
            m_pStartArea = area;
        }

        CNavArea* GetGoalArea() const { return m_pGoalArea; }
        void SetGoalPos( const Vector& vecPos, CNavArea* area )
        {
            m_vecEnd = vecPos;
            m_pGoalArea = area;
        }

        // Returns "cost" of moving from one area to another.
        // Return -1 if not possible.
        virtual float operator()( CNavArea* area, CNavArea* fromArea, const CNavLadder* ladder, const CFuncElevator* elevator, float length ) const OVERRIDE;

        virtual float operator()( const Vector& vecNodePos, const Vector& vecTestPos, int fCapBitMask ) const OVERRIDE;

    protected:
        float m_flMaxHeightChange;
        //float m_flHullHeight;


        Vector m_vecStart;
        CNavArea* m_pStartArea;

        Vector m_vecEnd;
        CNavArea* m_pGoalArea;
    };
}
