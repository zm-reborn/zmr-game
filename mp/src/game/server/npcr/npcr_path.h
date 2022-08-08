#pragma once

#include "nav_mesh.h"
#include "nav_pathfind.h"

#include "npcr_path_aigraph.h"
#include "npcr_path_cost.h"

namespace NPCR
{
#define MAX_PATH_LINKS   128


    enum NavBuildMethod_t
    {
        BUILD_NAVMESH = 0,
        BUILD_AIGRAPH,
        BUILD_DIRECT
    };

    enum NavTravel_t
    {
        TRAVEL_ONGROUND = 0,
        // There's a drop, use special goal detection so we properly follow the path.
        // This is for cases where the NPC turns around after dropping down.
        TRAVEL_DROPDOWN,
        TRAVEL_NAVJUMP, // We need to jump
    };
    
    struct NavLink_t
    {
    public:
        CNavArea* area;
        Vector pos;
        NavTravel_t navTravel;
        NavTraverseType navTraverse;
        Vector fwd; // Unit vector to next segment.
        float fwd_dot; // Dot product of previous link fwd and our fwd.
        float length;
    };

    class CBaseNavPath : public CAIGraphPath
    {
    public:
        CBaseNavPath();
        virtual ~CBaseNavPath();


        virtual NavBuildMethod_t GetBuildMethod();


        // We must at least have 2 segments. (start & end)
        virtual bool IsValid() const { return m_nLinkCount >= 2; }

        virtual void Invalidate() { m_nLinkCount = 0; }


        virtual void OnPathCreated() {}
        virtual void OnPathSuccess();
        virtual void OnPathFailed();


        virtual bool Compute( const Vector& vecStart, const Vector& vecGoal, CNavArea* pStartArea, CNavArea* pGoalArea, const CBasePathCost& cost );
        virtual bool Compute( const Vector& vecStart, const Vector& vecGoal, const CBasePathCost& cost, float maxDistanceToArea = 10000.0f );
        virtual bool Compute( CBaseCombatCharacter* pNPC, CBaseCombatCharacter* pTarget, const CBasePathCost& cost );
        virtual bool Compute( CBaseCombatCharacter* pNPC, CBaseEntity* pTarget, const CBasePathCost& cost, float maxDistanceToArea = 10000.0f );

        virtual bool ComputeNavPathDetails( const Vector& vecStart, CNavArea* pFirstArea, CNavArea* pLastArea, const Vector& vecGoal, const CBasePathCost& cost );

        // Builds a simple straight path with 2 links.
        virtual bool BuildSimplePath( const Vector& vecStart, const Vector& vecGoal, bool bNoNavArea = false, float maxDistanceToArea = 10000.0f );
        virtual bool BuildSimplePath( const Vector& vecStart, const Vector& vecGoal, CNavArea* pStartArea, CNavArea* pGoalArea );


        bool CheckSimpleLOS( const Vector& vecStart, const Vector& vecEnd, trace_t& tr ) const;


        float ComputeLength();


        bool ShouldDraw();
        virtual void Draw();

        int                 LinkCount() const { return m_nLinkCount; }
        const NavLink_t*    FirstLink() const;
        const NavLink_t*    PreviousLink( const NavLink_t* seg ) const;
        const NavLink_t*    NextLink( const NavLink_t* seg ) const;
        const NavLink_t*    LastLink() const;
        int                 GetLinkOffset( const NavLink_t* seg ) const;


        bool IsUsingExactGoal() const { return m_bExactGoal; }
        void UseExactGoal( bool state ) { m_bExactGoal = state; }


        virtual bool ShouldDecelerateToGoal() const { return false; }

    protected:
        virtual void DrawSegment( const NavLink_t* from, const NavLink_t* to );


        virtual bool BuildNavPath( const Vector& vecStart, const Vector& vecGoal, CNavArea* pStartArea, CNavArea* pGoalArea, const CBasePathCost& cost );
        virtual bool BuildGraphPath( const Vector& vecStart, const Vector& vecGoal, const CBasePathCost& cost );


        int m_nLinkCount;
        NavLink_t m_Links[MAX_PATH_LINKS];


        bool m_bExactGoal;
    };
}
