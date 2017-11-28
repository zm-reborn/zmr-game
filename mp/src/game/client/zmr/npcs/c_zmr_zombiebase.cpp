#include "cbase.h"

#include "clienteffectprecachesystem.h"



#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_global_shared.h"
#include "zmr/c_zmr_zmvision.h"
#include "c_zmr_zombiebase.h"


extern bool g_bRenderPostProcess;


static ConVar zm_cl_zombiefadein( "zm_cl_zombiefadein", "0.55", FCVAR_ARCHIVE, "How fast zombie fades.", true, 0.0f, true, 2.0f );


IMPLEMENT_CLIENTCLASS_DT( C_ZMBaseZombie, DT_ZM_BaseZombie, CZMBaseZombie )
	RecvPropInt( RECVINFO( m_iSelectorIndex ) ),
	RecvPropFloat( RECVINFO( m_flHealthRatio ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_ZMBaseZombie )
    DEFINE_PRED_FIELD( m_iSelectorIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

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

    m_iGroup = INVALID_GROUP_INDEX;
    

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

    g_ZMVision.RemoveSilhouette( this );
}

void C_ZMBaseZombie::Spawn( void )
{
    BaseClass::Spawn();

    // This allows the client to make us bleed and spawn blood decals.
    // Possibly add option to turn off?
    m_takedamage = DAMAGE_YES;


    g_ZMVision.AddSilhouette( this );
}

int C_ZMBaseZombie::DrawModel( int flags )
{
    CZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );

    if ( !pPlayer || !pPlayer->IsZM() )
    {
        return DrawModelAndEffects( flags );
    }
        

    if ( !g_bRenderPostProcess )
    {
        float ratio = m_flHealthRatio > 1.0f ? 1.0f : m_flHealthRatio;
        if ( ratio < 0.0f ) ratio = 0.0f;

        float g = ratio;
        float r = 1.0f - g;

        bool bSelected = m_iSelectorIndex > 0 && m_iSelectorIndex == GetLocalPlayerIndex();

        if ( m_fxInner )
        {
            m_fxInner->SetColor( r, g, 0 );
            m_fxInner->SetAlpha( bSelected ? 0.8f : 0.01f ); // Decrease alpha a bit.
            m_fxInner->SetPos( GetAbsOrigin() + Vector( 0.0f, 0.0f, 3.0f ) );
            m_fxInner->Draw();
        }


        if ( m_fxHealth )
        {
            m_fxHealth->SetColor( r, g, 0 );
            m_fxHealth->SetAlpha( bSelected ? 0.8f : 0.1f );
            m_fxHealth->SetPos( GetAbsOrigin() + Vector( 0.0f, 0.0f, 3.0f ) );
            m_fxHealth->Draw();
        }
    }


    return DrawModelAndEffects( flags );
}

int C_ZMBaseZombie::DrawModelAndEffects( int flags )
{
    if ( g_bRenderPostProcess )
        return BaseClass::DrawModel( flags );

    if ( !m_bReadyToDraw )
        return BaseClass::DrawModel( flags );


    float fadein = zm_cl_zombiefadein.GetFloat();

    if ( fadein <= EQUAL_EPSILON )
        return BaseClass::DrawModel( flags );



    float delta = gpGlobals->curtime - SpawnTime();

    if ( delta > fadein )
        return BaseClass::DrawModel( flags );

    delta /= fadein;

    const float l = delta * delta;
    const Vector clr( 1.0f, l, l );


    CMatRenderContextPtr pRenderContext( materials );
    
    // Pop into existence.
    const Vector down( 0.0f, 0.0f, -1.0f );

    Vector mins, maxs;
    GetRenderBounds( mins, maxs );

    Vector pos = GetAbsOrigin();
    pos += maxs.z * delta;

    const Vector4D plane( down.x, down.y, down.z, down.Dot( pos ) );
    pRenderContext->EnableClipping( true );
    pRenderContext->PushCustomClipPlane( plane.Base() );
    
    // Color it a bit.
    float blend = delta;

    // Don't go above our fx blend. Some zombies may have custom fx.
    float fxblend = GetFxBlend() / 255.0f;

    if ( blend > fxblend )
        blend = fxblend;

    render->SetBlend( blend );
    render->SetColorModulation( clr.Base() );


    int ret = BaseClass::DrawModel( flags );


    pRenderContext->PopCustomClipPlane();
    pRenderContext->EnableClipping( false );

    const Vector reset( 1.0f, 1.0f, 1.0f );
    render->SetBlend( 1.0f );
    render->SetColorModulation( reset.Base() );

    return ret;
}

/*void C_ZMBaseZombie::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
    BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}*/
