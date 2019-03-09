#include "cbase.h"
#include "bone_setup.h"
#include "eventlist.h"
#include "vprof.h"
#include "takedamageinfo.h"
#include <engine/ivdebugoverlay.h>

#include "clienteffectprecachesystem.h"



#include "zmr/zmr_player_shared.h"
#include "zmr/c_zmr_zmvision.h"
#include "zmr/npcs/zmr_zombieanimstate.h"
#include "zmr/npcs/zmr_zombiebase_shared.h"
#include "zmr/zmr_usercmd.h"


extern bool g_bRenderPostProcess;


static ConVar zm_cl_zombiefadein( "zm_cl_zombiefadein", "0.55", FCVAR_ARCHIVE, "How fast zombie fades.", true, 0.0f, true, 2.0f );


#undef CZMBaseZombie
IMPLEMENT_CLIENTCLASS_DT( C_ZMBaseZombie, DT_ZM_BaseZombie, CZMBaseZombie )
    // See server -> zmr_zombiebase.cpp
    RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),

    RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[1], m_angRotation[1] ) ),

	RecvPropInt( RECVINFO( m_iSelectorIndex ) ),
	RecvPropFloat( RECVINFO( m_flHealthRatio ) ),
    RecvPropBool( RECVINFO( m_bIsOnGround ) ),
    RecvPropInt( RECVINFO( m_iAnimationRandomSeed ) ),
    RecvPropInt( RECVINFO( m_lifeState ) ),
    RecvPropInt( RECVINFO( m_iPlayerControllerIndex ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_ZMBaseZombie )
    DEFINE_PRED_FIELD( m_iSelectorIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),


    // Animation
    DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    
    DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    //DEFINE_PRED_ARRAY( m_flPoseParameter, FIELD_FLOAT, MAXSTUDIOPOSEPARAM, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
    DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),


    // Misc
    DEFINE_PRED_ARRAY( m_flexWeight, FIELD_FLOAT, MAXSTUDIOFLEXCTRL, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_blinktoggle, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_viewtarget, FIELD_VECTOR, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
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
    m_pAnimState = new CZMZombieAnimState( this );


    g_ZombieManager.AddZombie( this );


    m_fxHealth = nullptr;
    m_fxInner = nullptr;

    m_iGroup = INVALID_GROUP_INDEX;
    m_flLastLocalSelect = 0.0f;

    m_pHat = nullptr;

    m_iAdditionalAnimRandomSeed = 0;
    

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
    delete m_pAnimState;
    

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
        

    if ( !g_bRenderPostProcess && m_iPlayerControllerIndex == 0 )
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

extern ConVar zm_sv_debug_zombieik;

void C_ZMBaseZombie::CalculateIKLocks( float currentTime )
{
    int targetCount = m_pIk->m_target.Count();
    if ( !targetCount )
        return;

    // In TF, we might be attaching a player's view to a walking model that's using IK. If we are, it can
    // get in here during the view setup code, and it's not normally supposed to be able to access the spatial
    // partition that early in the rendering loop. So we allow access right here for that special case.
    SpatialPartitionListMask_t curSuppressed = partition->GetSuppressedLists();
    partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );
    CBaseEntity::PushEnableAbsRecomputations( false );

    Ray_t ray;
    CTraceFilterNoNPCsOrPlayer traceFilter( this, GetCollisionGroup() );

    // FIXME: trace based on gravity or trace based on angles?
    Vector up;
    AngleVectors( GetRenderAngles(), nullptr, nullptr, &up );

    // FIXME: check number of slots?
    //float minHeight = FLT_MAX;
    //float maxHeight = -FLT_MAX;

    for ( int i = 0; i < targetCount; i++ )
    {
        CIKTarget* pTarget = &m_pIk->m_target[i];
        if ( !pTarget->IsActive() )
            continue;


        trace_t tr;

        switch ( pTarget->type )
        {
        case IK_GROUND :
            {
                Vector estGround;
                Vector p1, p2;

                // Adjust ground to original ground position
                estGround = pTarget->est.pos - GetRenderOrigin();
                estGround = estGround - (estGround * up) * up;
                estGround = GetAbsOrigin() + estGround + pTarget->est.floor * up;

                VectorMA( estGround, pTarget->est.height, up, p1 );
                VectorMA( estGround, -pTarget->est.height, up, p2 );

                float r = MAX( pTarget->est.radius, 1 );

                // Don't IK to other characters
                ray.Init( p1, p2, Vector(-r,-r,0), Vector(r,r,r*2) );
                enginetrace->TraceRay( ray, PhysicsSolidMaskForEntity(), &traceFilter, &tr );

                if ( zm_sv_debug_zombieik.GetBool() )
                {
                    bool bDidHit = tr.fraction != 1.0f;
                    debugoverlay->AddLineOverlay( ray.m_Start, ray.m_Start + ray.m_Delta, bDidHit ? 255 : 0, (!bDidHit) ? 255 : 0, 0, true, 0.0f );
                }

                if ( tr.m_pEnt && tr.m_pEnt->GetMoveType() == MOVETYPE_PUSH )
                {
                    pTarget->SetOwner( tr.m_pEnt->entindex(), tr.m_pEnt->GetAbsOrigin(), tr.m_pEnt->GetAbsAngles() );
                }
                else
                {
                    pTarget->ClearOwner();
                }

                if ( tr.startsolid )
                {
                    // Trace from back towards hip
                    Vector tmp = estGround - pTarget->trace.closest;
                    tmp.NormalizeInPlace();
                    ray.Init( estGround - tmp * pTarget->est.height, estGround, Vector(-r,-r,0), Vector(r,r,1) );
                    

                    enginetrace->TraceRay( ray, MASK_SOLID, &traceFilter, &tr );

                    if ( !tr.startsolid )
                    {
                        p1 = tr.endpos;
                        VectorMA( p1, - pTarget->est.height, up, p2 );
                        ray.Init( p1, p2, Vector(-r,-r,0), Vector(r,r,1) );

                        enginetrace->TraceRay( ray, MASK_SOLID, &traceFilter, &tr );
                    }
                }

                // Didn't work either, just stop here.
                if ( tr.startsolid )
                {
                    if ( !tr.DidHitWorld() )
                    {
                        pTarget->IKFailed();
                    }
                    else
                    {
                        pTarget->SetPos( tr.endpos );
                        pTarget->SetAngles( GetRenderAngles() );
                        pTarget->SetOnWorld( true );
                    }

                    continue;
                }


                if ( tr.DidHitWorld() )
                {
                    // clamp normal to 33 degrees
                    const float limit = 0.832f;
                    float dot = DotProduct( tr.plane.normal, up );
                    if ( dot < limit )
                    {
                        Assert( dot >= 0 );
                        // subtract out up component
                        Vector diff = tr.plane.normal - up * dot;
                        // scale remainder such that it and the up vector are a unit vector
                        float d = sqrt( (1 - limit * limit) / DotProduct( diff, diff ) );
                        tr.plane.normal = up * limit + d * diff;
                    }
                    // FIXME: this is wrong with respect to contact position and actual ankle offset
                    pTarget->SetPosWithNormalOffset( tr.endpos, tr.plane.normal );
                    pTarget->SetNormal( tr.plane.normal );
                    pTarget->SetOnWorld( true );

                    // only do this on forward tracking or commited IK ground rules
                    //if (pTarget->est.release < 0.1)
                    //{
                    //    // keep track of ground height
                    //    float offset = DotProduct( pTarget->est.pos, up );
                    //    if (minHeight > offset )
                    //        minHeight = offset;

                    //    if (maxHeight < offset )
                    //        maxHeight = offset;
                    //}
                    // FIXME: if we don't drop legs, running down hills looks horrible
                    /*
                    if (DotProduct( pTarget->est.pos, up ) < DotProduct( estGround, up ))
                    {
                        pTarget->est.pos = estGround;
                    }
                    */
                }
                else if ( tr.DidHitNonWorldEntity() )
                {
                    pTarget->SetPos( tr.endpos );
                    pTarget->SetAngles( GetRenderAngles() );

                    // only do this on forward tracking or commited IK ground rules
                    //if (pTarget->est.release < 0.1)
                    //{
                    //    float offset = DotProduct( pTarget->est.pos, up );
                    //    if (minHeight > offset )
                    //        minHeight = offset;

                    //    if (maxHeight < offset )
                    //        maxHeight = offset;
                    //}
                    // FIXME: if we don't drop legs, running down hills looks horrible
                    /*
                    if (DotProduct( pTarget->est.pos, up ) < DotProduct( estGround, up ))
                    {
                        pTarget->est.pos = estGround;
                    }
                    */
                }
                else
                {
                    pTarget->IKFailed();
                }
            }
            break;
        }
    }

    CBaseEntity::PopEnableAbsRecomputations();
    partition->SuppressLists( curSuppressed, true );
}

