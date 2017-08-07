#include "cbase.h"

#include "iclientmode.h"
#include "input.h"
#include "view.h"
#include "in_buttons.h"
#include "hud_basechat.h"
#include "clientmode_shared.h"
#include "clientsideeffects.h"
#include "fx_quad.h"


#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_global_shared.h"
#include "zmr/c_zmr_entities.h"
#include "zmr/npcs/c_zmr_zombiebase.h"


#include "zmr_buildmenu.h"
#include "zmr_manimenu.h"

#include "zmr_viewport.h"


DECLARE_HUDELEMENT( CZMFrame );


extern CZMBuildMenu* g_pBuildMenu;


extern IViewPort* gViewPortInterface;

CZMFrame* g_pZMView = nullptr;

CON_COMMAND( zm_observermode, "" )
{
    if ( g_pZMView )
    {
        g_pZMView->SetVisible( !g_pZMView->IsVisible() );
    }
}


class C_TraceFilterNoNPCs : public CTraceFilterSimple
{
public:
	C_TraceFilterNoNPCs( const IHandleEntity *passentity, int collisionGroup )
	: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
    {
	    if ( CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
	    {
		    CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		    if ( !pEntity )
			    return false;

		    return !pEntity->IsNPC();
	    }

	    return false;
    }
};

using namespace vgui;




CZMFrame::CZMFrame( const char* pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMFrame" )
{
    g_pZMView = this;

    SetKeyBoardInputEnabled( false );
    SetMouseInputEnabled( true );
    

    SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );

    SetVisible( false );
    SetProportional( false );
    SetSizeable( false );
    SetMoveable( false );
    SetCloseButtonVisible( false );
    SetTitleBarVisible( false );
    SetMaximizeButtonVisible( false );
    SetMinimizeButtonVisible( false );
    SetBorder( nullptr );
    SetAlpha( 255 );
    SetPaintBackgroundEnabled( false );
    SetZPos( -1337 );
    
    SetHiddenBits( HIDEHUD_WEAPONSELECTION );

    
    m_MouseDragStatus = BUTTON_CODE_INVALID;
    SetClickMode( ZMCLICKMODE_NORMAL );

    m_BoxSelect = GET_HUDELEMENT( CZMBoxSelect );


	m_pZMControl = new CZMControlPanel( this ); 
	m_pZMControl->PositionButtons();
	m_pZMControl->PositionComboBox();


	m_pManiMenu = new CZMManiMenu( this ); 
	m_pBuildMenu = new CZMBuildMenu( this ); 
}

CZMFrame::~CZMFrame()
{
    delete m_pZMControl;
}

void CZMFrame::Init()
{
    
}

void CZMFrame::SetVisible( bool state )
{
    BaseClass::SetVisible( state );

    engine->ClientCmd( "-left" );
    engine->ClientCmd( "-right" );
    engine->ClientCmd( "-lookup" );
    engine->ClientCmd( "-lookdown" );
}

void CZMFrame::SetClickMode( ZMClickMode_t mode, bool print )
{
    if ( mode == m_iClickMode ) return;

    switch ( mode )
    {
    case ZMCLICKMODE_HIDDEN :
        break;
    default : break;
    }
    m_iClickMode = mode;
}

void CZMFrame::OnCursorMoved( int x, int y )
{
    if ( m_MouseDragStatus == MOUSE_LEFT )
    {
        m_BoxSelect->SetEnd( x, y );
    }
}

void CZMFrame::OnMouseReleased( MouseCode code )
{
    switch ( code )
    {
    case MOUSE_RIGHT :
        OnRightRelease();
        break;
    case MOUSE_LEFT :
        OnLeftRelease();
        break;
    default : break;
    }
    
    m_MouseDragStatus = BUTTON_CODE_INVALID;
}

void CZMFrame::OnCommand( const char* command )
{
    if ( Q_stricmp( command, "TAB_POWERS" ) == 0 )
    {
        if ( m_pZMControl ) m_pZMControl->UpdateTabs( CZMControlPanel::TAB_POWERS );
    }
    else if ( Q_stricmp( command, "TAB_MODES" ) == 0 )
    {
    if ( m_pZMControl ) m_pZMControl->UpdateTabs( CZMControlPanel::TAB_MODES );
    }
    else if ( Q_stricmp( command, "TAB_ZEDS" ) == 0 )
    {
        if ( m_pZMControl ) m_pZMControl->UpdateTabs( CZMControlPanel::TAB_ZEDS );
    }
    else if ( Q_stricmp( command, "MODE_SELECT_ALL" ) == 0 )
    {
        engine->ClientCmd( VarArgs( "zm_cmd_selectall" ) );
    }
    else if ( Q_stricmp( command, "MODE_POWER_DELETEZOMBIES" ) == 0 )
    {
        engine->ClientCmd( VarArgs( "zm_cmd_delete" ) );
    }
    else if ( Q_stricmp( command, "MODE_POWER_SPOTCREATE" ) == 0 )
    {
        SetClickMode( ZMCLICKMODE_HIDDEN );
    }
    

    BaseClass::OnCommand( command );
}

void CZMFrame::OnMousePressed( MouseCode code )
{
    CloseChildMenus();


    switch ( code )
    {
    case MOUSE_RIGHT :
        OnRightClick();
        break;
    case MOUSE_LEFT :
        OnLeftClick();
        break;
    default : break;
    }


    m_MouseDragStatus = code;
}

// Too OP.
/*void CZMFrame::OnMouseDoublePressed( MouseCode code )
{
    if ( code != MOUSE_LEFT ) return;


    int mx, my;
    ::input->GetFullscreenMousePos( &mx, &my );

    trace_t trace;
    CTraceFilterNoNPCsOrPlayer filter( nullptr, COLLISION_GROUP_NONE );
    TraceScreenToWorld( mx, my, &trace, &filter );

    Vector end = trace.endpos;

    engine->ClientCmd( VarArgs( "zm_cmd_createhidden %.1f %.1f %.1f", end[0], end[1], end[2] ) );
}*/

void CZMFrame::OnThink()
{
    CZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );

