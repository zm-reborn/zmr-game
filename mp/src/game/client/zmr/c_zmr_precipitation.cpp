#include "cbase.h"
#include "precipitation_shared.h"
#include "timedevent.h"
#include "debugoverlay_shared.h"

#include <raytrace.h>
#include <tier3/tier3.h>


#include "c_zmr_entities.h"
#include "c_zmr_player.h"
#include "c_zmr_precipitation.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_cl_precipitationcenterdist( "zm_cl_precipitationcenterdist", "150" );
ConVar zm_cl_precipitationheight( "zm_cl_precipitationheight", "180" );

ConVar zm_cl_debug_precipitation_trace( "zm_cl_debug_precipitation_trace", "0", FCVAR_CHEAT );


#define NEAR_OFFSET         zm_cl_precipitationcenterdist.GetFloat()
#define HEIGHT_OFFSET       zm_cl_precipitationheight.GetFloat()

#define UPDATE_RATE         8.0f

CUtlVector<RayTracingEnvironment*> g_RayTraceEnvironments;

//extern ConVar r_RainSplashPercentage;
//ConVar r_RainParticleDensity( "r_RainParticleDensity", "1", FCVAR_NONE, "Density of Particle Rain 0-1" );
//ConVar r_RainAllowInSplitScreen( "r_RainAllowInSplitScreen", "0", FCVAR_NONE, "Allows rain in splitscreen" );

ConVar zm_cl_precipitationquality( "zm_cl_precipitationquality", "3", FCVAR_ARCHIVE );

C_ZMPrecipitationSystem* ZMGetPrecipitationSystem()
{
    static C_ZMPrecipitationSystem s;
    return &s;
}



C_ZMPrecipitationSystem::C_ZMPrecipitationSystem() : CAutoGameSystemPerFrame( "ZMPrecipitationSystem" )
{
    m_pszParticleInner = "";
    m_pszParticleOuter = "";
    m_pszParticleMist = "";


    m_tParticlePrecipTraceTimer.Init( UPDATE_RATE );


    m_pParticlePrecipInner = nullptr;
    m_pParticlePrecipOuter = nullptr;
    m_pParticlePrecipMist = nullptr;


    m_bInitialized = false;
    m_iLastQuality = PRECIPQ_NONE;
}

C_ZMPrecipitationSystem::~C_ZMPrecipitationSystem()
{   
}

void C_ZMPrecipitationSystem::PostInit()
{
    //PrecacheParticleSystem( "rain" );
}

void C_ZMPrecipitationSystem::Update( float frametime )
{
    if ( !m_bInitialized || m_iLastQuality != GetQuality() )
    {
        InitializeParticles();

        m_iLastQuality = GetQuality();


        DestroyAllParticles();
    }

    UpdateParticles();
}

void C_ZMPrecipitationSystem::LevelInitPostEntity()
{
    m_bInitialized = false;
}

bool C_ZMPrecipitationSystem::AddPrecipitation( C_ZMEntPrecipitation* pEnt )
{
    if ( m_vPrecipitations.Find( pEnt ) != m_vPrecipitations.InvalidIndex() )
        return false;


    m_vPrecipitations.AddToTail( pEnt );


    BuildRayTracingEnv();

    return true;
}

bool C_ZMPrecipitationSystem::RemovePrecipitation( C_ZMEntPrecipitation* pEnt )
{
    bool ret = m_vPrecipitations.FindAndRemove( pEnt );

    if ( ret )
    {
        BuildRayTracingEnv();
    }

    return ret;
}

float C_ZMPrecipitationSystem::GetCurrentDensity() const
{
    return 1.0f;
}

PrecipitationQuality_t C_ZMPrecipitationSystem::GetQuality() const
{
    return (PrecipitationQuality_t)zm_cl_precipitationquality.GetInt();
}

PrecipitationType_t C_ZMPrecipitationSystem::GetPrecipitationType() const
{
    auto precipType = (PrecipitationType_t)-1;

    FOR_EACH_VEC( m_vPrecipitations, i )
    {
        auto mytype = m_vPrecipitations[i]->GetPrecipitationType();

        if ( precipType != (PrecipitationType_t)-1 && mytype != precipType )
        {
            Warning( "Multiple func_precipitation types in a single level! This is not supported!\n" );
        }

        precipType = mytype;
    }

    // Default to rain.
    if ( precipType == (PrecipitationType_t)-1 )
    {
        precipType = PRECIPITATION_TYPE_RAIN;
    }

    return precipType;
}

