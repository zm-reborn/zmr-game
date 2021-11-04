#include "cbase.h"

#include "baseviewport.h"
#include "clienteffectprecachesystem.h"
#include "dlight.h"
#include "r_efx.h"
#include "takedamageinfo.h"
#include "view.h"
#include "bone_setup.h"
#include "in_buttons.h"
#include "debugoverlay_shared.h"


#include "c_zmr_util.h"
#include "ui/zmr_zmview_base.h"

#include "npcs/c_zmr_zombiebase.h"
#include "zmr_viewmodel.h"
#include "c_zmr_entities.h"
#include "zmr_softcollisions.h"
#include "c_zmr_player_ragdoll.h"
#include "c_zmr_teamkeys.h"
#include "c_zmr_colorcorrection.h"

#include "c_zmr_flashlighteffect.h"
#include "zmr_resource_system.h"
#include "c_zmr_player.h"
#include "c_zmr_zmvision.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern bool g_bRenderPostProcess;


// ZMRTODO: Replace these
#define MAT_HPCIRCLE        "effects/zm_healthring"
#define MAT_INNERFLARE      "effects/yellowflare"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectZMPlayerEffect )
CLIENTEFFECT_MATERIAL( MAT_HPCIRCLE )
CLIENTEFFECT_MATERIAL( MAT_INNERFLARE )
CLIENTEFFECT_REGISTER_END()