    if ( pPlayer && !pPlayer->IsZM() && IsVisible() )
    {
        SetVisible( false );
    }

    if ( !IsVisible() )
    {
        if ( IsMouseInputEnabled() )
            SetMouseInputEnabled( false );

        return;
    }

    if ( !IsMouseInputEnabled() )
        SetMouseInputEnabled( true );

#define SCRL_BORDER     10

	int mx, my;
	::input->GetFullscreenMousePos( &mx, &my );

    if ( mx < SCRL_BORDER )
    {
        engine->ClientCmd( "+left" );
        engine->ClientCmd( "-right" );
    }
    else if ( mx > (ScreenWidth() - SCRL_BORDER) )
    {
        engine->ClientCmd( "-left" );
        engine->ClientCmd( "+right" );
    }
    else
    {
        engine->ClientCmd( "-left" );
        engine->ClientCmd( "-right" );
    }

    if ( my < SCRL_BORDER )
    {
        engine->ClientCmd( "+lookup" );
        engine->ClientCmd( "-lookdown" );
    }
    else if ( my > (ScreenHeight() - SCRL_BORDER) )
    {
        engine->ClientCmd( "-lookup" );
        engine->ClientCmd( "+lookdown" );
    }
    else
    {
        engine->ClientCmd( "-lookup" );
        engine->ClientCmd( "-lookdown" );
    }
}

void CZMFrame::TraceScreenToWorld( int mx, int my, trace_t* res, CTraceFilterSimple* filter )
{
    CBasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

    if ( !pPlayer ) return;


    float fov = pPlayer->GetFOV();

    float dx, dy;
    float c_x, c_y;

    float dist;

    float aspect = ScreenWidth() / (float)ScreenHeight();

    aspect = ( 4.0f / 3.0f ) / aspect;


    c_x = ScreenWidth() / 2.0f;
    c_y = ScreenHeight() / 2.0f;

    dx = ((float)mx - c_x) / aspect;
    dy = (c_y - (float)my) / aspect;

    dist = c_x / tanf( M_PI_F * fov / 360.0f );

    Vector ray = (MainViewForward() * dist) + (MainViewRight() * dx) + (MainViewUp() * dy);


    VectorNormalize( ray );

    // ZMRTODO: See if this works properly.
    // This seems to work the best. It hits non-solid func_brush, npc clip, etc.
#define MASK_ZMVIEW     ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_MONSTERCLIP | CONTENTS_OPAQUE | CONTENTS_GRATE | CONTENTS_WINDOW )
    
    if ( filter )
    {
        UTIL_TraceLine( MainViewOrigin(), MainViewOrigin() + ray * MAX_TRACE_LENGTH, MASK_ZMVIEW, filter, res );
    }
    else
    {
        UTIL_TraceLine( MainViewOrigin(), MainViewOrigin() + ray * MAX_TRACE_LENGTH, MASK_ZMVIEW, pPlayer, COLLISION_GROUP_NONE, res );
    }
}