void C_ZMBaseZombie::OnDataChanged( DataUpdateType_t type )
{
    BaseClass::OnDataChanged( type );

    if ( type == DATA_UPDATE_CREATED )
    {
        SetNextClientThink( CLIENT_THINK_ALWAYS );
    }

    UpdateVisibility();
}

void C_ZMBaseZombie::UpdateClientSideAnimation()
{
    m_pAnimState->Update();

    BaseClass::UpdateClientSideAnimation();
}

void C_ZMBaseZombie::HandleAnimEvent( animevent_t* pEvent )
{
    if ( pEvent->event == AE_ZOMBIE_STEP_LEFT )
    {
        if ( ShouldPlayFootstepSound() )
            FootstepSound( false );

        return;
    }

    if ( pEvent->event == AE_ZOMBIE_STEP_RIGHT )
    {
        if ( ShouldPlayFootstepSound() )
            FootstepSound( true );

        return;
    }


    if ( pEvent->event == AE_ZOMBIE_SCUFF_LEFT )
    {
        if ( ShouldPlayFootstepSound() )
            FootscuffSound( false );

        return;
    }

    if ( pEvent->event == AE_ZOMBIE_SCUFF_RIGHT )
    {
        if ( ShouldPlayFootstepSound() )
            FootscuffSound( true );

        return;
    }

    if ( pEvent->event == AE_ZOMBIE_STARTSWAT )
    {
        AttackSound();
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_ATTACK_SCREAM )
    {
        AttackSound();
        return;
    }

    BaseClass::HandleAnimEvent( pEvent );
}