const CNewParticleEffect* C_ZMPrecipitationSystem::GetInner() const
{
    return m_pParticlePrecipInner;
}

const CNewParticleEffect* C_ZMPrecipitationSystem::GetOuter() const
{
    return m_pParticlePrecipOuter;
}

const CNewParticleEffect* C_ZMPrecipitationSystem::GetMist() const
{
    return m_pParticlePrecipMist;
}

CParticleProperty* C_ZMPrecipitationSystem::ParticleProp()
{
    // Get the world's particle property.
    auto* pEnt = cl_entitylist->GetBaseEntity( 0 );

    // How can the world not exist? :thinking:
    Assert( pEnt );

    if ( !pEnt )
        return nullptr;
    
    return pEnt->ParticleProp();
}

void C_ZMPrecipitationSystem::BuildRayTracingEnv()
{
    //
    // Sets up ray tracing environments for all func_precipitations and func_precipitation_blockers
    // There's no way to just add, we'll have to rebuild raytracing env.
    //
    int nTriCount;


    // We'll want to change this if/when we add more raytrace environments.
    g_RayTraceEnvironments.PurgeAndDeleteElements();


    const int fRtFlags = RTE_FLAGS_DONT_STORE_TRIANGLE_COLORS | RTE_FLAGS_DONT_STORE_TRIANGLE_MATERIALS;
    

    // Rain
    auto* rtEnvRainEmission = new RayTracingEnvironment();
    g_RayTraceEnvironments.AddToTail( rtEnvRainEmission );

    rtEnvRainEmission->Flags |= fRtFlags;

    nTriCount = 1;
    FOR_EACH_VEC( m_vPrecipitations, i )
    {
        auto* volume = m_vPrecipitations[i];

        auto* pCollide = modelinfo->GetVCollide( volume->GetModelIndex() );

        if ( !pCollide || pCollide->solidCount <= 0 )
        {
            Warning( "Precipitation %i has no collision data!\n", volume->entindex() );
            continue;
        }

        Vector* outVerts;
        int vertCount = g_pPhysicsCollision->CreateDebugMesh( pCollide->solids[0], &outVerts );

        if ( vertCount )
        {
            for ( int j = 0; j < vertCount; j += 3 )
            {
                rtEnvRainEmission->AddTriangle( nTriCount++, outVerts[j], outVerts[j + 1], outVerts[j + 2], Vector( 1, 1, 1 ) );
            }
        }

        physcollision->DestroyDebugMesh( vertCount, outVerts );
    }

    rtEnvRainEmission->SetupAccelerationStructure();


    // Blockers
    //auto* rtEnvRainBlocker = new RayTracingEnvironment();
    //g_RayTraceEnvironments.AddToTail( rtEnvRainBlocker );

    //rtEnvRainBlocker->Flags |= fRtFlags;

    //nTriCount = 1;

    //FOR_EACH_VEC( g_PrecipitationBlockers, i )
    //{
    //    auto* blocker = g_PrecipitationBlockers[i];

    //    auto* pCollide = modelinfo->GetVCollide( blocker->GetModelIndex() );

    //    if ( !pCollide || pCollide->solidCount <= 0 )
    //        continue;

    //    Vector* outVerts;
    //    int vertCount = g_pPhysicsCollision->CreateDebugMesh( pCollide->solids[0], &outVerts );

    //    if ( vertCount )
    //    {
    //        for ( int j = 0; j < vertCount; j += 3 )
    //        {
    //            rtEnvRainBlocker->AddTriangle( nTriCount++, outVerts[j], outVerts[j + 1], outVerts[j + 2], Vector( 1, 1, 1 ) );
    //        }
    //    }
    //    physcollision->DestroyDebugMesh( vertCount, outVerts );
    //}

    //rtEnvRainBlocker->SetupAccelerationStructure();
}

