#include "cbase.h"

#include "ai_link.h"
#include "ai_node.h"
#include "ai_network.h"


#include "npcr_path_aigraph.h"


bool NPCR::CAIGraphPath::HasAIGraph()
{
    return g_pBigAINet->NumNodes() > 0;
}

// Copied from ai_pathfinder.cpp
// AI_Waypoint_t *CAI_Pathfinder::FindBestPath(int startID, int endID)
AI_Waypoint_t* NPCR::CAIGraphPath::FindGraphPath( int startID, int endID, const CBasePathCost& cost )
{
    if ( startID == NO_NODE )
        return nullptr;
    if ( endID == NO_NODE )
        return nullptr;

    int nNodes = g_pBigAINet->NumNodes();
    if ( g_pBigAINet->NumNodes() < 1 )
        return nullptr;
    
    CAI_Node** pNodes = g_pBigAINet->AccessNodes();

    CVarBitVec openBS( nNodes );
    CVarBitVec closeBS( nNodes );

    // ------------- INITIALIZE ------------------------
    float*  nodeG = (float*)stackalloc( nNodes * sizeof( float ) );
    float*  nodeH = (float*)stackalloc( nNodes * sizeof( float ) );
    float*  nodeF = (float*)stackalloc( nNodes * sizeof( float ) );
    int*    nodeP = (int*)  stackalloc( nNodes * sizeof( int ) ); // Node parent 

    for ( int node = 0; node < nNodes; node++ )
    {
        nodeG[node] = FLT_MAX;
        nodeP[node] = -1;
    }

    nodeG[startID] = 0.0f;

    nodeH[startID] = 0.1f * (
        pNodes[startID]->GetPosition( GetGraphHullType() )-pNodes[endID]->GetPosition( GetGraphHullType() )
    ).Length(); // Don't want to over estimate
    nodeF[startID] = nodeG[startID] + nodeH[startID];

    openBS.Set( startID );
    closeBS.Set( startID );

    // --------------- FIND BEST PATH ------------------
    while ( !openBS.IsAllClear() )
    {
        int smallestID = CAI_Network::FindBSSmallest( &openBS, nodeF, nNodes );
    
        openBS.Clear( smallestID );

        CAI_Node* pSmallestNode = pNodes[smallestID];
        
        //if ( GetOuter()->IsUnusableNode(smallestID, pSmallestNode->GetHint()) )
        //    continue;

        if ( smallestID == endID )
        {
            AI_Waypoint_t* route = MakeRouteFromParents( &nodeP[0], endID );
            return route;
        }

        // Check this if the node is immediately in the path after the startNode 
        // that it isn't blocked
        for ( int link = 0; link < pSmallestNode->NumLinks(); link++ )
        {
            CAI_Link* nodeLink = pSmallestNode->GetLinkByIndex( link );
            
            //if (!IsLinkUsable(nodeLink,smallestID))
            //    continue;

            // FIXME: the cost function should take into account Node costs (danger, flanking, etc).
            int moveType = nodeLink->m_iAcceptedMoveTypes[GetGraphHullType()];
            int testID = nodeLink->DestNodeID( smallestID );

            Vector r1 = pSmallestNode->GetPosition( GetGraphHullType() );
            Vector r2 = pNodes[testID]->GetPosition( GetGraphHullType() );
            //float dist = GetOuter()->GetNavigator()->MovementCost( moveType, r1, r2 ); // MovementCost takes ref parameters!!


            // Ask our cost function whether this is a valid path.
            float c = cost( r1, r2, moveType );

            if ( c == PATHCOST_INVALID )
                continue;

            
            float new_cost = nodeG[smallestID] + c;

            if ( !closeBS.IsBitSet( testID ) || new_cost < nodeG[testID] )
            {
                nodeP[testID] = smallestID;
                nodeG[testID] = new_cost;
                nodeH[testID] = (
                    pNodes[testID]->GetPosition(GetGraphHullType()) - pNodes[endID]->GetPosition(GetGraphHullType())
                ).Length();
                nodeF[testID] = nodeG[testID] + nodeH[testID];

                closeBS.Set( testID );
                openBS.Set( testID );
            }
        }
    }

    return nullptr;
}

