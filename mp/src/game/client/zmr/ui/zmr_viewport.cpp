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
#include "zmr/zmr_util.h"




#include "zmr_viewport.h"



// ZMRTODO: See if these works properly.
#define MASK_ZMVIEW             ( CONTENTS_SOLID | CONTENTS_MOVEABLE ) // When testing box select.
#define MASK_ZMSELECTUSABLE     MASK_SOLID // When left clicking (not setting rallypoint, etc.)
#define MASK_ZMTARGET           MASK_SOLID // When right clicking.
#define MASK_ZMCREATE           MASK_SOLID // When left clicking.

ConVar zm_cl_poweruser_boxselect( "zm_cl_poweruser_boxselect", "0", FCVAR_ARCHIVE, "Select zombies through walls with box select." );
ConVar zm_cl_poweruser( "zm_cl_poweruser", "0", FCVAR_ARCHIVE, "Select spawns/traps/zombies through walls." );
ConVar zm_cl_hidemouseinscore( "zm_cl_hidemouseinscore", "1", FCVAR_ARCHIVE, "Is mouse input disabled while having scoreboard open?" );


ConVar zm_cl_usenewmenus( "zm_cl_usenewmenus", "1", FCVAR_ARCHIVE, "Use new ZM menus?" );



DECLARE_HUDELEMENT( CZMFrame );


extern CZMBuildMenu* g_pBuildMenu;


extern IViewPort* gViewPortInterface;

CZMFrame* g_pZMView = nullptr;

CON_COMMAND( zm_observermode, "" )
{
    if ( g_pZMView )
    {
        bool state = !g_pZMView->IsVisible();

        if ( args.ArgC() > 1 ) state = atoi( args.Arg( 1 ) ) ? true : false;


        g_pZMView->SetVisible( state );
    }
}


class CTraceFilterNoNPCs : public CTraceFilterSimple
{
public:
	CTraceFilterNoNPCs( const IHandleEntity *passentity, int collisionGroup )
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



    m_BoxSelect = new CZMBoxSelect( this );

	m_pZMControl = new CZMHudControlPanel();

	m_pManiMenu = new CZMManiMenu( this ); 
	m_pManiMenuNew = new CZMManiMenuNew( this ); 
	m_pBuildMenu = new CZMBuildMenu( this ); 
}

CZMFrame::~CZMFrame()
{
    delete m_BoxSelect;

    delete m_pZMControl;

    delete m_pManiMenu;
    delete m_pManiMenuNew;
    delete m_pBuildMenu;
}

void CZMFrame::Init()
{
    Reset();
}

void CZMFrame::VidInit()
{
    Reset();
}

void CZMFrame::Reset()
{
    //m_pZMControl = GET_HUDELEMENT( CZMHudControlPanel );
    if ( m_pZMControl )
    {
        m_pZMControl->PositionButtons();
        m_pZMControl->PositionComboBox();
    }
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


    if ( print )
    {
        switch ( mode )
        {
        case ZMCLICKMODE_HIDDEN :
            ZMClientUtil::PrintNotify( "#ZMClickModeHidden" );
            break;
        case ZMCLICKMODE_PHYSEXP :
            ZMClientUtil::PrintNotify( "#ZMClickModeExp" );
            break;
        case ZMCLICKMODE_RALLYPOINT :
            ZMClientUtil::PrintNotify( "#ZMClickModeRally" );
            break;
        default : break;
        }
    }


    m_iClickMode = mode;
}