ConVar cl_playermodel( "cl_playermodel", "none", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Your player model" );

ConVar zm_cl_participation( "zm_cl_participation", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Your participation setting. 0 = Want to be ZM, 1 = Only human, 2 = Only spectator" );

// General ZM moving
ConVar zm_cl_zmmovespeed( "zm_cl_zmmovespeed", "1600", FCVAR_USERINFO | FCVAR_ARCHIVE, "How fast you'll move." );
ConVar zm_cl_zmmoveaccelerate( "zm_cl_zmmoveaccelerate", "5", FCVAR_USERINFO | FCVAR_ARCHIVE, "How fast you'll accelerate." );
ConVar zm_cl_zmmovedecelerate( "zm_cl_zmmovedecelerate", "4", FCVAR_USERINFO | FCVAR_ARCHIVE, "How fast you'll decelerate." );
// Mouse wheel
ConVar zm_cl_zmmovemwheelmove( "zm_cl_zmmovemwheelmove", "1", FCVAR_ARCHIVE, "As the ZM, can you move up/down with mousewheel?" );
ConVar zm_cl_zmmovemwheelmovespd( "zm_cl_zmmovemwheelmovespd", "1600", FCVAR_ARCHIVE, "", true, 400.0f, true, 2000.0f );
// Button ones
ConVar zm_cl_zmmovejumpspd( "zm_cl_zmmovejumpspd", "1600", FCVAR_ARCHIVE );
ConVar zm_cl_zmmovespdspd( "zm_cl_zmmovespdspd", "1600", FCVAR_ARCHIVE );


// Yes, unfortunately because of how FOV is used, we need to change it on the server.
ConVar zm_cl_fov( "zm_cl_fov", "90", FCVAR_USERINFO | FCVAR_ARCHIVE, "What is our default field of view when playing.", true, ZM_MIN_FOV, true, MAX_FOV );

ConVar zm_cl_zmcontrolstyle( "zm_cl_zmcontrolstyle", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "How stubborn zombies will behave. 0 = Default, 1 = ZM 1.2.1 style" );
ConVar zm_cl_zmunitcommandinterrupt( "zm_cl_zmunitcommandinterrupt", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "What commands you as the ZM can interrupt. 0 = None, 1 = Stop swat, 2 = Stop attack, 3 = Stop both", true, 0.0f, false, 0.0f );



#undef CZMPlayer // We need to undefine it so we can get the server class.

BEGIN_RECV_TABLE_NOBASE( C_ZMPlayer, DT_ZMLocalPlayerExclusive )
    RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),

    //RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_ZMPlayer, DT_ZMNonLocalPlayerExclusive )
    RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),

    RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
    RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),

    RecvPropFloat( RECVINFO( m_vecVelocity[0] ) ),
    RecvPropFloat( RECVINFO( m_vecVelocity[1] ) ),

    RecvPropInt( RECVINFO( m_nMuzzleFlashParity ) ),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_ZMPlayer, DT_ZM_Player, CZMPlayer )
    RecvPropDataTable( "zmlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_ZMLocalPlayerExclusive) ),
    RecvPropDataTable( "zmnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_ZMNonLocalPlayerExclusive) ),

    RecvPropDataTable( RECVINFO_DT( m_ZMLocal ), 0, &REFERENCE_RECV_TABLE( DT_ZM_PlyLocal ) ),

    RecvPropInt( RECVINFO( m_iSpawnInterpCounter ) ),
    RecvPropEHandle( RECVINFO( m_hRagdoll ) ),

    RecvPropInt( RECVINFO( m_nWaterLevel ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_ZMPlayer )
    DEFINE_PRED_FIELD_TOL( m_ZMLocal.m_flAccuracyRatio, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.01f ),


    DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    //DEFINE_PRED_FIELD( m_fIsWalking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
    DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
    DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),

    DEFINE_PRED_FIELD( m_blinktoggle, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_viewtarget, FIELD_VECTOR, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),

    // Added poseparameter override here. See if this has any effect on crashing, since these are not networked anymore anyway.
    DEFINE_PRED_ARRAY_TOL( m_flPoseParameter, FIELD_FLOAT, MAXSTUDIOPOSEPARAM, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK, 0.0f ),
END_PREDICTION_DATA()


CZMPlayerAnimState* CreateZMPlayerAnimState( C_ZMPlayer* pPlayer );

C_ZMPlayer::C_ZMPlayer() : m_iv_angEyeAngles( "C_ZMPlayer::m_iv_angEyeAngles" )
{
    AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );
	AddVar( &m_Local.m_vecPunchAngle, &m_Local.m_iv_vecPunchAngle, LATCH_SIMULATION_VAR );
	AddVar( &m_Local.m_vecPunchAngleVel, &m_Local.m_iv_vecPunchAngleVel, LATCH_SIMULATION_VAR );

	m_flZoomStart		= 0.0f;
	m_flZoomEnd			= 0.0f;
	m_flZoomRate		= 0.0f;
	m_flZoomStartTime	= 0.0f;


    //m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;

    m_pFlashlight = nullptr;
    m_iIDEntIndex = 0;
    m_iSpawnInterpCounterCache = 0;


    m_pPlayerAnimState = CreateZMPlayerAnimState( this );



    m_flNextUpMove = 0.0f;
    m_flUpMove = 0.0f;

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
    ReleaseFlashlight();

    m_pPlayerAnimState->Release();


    delete m_fxHealth;
    delete m_fxInner;

    g_ZMVision.RemoveSilhouette( this );
}

void C_ZMPlayer::Spawn()
{
    BaseClass::Spawn();
}

C_ZMPlayer* C_ZMPlayer::GetLocalPlayer()
{
    return static_cast<C_ZMPlayer*>( C_BasePlayer::GetLocalPlayer() );
}

void C_ZMPlayer::ClientThink()
{
    UpdateIDTarget();
}

void C_ZMPlayer::PreThink()
{
    // Put here so we get predicted properly.
    if ( IsLocalPlayer() )
    {
        /*
        // Simulate our spectator target.
        if ( !IsAlive() )
        {
            C_ZMPlayer* pObserved = ( GetObserverMode() == OBS_MODE_IN_EYE ) ? ToZMPlayer( GetObserverTarget() ) : nullptr;

            if ( pObserved )
                pObserved->UpdateAccuracyRatio();
        }
        else
        {
            UpdateAccuracyRatio();
        }
        */

        if ( IsAlive() )
        {
            UpdateAccuracyRatio();
        }

        //if ( IsZM() )
        //{
        //    g_ZMResourceSystem.GainResources( this );
        //}
    }


    // Don't let base class change our max speed (sprinting)
    float spd = MaxSpeed();

    BaseClass::PreThink();

    if ( MaxSpeed() != spd )
        SetMaxSpeed( spd );

    //HandleSpeedChanges();

    //if ( m_HL2Local.m_flSuitPower <= 0.0f )
    //{
    //    if( IsSprinting() )
    //    {
    //        StopSprinting();
    //    }
    //}
}

void C_ZMPlayer::UpdateClientSideAnimation()
{
    m_pPlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );

    BaseClass::UpdateClientSideAnimation();
}