void CZMFrame::OnLeftClick()
{
    int mx, my;
    ::input->GetFullscreenMousePos( &mx, &my );


    m_BoxSelect = GET_HUDELEMENT( CZMBoxSelect );

    if ( m_BoxSelect )
    {
        m_BoxSelect->SetEnabled( true );


        ::input->GetFullscreenMousePos( &mx, &my );
        m_BoxSelect->SetStart( mx, my );
    }


    if ( GetClickMode() == ZMCLICKMODE_NORMAL ) return;

    trace_t trace;
    TraceScreenToWorld( mx, my, &trace, nullptr );

    if ( trace.fraction != 1.0f )
    {
        Vector pos = trace.endpos;

        switch ( GetClickMode() )
        {
        case ZMCLICKMODE_TRAP :
            if ( m_pManiMenu )
                engine->ClientCmd( VarArgs( "zm_cmd_createtrigger %i %.1f %.1f %.1f",
                    m_pManiMenu->GetTrapIndex(),
                    pos[0],
                    pos[1],
                    pos[2] ) );
            break;


        case ZMCLICKMODE_RALLYPOINT :
            if ( m_pBuildMenu )
            {
                engine->ClientCmd( VarArgs( "zm_cmd_setrally %i %.1f %.1f %.1f",
                    m_pBuildMenu->GetLastSpawnIndex(),
                    pos[0],
                    pos[1],
                    pos[2] ) );


                m_pBuildMenu->ShowPanel( true );
            }
            break;


        case ZMCLICKMODE_HIDDEN :
            engine->ClientCmd( VarArgs( "zm_cmd_createhidden %.1f %.1f %.1f",
                pos[0],
                pos[1],
                pos[2] ) );
            break;

        default : break;
        }
    }


    SetClickMode( ZMCLICKMODE_NORMAL );
}

void CZMFrame::OnLeftRelease()
{
    m_BoxSelect = GET_HUDELEMENT( CZMBoxSelect );

    if ( m_BoxSelect )
    {
        m_BoxSelect->SetEnabled( false );


        int mx, my;
        ::input->GetFullscreenMousePos( &mx, &my );

        m_BoxSelect->SetEnd( mx, my );
    

        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

        // If our box is too small then don't use it.
        if ( m_BoxSelect->ShouldSelect() )
        {
            int i, j, k, l;
            m_BoxSelect->GetBox( &i, &j, &k, &l );

        
            FindZombiesInBox( i, j, k, l, (pPlayer && pPlayer->m_nButtons & IN_DUCK) ? true : false );
        }
        else
        {
            FindZMObject( mx, my, (pPlayer && pPlayer->m_nButtons & IN_DUCK) ? true : false );
        }
    }

}

void CZMFrame::OnRightClick()
{
    // ZMRTODO: Fix this constant GET_HUDELEMENT shit.
    m_BoxSelect = GET_HUDELEMENT( CZMBoxSelect );

    if ( m_BoxSelect )
    {
        m_BoxSelect->SetEnabled( false );
    }
    


    int mx, my;
    ::input->GetFullscreenMousePos( &mx, &my );
    
    trace_t trace;


    C_TraceFilterNoNPCs filter( nullptr, COLLISION_GROUP_NONE );
    TraceScreenToWorld( mx, my, &trace, &filter );


    Vector end = trace.endpos;

    CBaseEntity* pTarget = trace.m_pEnt;

    // We hit an entity, let's see if we can target it.
    if ( trace.DidHitNonWorldEntity() && pTarget )
    {
        bool bTarget = false;

        if ( pTarget->IsPlayer() )
        {
            bTarget = true;
        }
        else
        {
            IPhysicsObject* phys = pTarget->VPhysicsGetObject();

            bTarget = (phys && phys->IsMoveable()) || !pTarget->IsBaseTrain();
        }

        if ( bTarget )
        {
            engine->ClientCmd( VarArgs( "zm_cmd_target %i %.1f %.1f %.1f", pTarget->entindex(), end[0], end[1], end[2] ) );
            return;
        }
    }


    // Otherwise, just move there.
    engine->ClientCmd( VarArgs( "zm_cmd_move %.1f %.1f %.1f", end[0], end[1], end[2] ) );

    FX_AddQuad( end,
                Vector( 0.0f, 0.0f, 1.0f ),
                10.0f,
                25.0f,
                0, 
                0.2f,
                0.8f,
                0.4f,
                random->RandomInt( 0, 360 ), 
                0,
                Vector( 1.0f, 1.0f, 1.0f ), 
                0.4f, 
                "effects/zm_ring",
                (FXQUAD_BIAS_ALPHA) );
}

void CZMFrame::OnRightRelease()
{
    
}