CZMManiMenuBase* CZMFrame::GetManiMenu()
{
    // Tertiary tries to make me cast... pshh, I'll just use good ol' if's. 
    if ( zm_cl_usenewmenus.GetBool() )
    {
        return m_pManiMenuNew;
    }
    else
    {
        return m_pManiMenu;
    }
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

void CZMFrame::OnMouseWheeled( int delta )
{
    BaseClass::OnMouseWheeled( delta );


    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    
    if ( pPlayer )
        pPlayer->SetMouseWheelMove( (float)delta );
}

void CZMFrame::OnCommand( const char* command )
{
    if ( Q_stricmp( command, "TAB_POWERS" ) == 0 )
    {
        if ( m_pZMControl ) m_pZMControl->UpdateTabs( CZMHudControlPanel::TAB_POWERS );
    }
    else if ( Q_stricmp( command, "TAB_MODES" ) == 0 )
    {
        if ( m_pZMControl ) m_pZMControl->UpdateTabs( CZMHudControlPanel::TAB_MODES );
    }
    else if ( Q_stricmp( command, "TAB_ZEDS" ) == 0 )
    {
        if ( m_pZMControl ) m_pZMControl->UpdateTabs( CZMHudControlPanel::TAB_ZEDS );
    }
    else if ( Q_stricmp( command, "MODE_SELECT_ALL" ) == 0 )
    {
        ZMClientUtil::SelectAllZombies();
    }
    else if ( Q_stricmp( command, "MODE_DEFENSIVE" ) == 0 )
    {
        engine->ClientCmd( VarArgs( "zm_cmd_zombiemode %i", ZOMBIEMODE_DEFEND ) );
    }
    else if ( Q_stricmp( command, "MODE_OFFENSIVE" ) == 0 )
    {
        engine->ClientCmd( VarArgs( "zm_cmd_zombiemode %i", ZOMBIEMODE_OFFENSIVE ) );
    }
    else if ( Q_stricmp( command, "MODE_POWER_DELETEZOMBIES" ) == 0 )
    {
        engine->ClientCmd( "zm_cmd_delete" );
    }
    else if ( Q_stricmp( command, "MODE_POWER_SPOTCREATE" ) == 0 )
    {
        if ( g_pZMView ) g_pZMView->SetClickMode( ZMCLICKMODE_HIDDEN );
    }
    else if ( Q_stricmp( command, "MODE_POWER_PHYSEXP" ) == 0 )
    {
        if ( g_pZMView ) g_pZMView->SetClickMode( ZMCLICKMODE_PHYSEXP );
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
    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();

    if ( !pPlayer ) return;


    if ( !pPlayer->IsZM() && IsVisible() )
    {
        SetVisible( false );
    }

    if ( !IsVisible() )
    {
        if ( IsMouseInputEnabled() )
            SetMouseInputEnabled( false );

        return;
    }


    if ( pPlayer->m_nButtons & IN_SCORE && zm_cl_hidemouseinscore.GetBool() )
    {
        if ( IsMouseInputEnabled() )
            SetMouseInputEnabled( false );
    }
    else
    {
        if ( !IsMouseInputEnabled() )
            SetMouseInputEnabled( true );
    }



    if ( IsMouseInputEnabled() )
    {
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
}

void CZMFrame::TraceScreenToWorld( int mx, int my, trace_t* res, CTraceFilterSimple* filter, int mask )
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

    
    if ( filter )
    {
        UTIL_TraceLine( MainViewOrigin(), MainViewOrigin() + ray * MAX_TRACE_LENGTH, mask, filter, res );
    }
    else
    {
        UTIL_TraceLine( MainViewOrigin(), MainViewOrigin() + ray * MAX_TRACE_LENGTH, mask, pPlayer, COLLISION_GROUP_NONE, res );
    }
}

void CZMFrame::OnLeftClick()
{
    int mx, my;
    ::input->GetFullscreenMousePos( &mx, &my );


    m_BoxSelect->SetEnabled( true );
    m_BoxSelect->SetStart( mx, my );


    if ( GetClickMode() == ZMCLICKMODE_NORMAL ) return;

    trace_t trace;
    CTraceFilterNoNPCsOrPlayer filter( nullptr, COLLISION_GROUP_NONE );
    TraceScreenToWorld( mx, my, &trace, &filter, MASK_ZMCREATE );

    if ( trace.fraction != 1.0f )
    {
        Vector pos = trace.endpos;

        switch ( GetClickMode() )
        {
        case ZMCLICKMODE_TRAP :
            if ( GetManiMenu() )
                engine->ClientCmd( VarArgs( "zm_cmd_createtrigger %i %.1f %.1f %.1f",
                    GetManiMenu()->GetTrapIndex(),
                    pos[0],
                    pos[1],
                    pos[2] ) );
            break;


        case ZMCLICKMODE_RALLYPOINT :
            if ( GetBuildMenu() )
            {
                engine->ClientCmd( VarArgs( "zm_cmd_setrally %i %.1f %.1f %.1f",
                    GetBuildMenu()->GetLastSpawnIndex(),
                    pos[0],
                    pos[1],
                    pos[2] ) );


                GetBuildMenu()->ShowPanel( true );
            }
            break;


        case ZMCLICKMODE_HIDDEN :
            engine->ClientCmd( VarArgs( "zm_cmd_createhidden %.1f %.1f %.1f",
                pos[0],
                pos[1],
                pos[2] ) );
            break;

        case ZMCLICKMODE_PHYSEXP :
            pos = pos + trace.plane.normal * 1.0f;
            engine->ClientCmd( VarArgs( "zm_cmd_physexp %.1f %.1f %.1f",
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

void CZMFrame::OnRightClick()
{
    m_BoxSelect->SetEnabled( false );


    int mx, my;
    ::input->GetFullscreenMousePos( &mx, &my );
    
    trace_t trace;
    CTraceFilterNoNPCs filter( nullptr, COLLISION_GROUP_NONE );
    TraceScreenToWorld( mx, my, &trace, &filter, MASK_ZMTARGET );


    Vector end = trace.endpos;

    CBaseEntity* pTarget = trace.m_pEnt;

    // We hit an entity, let's see if we can target it.
    if ( trace.DidHitNonWorldEntity() && pTarget )
    {
        bool bPlayer = false;
        bool bObj = false;

        if ( pTarget->IsPlayer() )
        {
            bPlayer = true; // ZMRTODO: Target player.
        }
        else
        {
            IPhysicsObject* phys = pTarget->VPhysicsGetObject();

            bObj = (phys && phys->IsMoveable()) || !pTarget->IsBaseTrain();
        }

        if ( bObj )
        {
            engine->ClientCmd( VarArgs( "zm_cmd_target %i %.1f %.1f %.1f", pTarget->entindex(), end[0], end[1], end[2] ) );


            FX_AddQuad( end + trace.plane.normal * 2.0f,
                        trace.plane.normal,
                        8.0f,
                        24.0f,
                        0, 
                        0.2f,
                        0.8f,
                        0.4f,
                        random->RandomInt( 0, 360 ), 
                        0,
                        Vector( 1.0f, 1.0f, 1.0f ), 
                        0.2f, 
                        "zmr_effects/target_break",
                        (FXQUAD_BIAS_ALPHA) );

            return;
        }
    }


    // Otherwise, just move there.
    engine->ClientCmd( VarArgs( "zm_cmd_move %.1f %.1f %.1f %.1f",
        end[0], end[1], end[2],
        clamp( (ZMClientUtil::GetSelectedZombieCount() - 1) * 2.5f, 0.0f, 92.0f ) ) );

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
    trace_t trace;
    CTraceFilterNoNPCsOrPlayer filter( nullptr, COLLISION_GROUP_NONE );

    CUtlVector<C_ZMBaseZombie*> vZombies;
    vZombies.Purge();

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

        // Do we see the mad man?
        if ( !zm_cl_poweruser_boxselect.GetBool() )
        {
            UTIL_TraceLine( MainViewOrigin(), pZombie->GetAbsOrigin() + Vector( 0, 0, 8 ), MASK_ZMVIEW, &filter, &trace );

            if ( trace.fraction != 1.0f && !trace.startsolid ) continue;
        }

        if ( !ZMClientUtil::WorldToScreen( pZombie->GetAbsOrigin(), screen, x, y ) )
            continue;

        if ( x > start_x && x < end_x && y > start_y && y < end_y )
        {
            vZombies.AddToTail( pZombie );
        }
    }


    ZMClientUtil::SelectZombies( vZombies, bSticky );
}

void CZMFrame::FindZMObject( int x, int y, bool bSticky )
{
    C_BaseEntity* list[32];
    Ray_t ray;
    trace_t trace;
    int i;
    C_ZMEntBaseUsable* pUsable;
    C_ZMBaseZombie* pZombie;

    TraceScreenToWorld( x, y, &trace, nullptr, zm_cl_poweruser.GetBool() ? 0 : MASK_ZMSELECTUSABLE );


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
                if ( GetBuildMenu() )
                {
                    GetBuildMenu()->ShowMenu( pSpawn );
                }

                bHit = true;
                return;
            }


            C_ZMEntManipulate* pTrap = dynamic_cast<C_ZMEntManipulate*> ( pUsable );

            if ( pTrap )
            {
                if ( GetManiMenu() )
                {
                    GetManiMenu()->ShowMenu( pTrap );

                    // Tell server we've opened this menu to get the real trap description.
                    if ( !*pTrap->GetDescription() )
                        engine->ClientCmd( VarArgs( "zm_cmd_openmanimenu %i", pTrap->entindex() ) );
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
            ZMClientUtil::SelectSingleZombie( pZombie, bSticky );

            bHit = true;
            return;
        }
    }

    // If we didn't hit anything special then just unselect everything.
    if ( !bHit && !bSticky )
        ZMClientUtil::DeselectAllZombies();
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

    if ( m_pManiMenuNew && m_pManiMenuNew->IsVisible() )
    {
        m_pManiMenuNew->Close();
    }
}