void C_ZMPlayer::CalculateIKLocks( float currentTime )
{
    if ( !m_pIk ) 
        return;

    int targetCount = m_pIk->m_target.Count();
    if ( targetCount == 0 )
        return;

    // In TF, we might be attaching a player's view to a walking model that's using IK. If we are, it can
    // get in here during the view setup code, and it's not normally supposed to be able to access the spatial
    // partition that early in the rendering loop. So we allow access right here for that special case.
    SpatialPartitionListMask_t curSuppressed = partition->GetSuppressedLists();
    partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );
    C_BaseEntity::PushEnableAbsRecomputations( false );

    for ( int i = 0; i < targetCount; i++ )
    {
        trace_t trace;
        CIKTarget *pTarget = &m_pIk->m_target[i];

        if ( !pTarget->IsActive() )
            continue;

        switch( pTarget->type )
        {
        case IK_GROUND:
            {
                pTarget->SetPos( Vector( pTarget->est.pos.x, pTarget->est.pos.y, GetRenderOrigin().z ));
                pTarget->SetAngles( GetRenderAngles() );
            }
            break;

        case IK_ATTACHMENT:
            {
                C_BaseEntity *pEntity = NULL;
                float flDist = pTarget->est.radius;

                // FIXME: make entity finding sticky!
                // FIXME: what should the radius check be?
                for ( CEntitySphereQuery sphere( pTarget->est.pos, 64 ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
                {
                    C_BaseAnimating* pAnim = pEntity->GetBaseAnimating();
                    if ( !pAnim )
                        continue;

                    int iAttachment = pAnim->LookupAttachment( pTarget->offset.pAttachmentName );
                    if ( iAttachment <= 0 )
                        continue;

                    Vector origin;
                    QAngle angles;
                    pAnim->GetAttachment( iAttachment, origin, angles );

                    // debugoverlay->AddBoxOverlay( origin, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), QAngle( 0, 0, 0 ), 255, 0, 0, 0, 0 );

                    float d = (pTarget->est.pos - origin).Length();

                    if ( d >= flDist )
                        continue;

                    flDist = d;
                    pTarget->SetPos( origin );
                    pTarget->SetAngles( angles );
                    // debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 255, 0, 0, 0 );
                }

                if ( flDist >= pTarget->est.radius )
                {
                    // debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 0, 255, 0, 0 );
                    // no solution, disable ik rule
                    pTarget->IKFailed();
                }
            }
            break;
        }
    }

    C_BaseEntity::PopEnableAbsRecomputations();
    partition->SuppressLists( curSuppressed, true );
}

void C_ZMPlayer::OnSpawn()
{
    if ( IsLocalPlayer() )
    {
        // By default display hands.
        // This hack has to be here because SetWeaponVisible isn't called on client when the player spawns.
        C_ZMViewModel* pHands = static_cast<C_ZMViewModel*>( GetViewModel( VMINDEX_HANDS, false ) );
        if ( pHands )
            pHands->SetDrawVM( true );
    }
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


    // Turn off nightvision.
    g_ZMVision.TurnOff();


    // Reset back to old team just in case something uses it.
    C_BaseEntity::ChangeTeam( iOldTeam );


    if ( iNewTeam == ZMTEAM_ZM )
    {
        ZMClientUtil::QueueTooltip( "zmintro", 6.0f );
        ZMClientUtil::QueueTooltip( "zmmoving", 16.0f );
    }


    TeamChangeStatic( iNewTeam );


    m_flNextUpMove = 0.0f;
}

void C_ZMPlayer::TeamChangeStatic( int iNewTeam )
{
    // It's possible to receive events from the server before our local player is created.
    // All crucial things that don't rely on local player
    // should be put here.


    if ( g_pZMView )
        g_pZMView->SetVisible( iNewTeam == ZMTEAM_ZM );


    // Execute team config.
    CZMTeamKeysConfig::ExecuteTeamConfig( iNewTeam );




    if ( iNewTeam == ZMTEAM_ZM )
    {
        engine->ClientCmd_Unrestricted( "exec zm.cfg" );
    }
    else if ( iNewTeam == ZMTEAM_HUMAN )
    {
        engine->ClientCmd_Unrestricted( "exec survivor.cfg" );
    }


    ZMGetCCSystem()->OnTeamChange( iNewTeam );
}

void C_ZMPlayer::UpdateFlashlight()
{
    // Flashlight isn't on.
    if ( !IsFlashlightOn() )
    {
        if ( m_pFlashlight )
            m_pFlashlight->TurnOff();
        return;
    }


    // Create the flashlight.
    if ( !m_pFlashlight )
    {
        m_pFlashlight = new CZMFlashlightEffect( this );
        if ( !m_pFlashlight )
            return;
    }


    // Update the light with the new position and direction.
    m_pFlashlight->TurnOn();
    m_pFlashlight->UpdateLight();
}

void C_ZMPlayer::Simulate()
{
    if ( IsLocalPlayer() )
    {
        // Update the player's fog data if necessary.
        UpdateFogController();
    }
    else
    {
        // Update step sounds for all other players
        Vector vel;
        EstimateAbsVelocity( vel );
        UpdateStepSound( GetGroundSurface(), GetAbsOrigin(), vel );
    }

    // Update player's flashlight
    UpdateFlashlight();

    g_ZMVision.UpdateLight();


	if ( gpGlobals->frametime != 0.0f  )
	{
        if ( ShouldMuzzleFlash() )
        {
            DisableMuzzleFlash();
		
            ProcessMuzzleFlashEvent();
        }

		//DoAnimationEvents( GetModelPtr() );
	}

    C_BaseEntity::Simulate();


    //
    // When we get teleported.
    //
    if ( IsNoInterpolationFrame() || Teleported() )
    {
        // Tell VM to release history.
        auto* pVM = GetViewModel();
        if ( pVM )
        {
            pVM->OnTeleported();
        }

        ResetLatched();
    }
}

bool C_ZMPlayer::ShouldDraw()
{
    // ZMRTODO: See if this has any side-effects.
    if ( IsDormant() )
        return false;

    if ( IsZM() )
        return false;

    if ( !IsAlive() )
        return false;

    if ( IsLocalPlayer() && IsRagdoll() )
        return true;

    if ( IsRagdoll() )
        return false;

    return BaseClass::ShouldDraw();
}

int C_ZMPlayer::DrawModel( int flags )
{
    if ( !m_bReadyToDraw ) return 0;

    // Again, ShouldDraw is not enough.
    if ( IsZM() ) return 0;


    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    if ( !pPlayer || !pPlayer->IsZM() )
    {
        return BaseClass::DrawModel( flags );
    }


    if ( !g_bRenderPostProcess )
    {
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
    }


    return DrawModelAndEffects( flags );
}

int C_ZMPlayer::DrawModelAndEffects( int flags )
{
    // Turn off lighting if we're using zm vision.
    const bool bNoLight = !g_bRenderPostProcess && g_ZMVision.IsOn();
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


    int ret = 0;

    const Vector reset( 1.0f, 1.0f, 1.0f );


    ret = BaseClass::DrawModel( flags );


    if ( bNoLight )
    {
        modelrender->SuppressEngineLighting( false );
        render->SetColorModulation( reset.Base() );
    }

    return ret;
}

ShadowType_t C_ZMPlayer::ShadowCastType() 
{
    if ( !IsVisible() )
         return SHADOWS_NONE;

    return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

bool C_ZMPlayer::ShouldReceiveProjectedTextures( int flags )
{
    Assert( flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK );

    if ( IsEffectActive( EF_NODRAW ) )
         return false;

    if( flags & SHADOW_FLAGS_FLASHLIGHT )
    {
        return true;
    }

    return BaseClass::ShouldReceiveProjectedTextures( flags );
}

const QAngle& C_ZMPlayer::GetRenderAngles()
{
    if ( IsRagdoll() )
    {
        return vec3_angle;
    }
    else
    {
        return m_pPlayerAnimState->GetRenderAngles();
    }
}

const QAngle& C_ZMPlayer::EyeAngles()
{
    return IsLocalPlayer() ? BaseClass::EyeAngles() : m_angEyeAngles;
}

void C_ZMPlayer::SetLocalAngles( const QAngle& angles )
{
    m_angEyeAngles = angles;
    BaseClass::SetLocalAngles( angles );
}

float C_ZMPlayer::GetFOV()
{
    float fov = C_BasePlayer::GetFOV() + GetZoom();
    
    return MAX( GetMinFOV(), fov );
}

void C_ZMPlayer::Zoom( float fov, float flTime )
{
	m_flZoomStart		= GetZoom();
	m_flZoomEnd			= fov;
	m_flZoomRate		= flTime;
	m_flZoomStartTime	= gpGlobals->curtime;
}

float C_ZMPlayer::GetZoom()
{
	float fFOV = m_flZoomEnd;

	// See if we need to lerp the values
	if ( ( m_flZoomStart != m_flZoomEnd ) && ( m_flZoomRate > 0.0f ) )
	{
		float deltaTime = ( gpGlobals->curtime - m_flZoomStartTime ) / m_flZoomRate;

		if ( deltaTime >= 1.0f )
		{
			// If we're past the zoom time, just take the new value and stop lerping
			fFOV = m_flZoomStart = m_flZoomEnd;
		}
		else
		{
			fFOV = SimpleSplineRemapVal( deltaTime, 0.0f, 1.0f, m_flZoomStart, m_flZoomEnd );
		}
	}

	return fFOV;
}

extern ConVar default_fov;

float C_ZMPlayer::GetLocalDefaultFOV()
{
    const float flLocalDefFOV = default_fov.GetFloat();

    auto* pLocal = C_ZMPlayer::GetLocalPlayer();
    if ( !pLocal )
        return flLocalDefFOV;

    return pLocal->GetDefaultFOV();
}

int C_ZMPlayer::GetIDTarget() const
{
    return m_iIDEntIndex;
}

void C_ZMPlayer::UpdateIDTarget()
{
    if ( !IsLocalPlayer() )
        return;

    // Clear old target and find a new one
    m_iIDEntIndex = 0;

    // don't show IDs in chase spec mode
    if ( GetObserverMode() == OBS_MODE_CHASE )
         return;

    C_BaseEntity* pIgnore = (IsObserver() && GetObserverTarget() && GetObserverMode() == OBS_MODE_IN_EYE) ? GetObserverTarget() : this;

    trace_t tr;
    Vector vecStart, vecEnd;
    VectorMA( MainViewOrigin(), 1500, MainViewForward(), vecEnd );
    VectorMA( MainViewOrigin(), 10, MainViewForward(), vecStart );
    UTIL_TraceLine(
        vecStart,
        vecEnd,
        MASK_SOLID,
        pIgnore,
        COLLISION_GROUP_NONE,
        &tr );

    if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
    {
        C_BaseEntity* pEntity = tr.m_pEnt;

        if ( pEntity && pEntity != pIgnore )
        {
            m_iIDEntIndex = pEntity->entindex();
        }
    }
}

void C_ZMPlayer::TraceAttack( const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr, CDmgAccumulator* pAccumulator )
{
    Vector vecOrigin = ptr->endpos - vecDir * 4;

    float flDistance = 0.0f;
    
    if ( info.GetAttacker() )
    {
        flDistance = (ptr->endpos - info.GetAttacker()->GetAbsOrigin()).Length();
    }

    if ( m_takedamage )
    {
        AddMultiDamage( info, this );

        int blood = BloodColor();
        
        C_BaseEntity* pAttacker = info.GetAttacker();

        if ( pAttacker && pAttacker->InSameTeam( this ) )
        {
            return;
        }

        if ( blood != DONT_BLEED )
        {
            UTIL_BloodDrips( vecOrigin, vecDir, blood, info.GetDamage() );// a little surface blood.
            TraceBleed( flDistance, vecDir, ptr, info.GetDamageType() );
        }
    }
}

void C_ZMPlayer::DoImpactEffect( trace_t& tr, int nDamageType )
{
    if ( GetActiveWeapon() )
    {
        GetActiveWeapon()->DoImpactEffect( tr, nDamageType );
        return;
    }

    BaseClass::DoImpactEffect( tr, nDamageType );
}

C_BaseAnimating* C_ZMPlayer::BecomeRagdollOnClient()
{
    return nullptr;
}

// ZMRTODO: When happy, set to 1.
ConVar zm_cl_firstperson_deathcam( "zm_cl_firstperson_deathcam", "0", FCVAR_ARCHIVE );

void C_ZMPlayer::CalcView( Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov )
{
    C_ZMPlayer* pPlayer = this;

    if (IsObserver()
    &&  GetObserverMode() == OBS_MODE_IN_EYE
    &&  GetObserverTarget()
    &&  GetObserverTarget()->IsPlayer() )
    {
        pPlayer = ToZMPlayer( GetObserverTarget() );
    }

    if ( pPlayer->m_lifeState != LIFE_ALIVE && !pPlayer->IsObserver() )
    {
        if ( zm_cl_firstperson_deathcam.GetBool() )
        {
            DeathCam_Firstperson( pPlayer, eyeOrigin, eyeAngles, zNear, zFar, fov );
        }
        else
        {
            DeathCam_Thirdperson( pPlayer, eyeOrigin, eyeAngles, zNear, zFar, fov );
        }

        return;
    }

    BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

void C_ZMPlayer::DeathCam_Firstperson( C_ZMPlayer* pPlayer, Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov )
{
    C_ZMRagdoll* pRagdoll = pPlayer->GetRagdoll();

    if ( !pRagdoll || pPlayer->GetEyeAttachment() == -1 )
    {
        DeathCam_Thirdperson( pPlayer, eyeOrigin, eyeAngles, zNear, zFar, fov );
        return;
    }


    pRagdoll->GetAttachment( pPlayer->GetEyeAttachment(), eyeOrigin, eyeAngles );
}

void C_ZMPlayer::DeathCam_Thirdperson( C_ZMPlayer* pPlayer, Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov )
{
    Vector origin = pPlayer->EyePosition();			

    IRagdoll* pRagdoll = pPlayer->GetRepresentativeRagdoll();

    if ( pRagdoll )
    {
        origin = pRagdoll->GetRagdollOrigin();
        origin.z += VEC_DEAD_VIEWHEIGHT_SCALED( pPlayer ).z; // look over ragdoll, not through
    }

    BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );

    eyeOrigin = origin;
        
    Vector vForward; 
    AngleVectors( eyeAngles, &vForward );

    VectorNormalize( vForward );
    VectorMA( origin, -CHASE_CAM_DISTANCE_MAX, vForward, eyeOrigin );

    Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
    Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );

    trace_t trace; // clip against world
    C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
    UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &trace );
    C_BaseEntity::PopEnableAbsRecomputations();

    if ( trace.fraction < 1.0f )
    {
        eyeOrigin = trace.endpos;
    }
}