void CZMFrame::FindZombiesInBox( int start_x, int start_y, int end_x, int end_y, bool bSticky )
{
    C_ZMBaseZombie* pZombie;
    Vector screen;
    int i;
    int x, y;

    CUtlVector<int> vZombieIndices;
    vZombieIndices.Purge();

    if ( start_x > end_x )
    {
        i = start_x;
        start_x = end_x;
        end_x = i;
    }

    if ( start_y > end_y )
    {
        i = start_y;
        start_y = end_y;
        end_y = i;
    }

    for ( i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );


        if ( !WorldToScreen( pZombie->GetAbsOrigin(), screen, x, y ) )
            continue;

        if ( x > start_x && x < end_x && y > start_y && y < end_y )
        {
            vZombieIndices.AddToTail( pZombie->entindex() );
        }
    }

    // We didn't select anything.
    if ( !vZombieIndices.Count() )
    {
        if ( !bSticky )
        {
            engine->ClientCmd( "zm_cmd_unselectall" );
        }

        return;
    }

    char cmdbuffer[512];
    cmdbuffer[0] = 0;

    for ( int i = 0; i < vZombieIndices.Count(); i++ )
    {
        sprintf_s( cmdbuffer, sizeof( cmdbuffer ), "%s%i ", cmdbuffer, vZombieIndices.Element( i ) );
    }

    if ( cmdbuffer[0] )
    {
        engine->ClientCmd( VarArgs( "zm_cmd_selectmult %s %s",  bSticky ? "1": "0", cmdbuffer ) );
    }
}

void CZMFrame::FindZMObject( int x, int y, bool bSticky )
{
    C_BaseEntity* list[32];
    Ray_t ray;
    trace_t trace;
    int i;
    C_ZMEntBaseUsable* pUsable;
    C_ZMBaseZombie* pZombie;

    
    TraceScreenToWorld( x, y, &trace, nullptr );


    ray.Init( MainViewOrigin(), trace.endpos,
        Vector( -1.0, -1.0, -1.0 ),
        Vector( 1.0, 1.0, 1.0 ) );

    
    int found = UTIL_EntitiesAlongRay( list, ARRAYSIZE( list ), ray, 0 );


    bool bHit = false;

    for ( i = 0; i < found; i++ )
    {
        pUsable = dynamic_cast<C_ZMEntBaseUsable*>( list[i] );

        if ( pUsable )
        {
            C_ZMEntZombieSpawn* pSpawn = dynamic_cast<C_ZMEntZombieSpawn*>( pUsable );

            if ( pSpawn )
            {
                if ( m_pBuildMenu )
                {
                    m_pBuildMenu->SetSpawnIndex( pSpawn->entindex() );
                    m_pBuildMenu->SetZombieFlags( pSpawn->GetZombieFlags() );
                    m_pBuildMenu->ShowPanel( true );
                }

                bHit = true;
                return;
            }


            C_ZMEntManipulate* pTrap = dynamic_cast<C_ZMEntManipulate*> ( pUsable );

            if ( pTrap )
            {
                if ( m_pManiMenu )
                {
                    m_pManiMenu->SetTrapIndex( pTrap->entindex() );
                    m_pManiMenu->SetDescription( "Activate trap." );
                    m_pManiMenu->SetCost( pTrap->GetCost() );
                    m_pManiMenu->SetTrapCost( pTrap->GetTrapCost() );
                    m_pManiMenu->SetTrapPos( pTrap->GetAbsOrigin() );
                    m_pManiMenu->ShowPanel( true );
                }

                bHit = true;
                return;
            }


            /*engine->ClientCmd( VarArgs( "zm_cmd_select %i%s",
                pUsable->entindex(),
                bSticky ? " 1" : "" ) );
            bHit = true;*/
            return;
        }


        pZombie = dynamic_cast<C_ZMBaseZombie*>( list[i] );

        if ( pZombie )
        {
            engine->ClientCmd( VarArgs( "zm_cmd_select %i%s",
                pZombie->entindex(),
                bSticky ? " 1" : "" ) );
            bHit = true;
        }
    }

    // If we didn't hit anything special then just unselect everything.
    if ( !bHit && !bSticky )
        engine->ClientCmd( "zm_cmd_unselectall" );
}

void CZMFrame::CloseChildMenus()
{
    if ( m_pBuildMenu && m_pBuildMenu->IsVisible() )
    {
        m_pBuildMenu->Close();
    }

    if ( m_pManiMenu && m_pManiMenu->IsVisible() )
    {
        m_pManiMenu->Close();
    }
}

bool CZMFrame::WorldToScreen( const Vector& pos, Vector& screen, int& x, int& y )
{
    int behind = ScreenTransform( pos, screen );

    if ( !behind )
    {
        int w = ScreenWidth();
        int h = ScreenHeight();
        x =  0.5 * screen[0] * w;
        y = -0.5 * screen[1] * h;
        x += 0.5 * w;
        y += 0.5 *	h;
    }

    return !behind ? true : false;
}