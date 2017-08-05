#include "cbase.h"

#include "ClientEffectPrecacheSystem.h"



#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_global_shared.h"
#include "c_zmr_zombiebase.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMBaseZombie, DT_ZM_BaseZombie, CZMBaseZombie )
	RecvPropInt( RECVINFO( m_iSelectorIndex ) ),
	RecvPropFloat( RECVINFO( m_flHealthRatio ) ),
END_RECV_TABLE()

BEGIN_DATADESC( C_ZMBaseZombie )
END_DATADESC()


// ZMRTODO: Replace these
#define MAT_HPCIRCLE        "effects/zm_healthring"
#define MAT_INNERCIRCLE     "effects/zombie_select"


CLIENTEFFECT_REGISTER_BEGIN( PrecacheZMSelectEffect )
//CLIENTEFFECT_MATERIAL( "effects/spark" )
//CLIENTEFFECT_MATERIAL( "effects/gunshiptracer" )
CLIENTEFFECT_MATERIAL( MAT_HPCIRCLE )
CLIENTEFFECT_MATERIAL( MAT_INNERCIRCLE )
CLIENTEFFECT_REGISTER_END()


C_ZMBaseZombie::C_ZMBaseZombie()
{
    g_pZombies->AddToTail( this );


    m_fxHealth = nullptr;
    m_fxInner = nullptr;
    

    // Always create FX.
    m_fxHealth = new CZMCharCircle();
    m_fxHealth->SetYaw( 0.0f );
    m_fxHealth->SetMaterial( MAT_HPCIRCLE );
    m_fxHealth->SetSize( 16.0f );

    m_fxInner = new CZMCharCircle();
    m_fxInner->SetYaw( 0.0f );
    m_fxInner->SetMaterial( MAT_INNERCIRCLE );
    m_fxInner->SetSize( 16.0f );
}

C_ZMBaseZombie::~C_ZMBaseZombie()
{
    g_pZombies->FindAndRemove( this );

    delete m_fxHealth;
    delete m_fxInner;
}

void C_ZMBaseZombie::Spawn( void )
{
    BaseClass::Spawn();

    // This allows the client to make us bleed and spawn blood decals.
    // Possibly add option to turn off?
    m_takedamage = DAMAGE_YES;
}

int C_ZMBaseZombie::DrawModel( int flags )
{
    CZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );

    if ( !pPlayer || !pPlayer->IsZM() )
    {
        return BaseClass::DrawModel( flags );
    }
        

    float ratio = m_flHealthRatio > 1.0f ? 1.0f : m_flHealthRatio;
    if ( ratio < 0.0f ) ratio = 0.0f;

    float g = ratio;
    float r = 1.0f - g;

    float alpha = (m_iSelectorIndex > 0 && m_iSelectorIndex == GetLocalPlayerIndex()) ? 0.8f : 0.1f;

    if ( m_fxInner )
    {
        m_fxInner->SetColor( r, g, 0 );
        m_fxInner->SetAlpha( alpha );
        m_fxInner->SetPos( GetAbsOrigin() + Vector( 0.0f, 0.0f, 3.0f ) );
        m_fxInner->Draw();
    }


    if ( m_fxHealth )
    {
        m_fxHealth->SetColor( r, g, 0 );
        m_fxHealth->SetAlpha( alpha );
        m_fxHealth->SetPos( GetAbsOrigin() + Vector( 0.0f, 0.0f, 3.0f ) );
        m_fxHealth->Draw();
    }

    return BaseClass::DrawModel( flags );
}

/*void C_ZMBaseZombie::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
    BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}*/