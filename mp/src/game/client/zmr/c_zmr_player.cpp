#include "cbase.h"

#include "baseviewport.h"
#include "ClientEffectPrecacheSystem.h"


#include "zmr/ui/zmr_viewport.h"

#include "npcs/c_zmr_zombiebase.h"
#include "zmr/zmr_global_shared.h"
#include "zmr/c_zmr_entities.h"

#include "c_zmr_player.h"


// ZMRTODO: Replace these
#define MAT_HPCIRCLE        "effects/zm_healthring"
#define MAT_INNERFLARE      "effects/yellowflare"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectZMPlayerEffect )
CLIENTEFFECT_MATERIAL( MAT_HPCIRCLE )
CLIENTEFFECT_MATERIAL( MAT_INNERFLARE )
CLIENTEFFECT_REGISTER_END()


ConVar zm_cl_participation( "zm_cl_participation", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Your participation setting. 0 = Want to be ZM, 1 = Only human, 2 = Only spectator" );


IMPLEMENT_CLIENTCLASS_DT( C_ZMPlayer, DT_ZM_Player, CZMPlayer )
    RecvPropInt( RECVINFO( m_nResources ) ),
END_RECV_TABLE()

//BEGIN_PREDICTION_DATA( C_ZMPlayer )
//END_PREDICTION_DATA();


C_ZMPlayer::C_ZMPlayer()
{
    m_fxHealth = new CZMCharCircle();
    m_fxHealth->SetYaw( 0.0f );
    m_fxHealth->SetMaterial( MAT_HPCIRCLE );
    m_fxHealth->SetSize( 16.0f );


    m_fxInner = new CZMCharCircle();
    m_fxInner->SetYaw( 0.0f );
    m_fxInner->SetColor( 1.0f, 1.0f, 1.0f );
    m_fxInner->SetAlpha( 0.8f );
    m_fxInner->SetMaterial( MAT_INNERFLARE );
    m_fxInner->SetSize( 22.0f );
}

C_ZMPlayer::~C_ZMPlayer()
{
    delete m_fxHealth;
    delete m_fxInner;
}

C_ZMPlayer* C_ZMPlayer::GetLocalPlayer()
{
    return static_cast<C_ZMPlayer*>( C_BasePlayer::GetLocalPlayer() );
}

void C_ZMPlayer::TeamChange( int iNewTeam )
{
    // ZMRTODO: Test if there are any cases when TeamChange isn't fired!!!
    BaseClass::TeamChange( iNewTeam );


    // Update ZM entities' visibility.
    // How visibility works is when player enters a leaf that can see given entity, ShouldDraw is fired.
    // However, if the player changes their team and they remain in the same leaf, the previous ShouldDraw is "active".
    // This makes sure we get updated on our ZM entities when changing teams, so orbs get drawn when changing to ZM and not drawn when changing to human/spec.
    DevMsg( "Updating ZM entity visibility...\n" );

    // The team number hasn't been updated yet.
    int iOldTeam = GetTeamNumber();
    C_BaseEntity::ChangeTeam( iNewTeam );

    C_ZMEntBaseSimple* pZMEnt;
    for ( C_BaseEntity* pEnt = ClientEntityList().FirstBaseEntity(); pEnt; pEnt = ClientEntityList().NextBaseEntity( pEnt ) )
    {
        pZMEnt = dynamic_cast<C_ZMEntBaseSimple*>( pEnt );
        if ( pZMEnt )
        {
            pZMEnt->UpdateVisibility();
        }
    }

    // Reset back to old team just in case something uses it.
    C_BaseEntity::ChangeTeam( iOldTeam );
    


    if ( g_pZMView )
        g_pZMView->SetVisible( iNewTeam == ZMTEAM_ZM );
}

int C_ZMPlayer::DrawModel( int flags )
{
    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    if ( !pPlayer || !pPlayer->IsZM() )
    {
        return BaseClass::DrawModel( flags );
    }


    float ratio = (GetHealth() > 0 ? GetHealth() : 1) / 100.0f;

    float g = ratio;
    float r = 1.0f - g;


    if ( m_fxInner )
    {
        m_fxInner->SetPos( GetAbsOrigin() + Vector( 0.0f, 0.0f, 3.0f ) );
        m_fxInner->SetYaw( random->RandomFloat( 0.0f, 360.0f ) );
        m_fxInner->Draw();
    }


    if ( m_fxHealth )
    {
        m_fxHealth->SetAlpha( 0.5f );
        m_fxHealth->SetColor( r, g, 0 );
        m_fxHealth->SetPos( GetAbsOrigin() + Vector( 0.0f, 0.0f, 3.0f ) );
        m_fxHealth->Draw();
    }

    return BaseClass::DrawModel( flags );
}