#include "cbase.h"
#include "view.h" // MainViewForward
#include "iclientmode.h" // g_pClientMode
#include "hud.h" // CHudElement
#include "in_buttons.h"
#include "input.h"
#include "clienteffectprecachesystem.h"
#include "fx_quad.h"

#include "zmr/npcs/zmr_zombiebase_shared.h"
#include "zmr/c_zmr_util.h"
#include "zmr_zmview_base.h"




CZMViewBase* g_pZMView = nullptr;


ConVar zm_cl_zmview_doubleclick( "zm_cl_zmview_doubleclick", "0.4", FCVAR_ARCHIVE );


ConVar zm_cl_poweruser_boxselect( "zm_cl_poweruser_boxselect", "0", FCVAR_ARCHIVE, "Select zombies through walls with box select." );
ConVar zm_cl_poweruser( "zm_cl_poweruser", "0", FCVAR_ARCHIVE, "Select spawns/traps/zombies through walls." );
ConVar zm_cl_hidemouseinscore( "zm_cl_hidemouseinscore", "1", FCVAR_ARCHIVE, "Is mouse input disabled while having scoreboard open?" );





#define MAT_RING            "effects/zm_ring"
#define MAT_TARGETBREAK     "zmr_effects/target_break"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheZMViewEffects )
CLIENTEFFECT_MATERIAL( MAT_RING )
CLIENTEFFECT_MATERIAL( MAT_TARGETBREAK )
CLIENTEFFECT_REGISTER_END()




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

		    return !pEntity->IsNPCR();
	    }

	    return false;
    }
};



CON_COMMAND( zm_observermode, "" )
{
    if ( g_pZMView )
    {
        bool state = !g_pZMView->IsVisible();

        if ( args.ArgC() > 1 ) state = atoi( args.Arg( 1 ) ) ? true : false;


        g_pZMView->SetVisible( state );
    }
}

CON_COMMAND( zm_hiddenspawn, "" )
{
    if ( g_pZMView )
    {
        g_pZMView->SetClickMode( ZMCLICKMODE_HIDDEN );
    }
}


CZMViewBase::CZMViewBase( const char* pElementName ) : CHudElement( pElementName ), CZMFramePanel( g_pClientMode->GetViewport(), pElementName )
{
    // Proportional doesn't stretch with widescreen anyway
    SetProportional( false );
    SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );


    SetVisible( false );
    SetBorder( nullptr );
    SetPaintBackgroundEnabled( false );

    SetHiddenBits( HIDEHUD_WEAPONSELECTION );


    // We cannot enable input. Especially not keyboard input.
    // Otherwise the input would be swallowed by us, so players binding shit to let's say mouse4 wouldn't work.
    // Instead, our clientmode will call the necessary functions.
    SetKeyBoardInputEnabled( false );
    SetMouseInputEnabled( true );



    m_BoxSelect = new CZMBoxSelect( this );
    m_LineTool = new CZMLineTool( this );
}

CZMViewBase::~CZMViewBase()
{
    delete m_BoxSelect;
    delete m_LineTool;
}

void CZMViewBase::LevelInit()
{
    m_bDraggingLeft = m_bDraggingRight = false;
    m_flLastLeftClick = m_flLastRightClick = 0.0f;
    SetClickMode( ZMCLICKMODE_NORMAL );

    CloseChildMenus();
}

bool CZMViewBase::ShouldDraw()
{
    return IsVisible();
}

void CZMViewBase::SetClickMode( ZMClickMode_t mode, bool print )
{
    if ( mode == GetClickMode() ) return;


    if ( print )
    {
        switch ( mode )
        {
        case ZMCLICKMODE_TRAP :
            ZMClientUtil::PrintNotify( "#ZMClickModeTrap", ZMCHATNOTIFY_ZM );
            break;
        case ZMCLICKMODE_HIDDEN :
            ZMClientUtil::PrintNotify( "#ZMClickModeHidden", ZMCHATNOTIFY_ZM );
            break;
        case ZMCLICKMODE_PHYSEXP :
            ZMClientUtil::PrintNotify( "#ZMClickModeExp", ZMCHATNOTIFY_ZM );
            break;
        case ZMCLICKMODE_RALLYPOINT :
            ZMClientUtil::PrintNotify( "#ZMClickModeRally", ZMCHATNOTIFY_ZM );
            break;
        case ZMCLICKMODE_AMBUSH :
            ZMClientUtil::PrintNotify( "#ZMClickModeAmbush", ZMCHATNOTIFY_ZM );
        default : break;
        }
    }


    m_iClickMode = mode;
}

float CZMViewBase::GetDoubleClickDelta() const
{
    return zm_cl_zmview_doubleclick.GetFloat();
}