IRagdoll* C_ZMPlayer::GetRepresentativeRagdoll() const
{
    if ( GetRagdoll() )
    {
        return GetRagdoll()->GetIRagdoll();
    }
    else
    {
        return nullptr;
    }
}

C_ZMRagdoll* C_ZMPlayer::GetRagdoll() const
{
    return m_hRagdoll.Get();
}

int C_ZMPlayer::GetEyeAttachment() const
{
    return m_iAttachmentEyes;
}

CStudioHdr* C_ZMPlayer::OnNewModel( void )
{
    CStudioHdr* hdr = BaseClass::OnNewModel();
    
    Initialize();

    if ( m_pPlayerAnimState )
    {
        m_pPlayerAnimState->OnNewModel();
    }
    
    return hdr;
}

void C_ZMPlayer::Initialize()
{
    CStudioHdr *hdr = GetModelPtr();
    for ( int i = 0; i < hdr->GetNumPoseParameters() ; i++ )
    {
        SetPoseParameter( hdr, i, 0.0 );
    }


    // This is used for deathcam. Should be safe since we're using the same model as the ragdoll.
    m_iAttachmentEyes = LookupAttachment( "eyes" );
    
    if ( m_pFlashlight )
    {
        m_pFlashlight->SetRightHandAttachment( LookupAttachment( "anim_attachment_RH" ) );
    }
}

