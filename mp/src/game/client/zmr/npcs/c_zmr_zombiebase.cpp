#include "cbase.h"

#include "clienteffectprecachesystem.h"



#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_global_shared.h"
#include "zmr/c_zmr_zmvision.h"
#include "zmr/npcs/zmr_zombiebase_shared.h"


extern bool g_bRenderPostProcess;


static ConVar zm_cl_zombiefadein( "zm_cl_zombiefadein", "0.55", FCVAR_ARCHIVE, "How fast zombie fades.", true, 0.0f, true, 2.0f );


#undef CZMBaseZombie
IMPLEMENT_CLIENTCLASS_DT( C_ZMBaseZombie, DT_ZM_BaseZombie, CZMBaseZombie )
    // See server -> zmr_zombiebase.cpp
    RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),

    RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[1], m_angRotation[1] ) ),

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
    g_ZombieManager.AddZombie( this );


    m_fxHealth = nullptr;
    m_fxInner = nullptr;

    m_iGroup = INVALID_GROUP_INDEX;

    m_pHat = nullptr;
    

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
    g_ZombieManager.RemoveZombie( this );

    delete m_fxHealth;
    delete m_fxInner;

    ReleaseHat();

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

const QAngle& C_ZMBaseZombie::EyeAngles()
{
    if ( m_iEyeAttachment > 0 )
    {
        Vector origin;
        GetAttachment( m_iEyeAttachment, origin, m_angEyeAttachment );

        return m_angEyeAttachment;
    }

    return BaseClass::EyeAngles();
}

Vector C_ZMBaseZombie::EyePosition()
{
    if ( m_iEyeAttachment > 0 )
    {
        Vector origin;
        GetAttachment( m_iEyeAttachment, origin );

        return origin;
    }

    return BaseClass::EyePosition();
}

int C_ZMBaseZombie::DrawModel( int flags )
{
    CZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );

    if (pPlayer
    &&  pPlayer->IsObserver()
    &&  pPlayer->GetObserverTarget() == this
    &&  pPlayer->GetObserverMode() == OBS_MODE_IN_EYE)
        return 0;

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


    // Turn off lighting if we're using zm vision.
    const bool bNoLight = g_ZMVision.IsOn();
    if ( bNoLight )
    {
        const Vector clr( 1.0f, 0.0f, 0.0f );

        static const Vector lightlvl[6] = 
        {
            Vector( 0.7f, 0.7f, 0.7f ),
            Vector( 0.7f, 0.7f, 0.7f ),
            Vector( 0.7f, 0.7f, 0.7f ),
            Vector( 0.7f, 0.7f, 0.7f ),
            Vector( 0.7f, 0.7f, 0.7f ),
            Vector( 0.7f, 0.7f, 0.7f ),
        };

        g_pStudioRender->SetAmbientLightColors( lightlvl );
        g_pStudioRender->SetLocalLights( 0, NULL );

        render->SetColorModulation( clr.Base() );
        modelrender->SuppressEngineLighting( true );
    }


    const Vector reset( 1.0f, 1.0f, 1.0f );


    int ret = 0;


    float fadein = zm_cl_zombiefadein.GetFloat();
    float delta = gpGlobals->curtime - SpawnTime();

    if ( fadein > EQUAL_EPSILON && delta < fadein )
    {
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


        ret = BaseClass::DrawModel( flags );


        pRenderContext->PopCustomClipPlane();
        pRenderContext->EnableClipping( false );

        
        render->SetBlend( 1.0f );
        render->SetColorModulation( reset.Base() );
    }
    else
    {
        ret = BaseClass::DrawModel( flags );
    }

    if ( bNoLight )
    {
        modelrender->SuppressEngineLighting( false );
        render->SetColorModulation( reset.Base() );
    }


    return ret;
}

/*void C_ZMBaseZombie::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
    BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}*/

extern ConVar zm_sv_happyzombies;
ConVar zm_cl_happyzombies_disable( "zm_cl_happyzombies_disable", "0", 0, "No fun :(" );
ConVar zm_cl_happyzombies_chance( "zm_cl_happyzombies_chance", "0.2", FCVAR_ARCHIVE );


CStudioHdr* C_ZMBaseZombie::OnNewModel()
{
    CStudioHdr* hdr = BaseClass::OnNewModel();
    

    HappyZombieEvent_t iEvent = (HappyZombieEvent_t)zm_sv_happyzombies.GetInt();

    if (iEvent > HZEVENT_INVALID
    &&  !zm_cl_happyzombies_disable.GetBool()
    &&  IsAffectedByEvent( iEvent )
    &&  random->RandomFloat( 0.0f, 1.0f ) <= zm_cl_happyzombies_chance.GetFloat())
    {
        CreateEventAccessories();
    }

    return hdr;
}

C_BaseAnimating* C_ZMBaseZombie::BecomeRagdollOnClient()
{
    C_BaseAnimating* pRagdoll = BaseClass::BecomeRagdollOnClient();

    ReleaseHat();

    return pRagdoll;
}

void C_ZMBaseZombie::UpdateVisibility()
{
    BaseClass::UpdateVisibility();

    // Stay parented, silly.
    if ( m_pHat )
    {
        //m_pHat->UpdateVisibility();

        if ( !IsDormant() )
        {
            m_pHat->AttachToEntity( this );
        }
    }
}

bool C_ZMBaseZombie::CreateEventAccessories()
{
    const char* model = GetEventHatModel( (HappyZombieEvent_t)zm_sv_happyzombies.GetInt() );
    if ( !model || !(*model) )
        return false;


    ReleaseHat();

    m_pHat = new C_ZMHolidayHat();


    if ( !m_pHat || !m_pHat->Initialize( this, model )/* || !m_pHat->Parent( "eyes" )*/ )
    {
        ReleaseHat();
        return false;
    }

    return true;
}

void C_ZMBaseZombie::ReleaseHat()
{
    if ( m_pHat )
    {
        m_pHat->Release();
        m_pHat = nullptr;
    }
}