bool CZMViewBase::IsDoubleClickLeft() const
{
    return (gpGlobals->curtime - m_flLastLeftClick) <= GetDoubleClickDelta();
}

bool CZMViewBase::IsDoubleClickRight() const
{
    return (gpGlobals->curtime - m_flLastRightClick) <= GetDoubleClickDelta();
}

bool CZMViewBase::IsDraggingLeft() const
{
    return m_bDraggingLeft;
}

bool CZMViewBase::IsDraggingRight() const
{
    return m_bDraggingRight;
}

void CZMViewBase::CloseChildMenus()
{

}

void CZMViewBase::HideMouseTools()
{
    if ( m_BoxSelect )
        m_BoxSelect->SetVisible( false );
    if ( m_LineTool )
        m_LineTool->SetVisible( false );
}



void CZMViewBase::SetVisible( bool state )
{
    BaseClass::SetVisible( state );

    engine->ClientCmd( "-left" );
    engine->ClientCmd( "-right" );
    engine->ClientCmd( "-lookup" );
    engine->ClientCmd( "-lookdown" );


    HideMouseTools();
}

bool CZMViewBase::IsVisible()
{
    return BaseClass::IsVisible();
}

void CZMViewBase::OnCursorMoved( int x, int y )
{
    if ( IsDraggingLeft() )
    {
        m_BoxSelect->SetEnd( x, y );
    }

    if ( IsDraggingRight() )
    {
        m_LineTool->SetEnd( x, y );
    }
}

void CZMViewBase::OnMouseReleased( MouseCode code )
{
    switch ( code )
    {
    case MOUSE_RIGHT :
        OnRightRelease();
        m_flLastRightClick = gpGlobals->curtime;
        m_bDraggingRight = false;
        break;
    case MOUSE_LEFT :
        OnLeftRelease();
        m_flLastLeftClick = gpGlobals->curtime;
        m_bDraggingLeft = false;
        break;
    default : break;
    }
}

void CZMViewBase::OnMousePressed( MouseCode code )
{
    CloseChildMenus();


    switch ( code )
    {
    case MOUSE_RIGHT :
        OnRightClick();
        m_bDraggingRight = true;
        break;
    case MOUSE_LEFT :
        OnLeftClick();
        m_bDraggingLeft = true;
        break;
    default : break;
    }
}

void CZMViewBase::OnMouseWheeled( int delta )
{
    BaseClass::OnMouseWheeled( delta );


    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    
    if ( pPlayer )
        pPlayer->SetMouseWheelMove( (float)delta );
}

void CZMViewBase::OnThink()
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



void CZMViewBase::OnLeftClick()
{
    int mx, my;
    ::input->GetFullscreenMousePos( &mx, &my );


    m_BoxSelect->SetVisible( true );
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

        case ZMCLICKMODE_AMBUSH :
            pos = pos + trace.plane.normal * 1.0f;
            engine->ClientCmd( VarArgs( "zm_cmd_createambush %.1f %.1f %.1f",
                pos[0],
                pos[1],
                pos[2]  ) );

        default : break;
        }
    }


    SetClickMode( ZMCLICKMODE_NORMAL );
}

void CZMViewBase::OnLeftRelease()
{
    m_BoxSelect->SetVisible( false );


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

void CZMViewBase::OnRightClick()
{
    m_BoxSelect->SetVisible( false );

    int mx, my;
    ::input->GetFullscreenMousePos( &mx, &my );

    m_LineTool->SetVisible( true );
    m_LineTool->SetStart( mx, my );
    m_LineTool->SetEnd( mx, my );
}

void CZMViewBase::OnRightRelease()
{
    m_LineTool->SetVisible( false );


    int mx, my;
    ::input->GetFullscreenMousePos( &mx, &my );

    m_LineTool->SetEnd( mx, my );

    // We were dragging, move units into a line.
    if ( IsDraggingRight() && m_LineTool->IsValidLine( 10 ) )
    {
        DoMoveLine();
        return;
    }

    // We're just commanding zombies.
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
            bool bForceBreak = IsDoubleClickRight();

            engine->ClientCmd( VarArgs( "zm_cmd_target %i %i %.1f %.1f %.1f",
                pTarget->entindex(),
                bForceBreak ? 1 : 0,
                end[0], end[1], end[2] ) );


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
                        MAT_TARGETBREAK,
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
                MAT_RING,
                (FXQUAD_BIAS_ALPHA) );

}