bool C_ZMPlayer::ShouldInterpolate()
{
    // Always interpolate our observer target.
    // Mainly here for ZM observering being laggy.
    // ZMRTODO: See if this has any side-effects.
    if ( !IsLocalPlayer() )
    {
        C_ZMPlayer* pPlayer = GetLocalPlayer();

        if ( pPlayer && pPlayer->GetObserverTarget() == this )
        {
            return true;
        }
    }

    return BaseClass::ShouldInterpolate();
}

void C_ZMPlayer::NotifyShouldTransmit( ShouldTransmitState_t state )
{
    if ( state == SHOULDTRANSMIT_END )
    {
        if ( m_pFlashlight )
        {
            ReleaseFlashlight();
        }
    }

    BaseClass::NotifyShouldTransmit( state );
}

void C_ZMPlayer::OnDataChanged( DataUpdateType_t type )
{
    BaseClass::OnDataChanged( type );

    if ( type == DATA_UPDATE_CREATED )
    {
        SetNextClientThink( CLIENT_THINK_ALWAYS );
    }

    UpdateVisibility();
}

void C_ZMPlayer::PostDataUpdate( DataUpdateType_t updateType )
{
    if ( m_iSpawnInterpCounter != m_iSpawnInterpCounterCache )
    {
        MoveToLastReceivedPosition( true );
        ResetLatched();
        m_iSpawnInterpCounterCache = m_iSpawnInterpCounter;
    }

    BaseClass::PostDataUpdate( updateType );

    if ( updateType == DATA_UPDATE_CREATED )
    {
        if ( IsLocalPlayer() )
        {
            g_ZMVision.Init();
        }
        else
        {
            g_ZMVision.AddSilhouette( this );
        }
    }
}