void C_ZMPrecipitationSystem::InitializeParticles()
{
    auto precipType = GetPrecipitationType();

//    //Set up which type of precipitation particle we'll use
//    if ( m_nPrecipType == PRECIPITATION_TYPE_PARTICLEASH )
//    {
//        m_pszParticleInner = "ash";
//        m_pszParticleOuter = "ash";
//        m_pszParticleOuter = "ash_outer";
//        m_flParticleInnerDist = 280.0;
//    }
//    else if ( m_nPrecipType == PRECIPITATION_TYPE_PARTICLESNOW )
//    {
//#ifdef INFESTED_DLL
//        m_pParticleInnerNearDef = "asw_snow";
//        m_pParticleInnerFarDef = "asw_snow";
//        m_pParticleOuterDef = "asw_snow_outer";
//        m_flParticleInnerDist = 240.0;
//#else
//        m_pszParticleInner = "snow";
//        m_pszParticleOuter = "snow";
//        m_pszParticleOuter = "snow_outer";
//        m_flParticleInnerDist = 280.0;
//#endif
//    }
//    else if ( m_nPrecipType == PRECIPITATION_TYPE_PARTICLERAINSTORM )
//    {
//        m_pszParticleInner = "rain_storm";
//        m_pszParticleOuter = "rain_storm_screen";
//        m_pszParticleOuter = "rain_storm_outer";
//        m_flParticleInnerDist = 0.0;
//    }
    if ( precipType == PRECIPITATION_TYPE_SNOW || precipType == PRECIPITATION_TYPE_SNOWFALL )
    {
        m_pszParticleInner = "snow";
        m_pszParticleOuter = "snow_outer";
        m_pszParticleMist = "snow_mist";
    }
    else  // Default to rain
    {
        if ( precipType != PRECIPITATION_TYPE_RAIN )
        {
            Warning( "Precipitation type %i is not supported! Defaulting to rain...\n", precipType );
        }

        m_pszParticleInner = "rain";
        m_pszParticleOuter = "rain_outer";
        m_pszParticleMist = "rain_mist";
    }


    Assert( m_pszParticleInner && m_pszParticleOuter && m_pszParticleMist );

    if ( !g_pParticleSystemMgr->FindParticleSystem( m_pszParticleInner ) )
        Warning( "Couldn't find rain particle effect '%s'!!\n", m_pszParticleInner );
    if ( !g_pParticleSystemMgr->FindParticleSystem( m_pszParticleOuter ) )
        Warning( "Couldn't find rain particle effect '%s'!!\n", m_pszParticleOuter );
    if ( !g_pParticleSystemMgr->FindParticleSystem( m_pszParticleMist ) )
        Warning( "Couldn't find rain particle effect '%s'!!\n", m_pszParticleMist );

    BuildRayTracingEnv();

    m_bInitialized = true;
}

