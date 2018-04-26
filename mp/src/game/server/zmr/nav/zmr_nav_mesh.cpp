#include "cbase.h"


#include "npcr/npcr_basenpc.h"
#include "npcr/npcr_manager.h"

#include "zmr_nav_area.h"
#include "zmr_nav_mesh.h"


CZMRNavMesh::CZMRNavMesh()
{
    ListenForGameEvent( "round_restart_pre" );
    ListenForGameEvent( "round_restart_post" );
    ListenForGameEvent( "nav_generate" );
}

CZMRNavMesh::~CZMRNavMesh()
{
}

CNavArea* CZMRNavMesh::CreateArea() const
{
    return new CZMRNavArea;
}

void CZMRNavMesh::OnServerActivate()
{
    BaseClass::OnServerActivate();

    // This is post nav file load.
    if ( !IsLoaded() )
    {
        Warning( "!!\n!! Map does not have a NAV mesh loaded!\n!!\n" );
    }
}

class NavRoundRestart
{
public:
    bool operator()( CNavArea *area )
    {
        area->OnRoundRestart();
        return true;
    }

    bool operator()( CNavLadder *ladder )
    {
        ladder->OnRoundRestart();
        return true;
    }
};

void CZMRNavMesh::FireGameEvent( IGameEvent* pEvent )
{
    if ( FStrEq( pEvent->GetName(), "round_restart_pre" ) )
    {
        OnRoundRestartPreEntity();

        FOR_EACH_VEC( TheNavAreas, it )
        {
            CNavArea* area = TheNavAreas[it];
            area->OnRoundRestartPreEntity();
        }

        return;
    }

    if ( FStrEq( pEvent->GetName(), "round_restart_post" ) )
    {
        OnRoundRestart();
        
        NavRoundRestart restart;
        ForAllAreas( restart );
        ForAllLadders( restart );

        return;
    }

    if ( FStrEq( pEvent->GetName(), "nav_generate" ) )
    {
        engine->ServerCommand( "npcr_remove_all\n" );
        return;
    }


    BaseClass::FireGameEvent( pEvent );
}