void C_ZMPlayer::ReleaseFlashlight()
{
    delete m_pFlashlight;
    m_pFlashlight = nullptr;
}



#define FLASH_NONE              0
#define FLASH_LOCAL_ONLY        1
#define FLASH_ALL_PLAYERS       2

void UTIL_ParseColorFromString( const char* str, int clr[], int nColors );

extern ConVar muzzleflash_light;

ConVar zm_cl_muzzleflash_light( "zm_cl_muzzleflash_light", "2", FCVAR_ARCHIVE, "0 = No light, 1 = Only local player, 2 = All players" );
ConVar zm_cl_muzzleflash_light_color( "zm_cl_muzzleflash_light_color", "255 240 120", FCVAR_ARCHIVE, "Color of the muzzleflash light" );

ConVar zm_cl_muzzleflash_light_size_min( "zm_cl_muzzleflash_light_size_min", "128", FCVAR_ARCHIVE, "", true, 0.0f, true, 512.0f );
ConVar zm_cl_muzzleflash_light_size_max( "zm_cl_muzzleflash_light_size_max", "256", FCVAR_ARCHIVE, "", true, 0.0f, true, 512.0f );
ConVar zm_cl_muzzleflash_light_lifetime( "zm_cl_muzzleflash_light_lifetime", "0.05", FCVAR_ARCHIVE, "", true, 0.01f, true, 0.1f );
ConVar zm_cl_muzzleflash_light_exponent( "zm_cl_muzzleflash_light_exponent", "5", FCVAR_ARCHIVE, "", true, 1.0f, true, 20.0f );

