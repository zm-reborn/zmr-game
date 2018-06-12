#pragma once

#include "ai_hull.h"

#include "ai_navtype.h"
#include "ai_waypoint.h"
#include "ai_node.h"


#include "npcr_path_cost.h"



namespace NPCR
{
    class CAIGraphPath
    {
    public:
        static bool HasAIGraph();

        // By default search ground only nodes.
        int FindAINodeOfPosition( const Vector& pos, float flMaxDist = 512.0f, NodeType_e nodeType = NODE_GROUND ) const;

    protected:
        virtual bool BuildGraphPath() { return false; }

        AI_Waypoint_t* FindGraphPath( int startID, int endID, const CBasePathCost& cost );
        
        
        virtual Hull_t GetGraphHullType() const { return HULL_HUMAN; }
        
    private:
        AI_Waypoint_t* MakeRouteFromParents( int* parentArray, int endID );
        Navigation_t ComputeWaypointType( CAI_Node* pParentNode, int destID );
    };
}