void C_ZMPrecipitationSystem::UpdateParticles()
{
    auto* pPlayer = C_ZMPlayer::GetLocalPlayer();

    if (!pPlayer
    ||  m_iLastQuality == PRECIPQ_NONE // We don't want rain
    ||  g_RayTraceEnvironments.Count() < 1
    ||  !ParticleProp() )
    {
        DestroyAllParticles();
        return;
    }


    float flCurTime = gpGlobals->frametime;

    while ( m_tParticlePrecipTraceTimer.NextEvent( flCurTime ) )
    {
        Vector vForward;
        Vector vTraceForward;
        Vector vRight;
        Vector vUp;

        // Get the rain volume Ray Tracing Environment.  Currently hard coded to 0, should have this lookup 
        RayTracingEnvironment* rtEnv = g_RayTraceEnvironments.Element( 0 );

        //float flDensity = GetCurrentDensity();

        pPlayer->GetVectors( &vForward, nullptr, nullptr );

        vForward.z = 0.0f;
        vForward.NormalizeInPlace();

        // Point the forward vector up a bit.
        vTraceForward = vForward;
        vTraceForward.z = 0.5f;
        vTraceForward.NormalizeInPlace();

        VectorVectors( vForward, vRight, vUp );

        // Trace more to the sides.
        Vector vForwardRight = vTraceForward * 0.8f + vRight;
        Vector vForwardLeft = vTraceForward * 0.8f - vRight;
        vForwardRight.NormalizeInPlace();
        vForwardLeft.NormalizeInPlace();

        fltx4 TMax = ReplicateX4( 600.0f );
        SubFloat( TMax, 3 ) = 2048.0f;


        Vector vPlayerPos = pPlayer->EyePosition();
        Vector vOffsetPos = vPlayerPos + Vector ( 0, 0, HEIGHT_OFFSET );
        Vector vOffsetPosNear = vOffsetPos + ( vForward * NEAR_OFFSET );

        //Vector vDensity = Vector( r_RainParticleDensity.GetFloat(), 0, 0 ) * flDensity;


        // Our 4 Rays are forward, off to the left and right, and directly up.
        // Use the first three to determine if there's generally visible rain where we're looking.
        // The forth, straight up, tells us if we're standing inside a rain volume 
        // (based on the normal that we hit or if we miss entirely)
        FourRays frRays;
        FourVectors fvDirection;
        fvDirection = FourVectors( vTraceForward, vForwardLeft, vForwardRight, Vector( 0, 0, 1 ) );
        frRays.direction = fvDirection; 
        frRays.origin.DuplicateVector( vPlayerPos );
        RayTracingResult Result;

        rtEnv->Trace4Rays( frRays, Four_Zeros, TMax, &Result );

        i32x4 in4HitIds = LoadAlignedIntSIMD( Result.HitIds );
        fltx4 fl4HitIds = SignedIntConvertToFltSIMD ( in4HitIds );

        bool bInside = ( Result.HitIds[3] != -1 );
        bool bNearby = ( IsAnyNegative( CmpGeSIMD( fl4HitIds, Four_Zeros ) ) && IsAnyNegative( CmpLtSIMD( Result.HitDistance, TMax ) ) );


        // Debug
        if ( zm_cl_debug_precipitation_trace.GetBool() )
        {
            static bool bLastInside = false;
            static bool bLastNearby = false;

            for ( int i = 0; i < 4; i++ )
            {
                bool bHit = Result.HitIds[i] != -1;

                Vector dir( SubFloat( frRays.direction[0], i ), SubFloat( frRays.direction[1], i ), SubFloat( frRays.direction[2], i ) );
                NDebugOverlay::Line(
                    vPlayerPos,
                    vPlayerPos + dir *  SubFloat( Result.HitDistance, i ),
                    (!bHit) ? 255: 0, bHit ? 255 : 0, 0,
                    true, 1.0f / UPDATE_RATE );
            }

            if ( bLastInside != bInside || bLastNearby != bNearby )
            {
                Msg( "Inside: %i | Nearby: %i\n", (int)bInside, (int)bNearby );
            }

            bLastInside = bInside;
            bLastNearby = bNearby;
        }
        
        if ( bInside || bNearby )
        {
            //
            // Update if we've already got systems, otherwise, create them.
            //

            // Inner
            if ( m_pParticlePrecipInner )
            {
                m_pParticlePrecipInner->SetControlPoint( 1, vOffsetPos );
                //m_pParticlePrecipInner->SetControlPoint( 3, vDensity );
                m_pParticlePrecipInner->SetControlPoint( 3, vOffsetPosNear );
            }
            else
            {
                DispatchInnerParticlePrecip( pPlayer, vForward );
            }

            // Outer
            if ( m_pParticlePrecipOuter )
            {
                m_pParticlePrecipOuter->SetControlPoint( 1, vOffsetPos );
                //m_pParticlePrecipOuter->SetControlPoint( 3, vDensity );
                m_pParticlePrecipOuter->SetControlPoint( 3, vOffsetPosNear );
            }
            else
            {
                DispatchOuterParticlePrecip( pPlayer, vForward );
            }

            // Mist
            if ( m_pParticlePrecipMist )
            {
                m_pParticlePrecipMist->SetControlPoint( 1, vOffsetPos );
                //m_pParticlePrecipOuter->SetControlPoint( 3, vDensity );
                m_pParticlePrecipMist->SetControlPoint( 3, vOffsetPosNear );
            }
            else
            {
                DispatchMistParticlePrecip( pPlayer, vForward );
            }
        }
        else  // No rain in the area, kill any leftover systems.
        {
            DestroyAllParticles();
        }
    }
}

// IMPORTANT: Always destroy particle systems through the particle prop and not directly.
// The prop holds a reference to the particle system.
// There was a crash when an entity was destroyed with its particle prop in ParticleProperty::StopEmission.
void C_ZMPrecipitationSystem::DestroyInnerParticlePrecip()
{
    if ( m_pParticlePrecipInner )
    {
        auto* pParticles = ParticleProp();
        pParticles->StopEmission( m_pParticlePrecipInner );
        m_pParticlePrecipInner = nullptr;
    }
}

void C_ZMPrecipitationSystem::DestroyOuterParticlePrecip()
{
    if ( m_pParticlePrecipOuter )
    {
        auto* pParticles = ParticleProp();
        pParticles->StopEmission( m_pParticlePrecipOuter );
        m_pParticlePrecipOuter = nullptr;
    }
}