ConVar zm_cl_muzzleflash_light_debug( "zm_cl_muzzleflash_light_debug", "0", FCVAR_CHEAT );

void C_ZMPlayer::ProcessMuzzleFlashEvent()
{
    // How many units we push the light forward from player's eyes.
    const float flMuzzleflashFwd = 32.0f;


    int allowed = zm_cl_muzzleflash_light.GetInt();

    if ( allowed == FLASH_NONE )
    {
        return;
    }


    float lifetime = zm_cl_muzzleflash_light_lifetime.GetFloat();
    if ( lifetime <= 0.0f )
    {
        return;
    }


    Vector pos = vec3_origin;
    bool bHasPos = false;
    

    // Local player doesn't get rendered, so just estimate the position.
    if ( IsLocalPlayer() )
    {
        pos = EyePosition() + MainViewForward() * flMuzzleflashFwd;

        bHasPos = true;
    }
    else
    {
        if ( allowed != FLASH_ALL_PLAYERS )
        {
            return;
        }


        // ZMRTODO: Use the proper muzzle position.
        // This doesn't seem to work, because the attachment abs position
        // doesn't account the player angles, etc.
        //auto* pWep = GetActiveWeapon();
        //
        //if ( pWep )
        //{
        //    QAngle ang;

        //    if (pWep->GetAttachment( "muzzle", pos, ang )
        //    ||  pWep->GetAttachment( 1, pos, ang ))
        //    {
        //        bHasPos = true;
        //    }
        //}

        Vector fwd;
        EyeVectors( &fwd );
        
        pos = EyePosition() + fwd * flMuzzleflashFwd;
        bHasPos = true;
    }
    

    if ( !bHasPos )
    {
        pos = WorldSpaceCenter();
    }



    //
    // Perform the muzzleflash light
    //

    // Scale depending on how dark it is.
    // It looks silly if we're in daylight getting ridiculous flashes.
    Vector vecLight = engine->GetLightForPoint( pos, false );
    float flBias = 0.3f;

    float intensity = 1.0f - ((0.21f * vecLight.x) + (0.72f * vecLight.y) + (0.07f * vecLight.z) + flBias);
    intensity = clamp( intensity, 0.0f, 1.0f );


    float flRadiusMult = intensity < 0.7f ? intensity : 1.0f;
    
    float radius = random->RandomInt(   zm_cl_muzzleflash_light_size_min.GetFloat(),
                                        zm_cl_muzzleflash_light_size_max.GetFloat() ) * flRadiusMult;





    float debugtime = zm_cl_muzzleflash_light_debug.GetFloat();

    if ( debugtime > 0.0f )
    {
        NDebugOverlay::Sphere( pos, radius, 255, 0, 0, false, debugtime );
    }


    int clr[3];
    UTIL_ParseColorFromString( zm_cl_muzzleflash_light_color.GetString(), clr, ARRAYSIZE( clr ) );


    // Dynamic light
    dlight_t* el = effects->CL_AllocDlight( LIGHT_INDEX_MUZZLEFLASH + index );
    el->origin = pos;
    el->radius = radius; 
    el->decay = el->radius / lifetime;
    el->die = gpGlobals->curtime + lifetime;
    el->color.r = (byte)clr[0];
    el->color.g = (byte)clr[1];
    el->color.b = (byte)clr[2];
    el->color.exponent = (signed char)zm_cl_muzzleflash_light_exponent.GetInt();
}