void C_ZMBaseZombie::TraceAttack( const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr, CDmgAccumulator* pAccumulator )
{
    BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );

    if ( m_takedamage == DAMAGE_YES && g_ZMUserCmdSystem.UsesClientsideDetection( this ) )
    {
        ZMUserCmdHitData_t hit;
        hit.entindex = entindex();
        hit.nHits = 1;
        hit.hitgroups[0] = ptr->hitgroup;

        g_ZMUserCmdSystem.AddDamage( hit );
    }
}

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

bool C_ZMBaseZombie::ShouldPlayFootstepSound() const
{
    if ( !m_bIsOnGround )
        return false;

    // If our idle animation is more prominent, don't play any footsteps.
    return m_AnimOverlay.Count() <= ANIMOVERLAY_SLOT_IDLE || m_AnimOverlay[ANIMOVERLAY_SLOT_IDLE].m_flWeight < 0.8f;
}

void C_ZMBaseZombie::PlayFootstepSound( const char* soundname )
{
    VPROF_BUDGET( "C_ZMBaseZombie::PlayFootstepSound", _T( "CBaseEntity::EmitSound" ) );


    // Depending on how "active" our walking animation is, change the footstep volume.
    // I can't seem to notice a difference, but whatever. Perhaps the soundscript affects this in some way?
    float volume = VOL_NORM;

    if ( m_AnimOverlay.Count() > ANIMOVERLAY_SLOT_IDLE )
    {
        float value = 1.0f - m_AnimOverlay[ANIMOVERLAY_SLOT_IDLE].m_flWeight;
        value *= value;

        volume = clamp( value, 0.05f, 1.0f );
    }


    CLocalPlayerFilter filter;

    EmitSound_t params;
    params.m_pSoundName = soundname;
    params.m_pflSoundDuration = nullptr;
    params.m_bWarnOnDirectWaveReference = true;
    params.m_flVolume = volume;

    EmitSound( filter, entindex(), params );
}