void CZMViewBase::DoMoveLine()
{
    int x1, y1, x2, y2;
    Vector start, end;
    trace_t trace;
    CTraceFilterNoNPCs filter( nullptr, COLLISION_GROUP_NONE );

    m_LineTool->GetLine( x1, y1, x2, y2 );

    TraceScreenToWorld( x1, y1, &trace, &filter, MASK_ZMTARGET );
    start = trace.endpos;

    TraceScreenToWorld( x2, y2, &trace, &filter, MASK_ZMTARGET );
    end = trace.endpos;

    engine->ClientCmd( VarArgs( "zm_cmd_moveline %.1f %.1f %.1f %.1f %.1f %.1f",
        start[0], start[1], start[2],
        end[0], end[1], end[2] ) );
        //clamp( (ZMClientUtil::GetSelectedZombieCount() - 1) * 2.5f, 0.0f, 92.0f ) ) );
}

void CZMViewBase::FindZombiesInBox( int start_x, int start_y, int end_x, int end_y, bool bSticky )
{
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

    g_ZombieManager.ForEachAliveZombie( [ &vZombies, &trace, &filter, &screen, &x, &y, start_x, start_y, end_x, end_y ]( C_ZMBaseZombie* pZombie )
    {
        // Do we see the mad man?
        if ( !zm_cl_poweruser_boxselect.GetBool() )
        {
            UTIL_TraceZMView( &trace, pZombie->GetAbsOrigin() + Vector( 0, 0, 8 ), MASK_ZMVIEW, &filter );

            if ( trace.fraction != 1.0f && !trace.startsolid ) return;
        }

        if ( !ZMClientUtil::WorldToScreen( pZombie->GetAbsOrigin(), screen, x, y ) )
            return;

        if ( x > start_x && x < end_x && y > start_y && y < end_y )
        {
            vZombies.AddToTail( pZombie );
        }
    } );


    ZMClientUtil::SelectZombies( vZombies, bSticky );
}

void CZMViewBase::FindZMObject( int x, int y, bool bSticky )
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

            if ( pSpawn && !pSpawn->IsEffectActive( EF_NODRAW ) )
            {
                if ( GetBuildMenu() )
                {
                    GetBuildMenu()->ShowMenu( pSpawn );
                }

                bHit = true;
                return;
            }


            C_ZMEntManipulate* pTrap = dynamic_cast<C_ZMEntManipulate*> ( pUsable );

            if ( pTrap && !pTrap->IsEffectActive( EF_NODRAW ) )
            {
                if ( GetManiMenu() )
                {
                    GetManiMenu()->ShowMenu( pTrap );
                }

                bHit = true;
                return;
            }


            /*engine->ClientCmd( VarArgs( "zm_cmd_select %i%s",
                pUsable->entindex(),
                bSticky ? " 1" : "" ) );
            bHit = true;*/
            continue;
        }


        pZombie = ToZMBaseZombie( list[i] );

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





void CZMViewBase::TraceScreenToWorld( int mx, int my, trace_t* res, CTraceFilterSimple* filter, int mask )
{
    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();

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

    UTIL_TraceZMView( res, MainViewOrigin() + ray * MAX_COORD_FLOAT, mask, filter, pPlayer );
}

void UTIL_TraceZMView( trace_t* trace, Vector endpos, int mask, CTraceFilterSimple* filter, C_BaseEntity* pEnt, int collisionGroup )
{
    Vector pos = MainViewOrigin();

    const bool startinside = (UTIL_PointContents( pos ) & CONTENTS_SOLID) == 0;

    // If we started inside the world, only trace through nodraw surfaces.
    // It is possible to be able to see some things behind the sky while inside the world but let's stay safe.
    const unsigned short invalidsurfaces = startinside ? SURF_NODRAW : (SURF_SKY | SURF_NODRAW);

    Vector dir = (endpos - pos).Normalized();

    // Keep tracing until we hit something that the player can't see through.
    while ( true )
    {
        // NOTE: You cannot rely on trace startsolid! This is why we check for point contents here.
        // It only works if you start inside a brush leaf (yes, the outside world also has leafs), simply being outside is apparently not solid.
        bool bOutsideWorld = (UTIL_PointContents( pos ) & CONTENTS_SOLID) != 0;

        if ( filter )
        {
            UTIL_TraceLine( pos, endpos, mask, filter, trace );
        }
        else
        {
            UTIL_TraceLine( pos, endpos, mask, pEnt, collisionGroup, trace );
        }

        // Didn't hit anything.
        if ( trace->fraction == 1.0f )
            break;

        // Didn't start outside world, should be a valid trace.
        if ( !bOutsideWorld )
        {
            if ( !trace->DidHitWorld() || (trace->surface.flags & invalidsurfaces) == 0 )
            {
                break;
            }
        }

        // Go to the next spot to start at.
        // A bit hacky way of doing this. Trace endpos doesn't seem to be the pos where we left solid.
        float dist = pos.DistTo( endpos );
        float frac = trace->startsolid ? trace->fractionleftsolid : trace->fraction;

        pos += dir * (frac * dist) + dir * 2.0f; // Add a bit of extra so we definitely don't hit the same wall again.
    }
}