// Copied from ai_pathfinder.cpp
// AI_Waypoint_t* CAI_Pathfinder::MakeRouteFromParents( int *parentArray, int endID )
AI_Waypoint_t* NPCR::CAIGraphPath::MakeRouteFromParents( int* parentArray, int endID )
{
    AI_Waypoint_t* pOldWaypoint = nullptr;
    AI_Waypoint_t* pNewWaypoint = nullptr;
    int currentID = endID;

    CAI_Node** pNodes = g_pBigAINet->AccessNodes();

    while ( currentID != NO_NODE )
    {
        // Try to link it to the previous waypoint
        int prevID = parentArray[currentID];

        int destID; 
        if ( prevID != NO_NODE )
        {
            destID = prevID;
        }
        else
        {
            // If we have no previous node, then use the next node
            if ( !pOldWaypoint )
                return nullptr;
            destID = pOldWaypoint->iNodeID;
        }

        Navigation_t waypointType = ComputeWaypointType( pNodes[currentID], destID );

        // BRJ 10/1/02
        // FIXME: It appears potentially possible for us to compute waypoints 
        // here which the NPC is not capable of traversing (because 
        // pNPC->CapabilitiesGet() in ComputeWaypointType() above filters it out). 
        // It's also possible if none of the lines have an appropriate DestNodeID.
        // Um, shouldn't such a waypoint not be allowed?!?!?
        Assert( waypointType != NAV_NONE );

        pNewWaypoint = new AI_Waypoint_t(
            pNodes[currentID]->GetPosition( GetGraphHullType() ),
            pNodes[currentID]->GetYaw(), waypointType, bits_WP_TO_NODE, currentID
        );

        // Link it up...
        pNewWaypoint->SetNext( pOldWaypoint );
        pOldWaypoint = pNewWaypoint;

        currentID = prevID;
    }

    return pOldWaypoint;
}


// Copied from ai_pathfinder.cpp
// Navigation_t CAI_Pathfinder::ComputeWaypointType( CAI_Node **ppNodes, int parentID, int destID )
Navigation_t NPCR::CAIGraphPath::ComputeWaypointType( CAI_Node* pParentNode, int destID )
{
    int parentID = pParentNode->GetId();
    for ( int link = 0; link < pParentNode->NumLinks(); link++ ) 
    {
        CAI_Link* pLink = pParentNode->GetLinkByIndex( link );
        if ( pLink->DestNodeID( parentID ) == destID )
        {
            byte fCapBits = pLink->m_iAcceptedMoveTypes[GetGraphHullType()];

            // Our cost function should take care of this
            Assert( fCapBits != 0 );



            //Navigation_t linkType = MoveBitsToNavType( moveTypeBits );

            if ( fCapBits & bits_CAP_MOVE_JUMP )
                return NAV_JUMP;

            return NAV_GROUND;
        }
    }

    return NAV_GROUND;
}


int NPCR::CAIGraphPath::FindAINodeOfPosition( const Vector& pos, float flMaxDist, NodeType_e nodeType ) const
{
    int nNodes = g_pBigAINet->NumNodes();
    if ( nNodes < 1 )
        return NO_NODE;


    int iClosestNode = NO_NODE;
    float closestdistsqr = flMaxDist * flMaxDist;

    CAI_Node** pNodes = g_pBigAINet->AccessNodes();
    for ( int i = 0; i < nNodes; i++ )
    {
        if ( pNodes[i]->GetType() != nodeType )
            continue;

        float distsqr = (pos - pNodes[i]->GetOrigin()).LengthSqr();
        if ( distsqr < closestdistsqr )
        {
            closestdistsqr = distsqr;
            iClosestNode = i;
        }
    }

    return iClosestNode;
}