void C_ZMPrecipitationSystem::DestroyMistParticlePrecip()
{
    if ( m_pParticlePrecipMist )
    {
        auto* pParticles = ParticleProp();
        pParticles->StopEmission( m_pParticlePrecipMist );
        m_pParticlePrecipMist = nullptr;
    }
}

void C_ZMPrecipitationSystem::DestroyAllParticles()
{
    DestroyInnerParticlePrecip();
    DestroyOuterParticlePrecip();
    DestroyMistParticlePrecip();
}

void C_ZMPrecipitationSystem::DispatchOuterParticlePrecip( C_BasePlayer *pPlayer, const Vector& vForward )
{
    DestroyOuterParticlePrecip();


    // Low quality gets no outer particles.
    if ( m_iLastQuality < PRECIPQ_MEDIUM )
    {
        return;
    }


    Vector vPlayerPos = pPlayer->EyePosition();
    Vector vOffsetPos = vPlayerPos + Vector ( 0, 0, HEIGHT_OFFSET );
    Vector vOffsetPosNear = vOffsetPos + ( vForward * NEAR_OFFSET );


    auto* pParticles = ParticleProp();
    Assert( pParticles );

    m_pParticlePrecipOuter = pParticles->Create( m_pszParticleOuter, PATTACH_ABSORIGIN_FOLLOW );
    Assert( m_pParticlePrecipOuter );


    m_pParticlePrecipOuter->SetControlPointObject( 4, pPlayer );
    m_pParticlePrecipOuter->SetControlPoint( 1, vOffsetPos );
    m_pParticlePrecipOuter->SetControlPointEntity( 2, pPlayer );
    m_pParticlePrecipOuter->SetControlPoint( 3, vOffsetPosNear );
    //m_pParticlePrecipOuter->SetDrawOnlyForSplitScreenUser( nSlot );
}

void C_ZMPrecipitationSystem::DispatchMistParticlePrecip( C_BasePlayer *pPlayer, const Vector& vForward )
{
    DestroyOuterParticlePrecip();


    // Medium quality gets no outer mist particles.
    if ( m_iLastQuality <= PRECIPQ_MEDIUM )
    {
        return;
    }


    Vector vPlayerPos = pPlayer->EyePosition();
    Vector vOffsetPos = vPlayerPos + Vector ( 0, 0, HEIGHT_OFFSET );
    Vector vOffsetPosNear = vOffsetPos + ( vForward * NEAR_OFFSET );


    auto* pParticles = ParticleProp();
    Assert( pParticles );

    m_pParticlePrecipOuter = pParticles->Create( m_pszParticleMist, PATTACH_ABSORIGIN_FOLLOW );
    Assert( m_pParticlePrecipOuter );


    m_pParticlePrecipOuter->SetControlPointObject( 4, pPlayer );
    m_pParticlePrecipOuter->SetControlPoint( 1, vOffsetPos );
    m_pParticlePrecipOuter->SetControlPointEntity( 2, pPlayer );
    m_pParticlePrecipOuter->SetControlPoint( 3, vOffsetPosNear );
    //m_pParticlePrecipOuter->SetDrawOnlyForSplitScreenUser( nSlot );
}

void C_ZMPrecipitationSystem::DispatchInnerParticlePrecip( C_BasePlayer *pPlayer, const Vector& vForward )
{
    DestroyInnerParticlePrecip();


    Vector vPlayerPos = pPlayer->EyePosition();
    Vector vOffsetPos = vPlayerPos + Vector ( 0, 0, HEIGHT_OFFSET );
    Vector vOffsetPosNear = vOffsetPos + ( vForward * NEAR_OFFSET );


    auto* pParticles = ParticleProp();
    Assert( pParticles );

    m_pParticlePrecipInner = pParticles->Create( m_pszParticleInner, PATTACH_ABSORIGIN_FOLLOW );
    Assert( m_pParticlePrecipInner );


    m_pParticlePrecipInner->SetControlPoint( 1, vOffsetPos );
    m_pParticlePrecipInner->SetControlPointEntity( 2, pPlayer );
    m_pParticlePrecipInner->SetControlPoint( 3, vOffsetPosNear );
    //m_pParticlePrecipInner->SetDrawOnlyForSplitScreenUser( nSlot );
}
