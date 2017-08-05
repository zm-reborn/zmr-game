#include "cbase.h"

#include "ClientEffectPrecacheSystem.h"



#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_global_shared.h"
#include "c_zmr_zombiebase.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMBaseZombie, DT_ZM_BaseZombie, CZMBaseZombie )
	RecvPropInt( RECVINFO( m_iSelectorIndex ) ),
END_RECV_TABLE()

BEGIN_DATADESC( C_ZMBaseZombie )
END_DATADESC()


// ZMRTODO: Replace these
#define MAT_HPCIRCLE        "effects/zm_healthring"
#define MAT_INNERCIRCLE     "effects/zombie_select"


CLIENTEFFECT_REGISTER_BEGIN( PrecacheZombieEffect )
//CLIENTEFFECT_MATERIAL( "effects/spark" )
//CLIENTEFFECT_MATERIAL( "effects/gunshiptracer" )
CLIENTEFFECT_MATERIAL( MAT_HPCIRCLE )
CLIENTEFFECT_MATERIAL( MAT_INNERCIRCLE )
CLIENTEFFECT_REGISTER_END()


void CZMEasyQuad::Draw()
{
    float color[4];

    color[0] = m_QuadData.m_Color[0];
    color[1] = m_QuadData.m_Color[1];
    color[2] = m_QuadData.m_Color[2];
    color[3] = m_QuadData.m_flStartAlpha;

    Vector pos;


    CMatRenderContextPtr pRenderContext( materials );
    //true, NULL, NULL, m_QuadData.m_pMaterial
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true, nullptr, nullptr, m_QuadData.m_pMaterial );
	CMeshBuilder meshBuilder;

    meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

    pos = m_QuadData.m_vecOrigin - Vector( -16.0f, -16.0f, 0.0f );

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_QuadData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0.0f, 1.0f, 1.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();


    pos = m_QuadData.m_vecOrigin - Vector( -16, 16, 0 );

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_QuadData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0.0f, 0.0f, 1.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();


    pos = m_QuadData.m_vecOrigin - Vector( 16, 16, 0 );

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_QuadData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0.0f, 0.0f, 0.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();


    pos = m_QuadData.m_vecOrigin - Vector( 16, -16, 0 );

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_QuadData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0.0f, 1.0f, 0.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();


	meshBuilder.End();
	pMesh->Draw();
}

C_ZMBaseZombie::C_ZMBaseZombie()
{
    g_pZombies->AddToTail( this );


    m_fxHealth = nullptr;
    m_fxInner = nullptr;
    

    // Always create FX.
    CreateHealthFX();
    CreateInnerFX();
}

C_ZMBaseZombie::~C_ZMBaseZombie()
{
    g_pZombies->FindAndRemove( this );

    RemoveFXs();
}

void C_ZMBaseZombie::Spawn( void )
{
    BaseClass::Spawn();

    // This allows the client to make us bleed and spawn blood decals.
    // Possibly add option to turn off?
    m_takedamage = DAMAGE_YES;
}

void C_ZMBaseZombie::CreateHealthFX()
{
    FXQuadData_t data;

    data.SetAlpha( 1.0f, 1.0f );
    data.SetScale( 4 * 10.0f, 0 );
    data.SetMaterial( MAT_HPCIRCLE );
    data.SetNormal( Vector(0,0,1) );
    data.SetOrigin( vec3_origin );
    data.SetColor( 0.0f, 1.0f, 0.0f );
    data.SetScaleBias( 0 );
    data.SetAlphaBias( 0 );
    data.SetYaw( 0, 0 );

    m_fxHealth = new CZMEasyQuad( &data );
}

void C_ZMBaseZombie::RemoveFXs()
{
    delete m_fxHealth;
    delete m_fxInner;
}

void C_ZMBaseZombie::CreateInnerFX()
{
    FXQuadData_t data;

    data.SetAlpha( 1.0, 0 );
    data.SetScale( 4 * 10.0f, 0 );
    data.SetMaterial( MAT_INNERCIRCLE );
    data.SetNormal( Vector(0,0,1) );
    data.SetOrigin( vec3_origin );
    data.SetColor( 1.0f, 1.0f, 1.0f );
    data.SetScaleBias( 0 );
    data.SetAlphaBias( 0 );
    data.SetYaw( 0, 0 );

    m_fxInner = new CZMEasyQuad( &data );
}

int C_ZMBaseZombie::DrawModel( int flags )
{
    CZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );

    if ( !pPlayer || pPlayer->IsHuman() )
        return BaseClass::DrawModel( flags );


    float alpha = (m_iSelectorIndex > 0 && m_iSelectorIndex == GetLocalPlayerIndex()) ? 0.8f : 0.1f;

    if ( m_fxInner )
    {
        m_fxInner->SetAlpha( alpha );
        m_fxInner->SetPos( GetAbsOrigin() + Vector( 0.0f, 0.0f, 3.0f ) );
        m_fxInner->Draw();
    }


    if ( m_fxHealth )
    {
        /*float hp = GetHealth() > 0 ? (float)GetHealth() : 1.0f;
        float maxhp = GetMaxHealth() > 0 ? (float)GetMaxHealth() : 1.0f;

        float g = maxhp / hp;
        float r = 1.0f - g;

        m_fxHealth->SetColor( r, g, 0 );
        */

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