bool C_ZMPlayer::CreateMove( float delta, CUserCmd* cmd )
{
    bool bResult = BaseClass::CreateMove( delta, cmd );
    

    if ( IsZM() && m_flNextUpMove > gpGlobals->curtime && zm_cl_zmmovemwheelmove.GetBool() )
    {
        cmd->upmove += m_flUpMove;
    }


    if ( IsZM() )
    {
        // Encode the real wanted movement speed to 1/100


        if ( cmd->forwardmove != 0.0f )
        {
            float value = zm_cl_zmmovespeed.GetFloat() * 0.01f;
            cmd->forwardmove = cmd->forwardmove > 0.0f ? value : -value;
        }

        if ( cmd->sidemove != 0.0f )
        {
            float value = zm_cl_zmmovespeed.GetFloat() * 0.01f;
            cmd->sidemove = cmd->sidemove > 0.0f ? value : -value;
        }


        // Let players go up/down with jump and sprint key.
        if ( cmd->buttons & IN_JUMP )
        {
            cmd->upmove += zm_cl_zmmovejumpspd.GetFloat() * 0.01f;
        }

        if ( cmd->buttons & IN_SPEED )
        {
            cmd->upmove -= zm_cl_zmmovespdspd.GetFloat() * 0.01f;
        }


        if ( g_pZMView && !g_pZMView->IsVisible() )
        {
            cmd->buttons |= IN_ZM_OBSERVERMODE;
        }
    }
    else if ( IsHuman() && IsAlive() )
    {
        GetZMSoftCollisions()->PerformPlayerSoftCollisions( cmd );
    }

    return bResult;
}

void C_ZMPlayer::SetMouseWheelMove( float dir )
{
    if ( dir != 0.0f )
    {
        // Keep going up until we are called again with 0.
        m_flNextUpMove = FLT_MAX;
        m_flUpMove = zm_cl_zmmovemwheelmovespd.GetFloat() * dir;
    }
    else
    {
        // We want to be stopped.
        if ( m_flNextUpMove > gpGlobals->curtime )
        {
            m_flNextUpMove = (gpGlobals->curtime + 0.1f);
        }
    }
}

bool C_ZMPlayer::HasExpensiveFlashlightOn() const
{
    return m_pFlashlight && m_pFlashlight->HasExpensiveOn();
}

void C_ZMPlayer::PreferExpensiveFlashlight( bool state )
{
    if ( m_pFlashlight )
    {
        m_pFlashlight->PreferExpensive( state );
    }
}
