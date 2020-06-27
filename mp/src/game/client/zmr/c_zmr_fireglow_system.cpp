#include "cbase.h"
#include "iefx.h"
#include "debugoverlay_shared.h"

#include "c_zmr_fireglow_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//
// The fireglow system attempts to centralize dynamic fire lights.
//
// A few methods are used to reduce the amount of active dynamic lights:
//
// - Lights will be merged together if they are the same type and within proximity.
// - If a light is spawned during a round start, a special check is performed
//   to see if the mapper already inserted a world light at that point making the dynamic light pointless.
//   This was a problem in bluevelvet, but it seems to be the only map having this issue. (UNDONE FOR NOW)
//

CountdownTimer CZMFireGlowSystem::m_MapStartTimer;

void UTIL_ParseColorFromString( const char* str, int clr[], int nColors );

ConVar zm_cl_fireglow_debug( "zm_cl_fireglow_debug", "0", FCVAR_CHEAT );
ConVar zm_cl_fireglow_max( "zm_cl_fireglow_max", "10", FCVAR_ARCHIVE );
ConVar zm_cl_fireglow_decay( "zm_cl_fireglow_decay", "1.0" );
ConVar zm_cl_fireglow_updaterate( "zm_cl_fireglow_updaterate", "30" );
ConVar zm_cl_fireglow_flickerrate( "zm_cl_fireglow_flickerrate", "10" );
ConVar zm_cl_fireglow_radius_base( "zm_cl_fireglow_radius_base", "120" );
ConVar zm_cl_fireglow_radius_random( "zm_cl_fireglow_radius_random", "32" );
ConVar zm_cl_fireglow_color( "zm_cl_fireglow_color", "254 95 10 2", 0, "Fourth number is exponent." );
ConVar zm_cl_fireglow_combine_dist( "zm_cl_fireglow_combine_dist", "35" );


CON_COMMAND_F( zm_cl_debug_getlightatpos, "", FCVAR_CHEAT )
{
    auto* pLocal = CBasePlayer::GetLocalPlayer();
    if ( !pLocal ) return;


    Vector pos;

    if ( args.ArgC() >= 4 )
    {
        for ( int i = 0; i < 3; i++ )
            pos[i] = Q_atof( args.Arg( 1 + i ) );
    }
    else
    {
        pos = pLocal->EyePosition();
    }

    Vector clr;

    engine->ComputeLighting( pos, nullptr, true, clr, nullptr );


    Msg( "Lighting color at pos (%.1f, %.1f, %.1f)\n",
        pos.x, pos.y, pos.z );

    Msg( "Clamped: %.2f, %.2f, %.2f\n",
        clr.x, clr.y, clr.z );

    clr.NormalizeInPlace();
    Msg( "Normalized: %.2f, %.2f, %.2f\n",
        clr.x, clr.y, clr.z );
}

FireGlow_t::FireGlow_t( FireGlowEnt_t* pEnt, FireGlowType_t glowType ) : glowType( glowType )
{
    static int lightIndices = 4096;


    pLight = nullptr;
    flNextUpdate = 0.0f;
    flNextFlicker = 0.0f;

    vpEnts.AddToTail( pEnt );

    // If it's a client entity, it will have no index.
    // Create a unique index if so.
    iLightIndex = ( pEnt->index > 0 ) ? pEnt->index : lightIndices++;

    vecLastPos = ComputePosition();

    bDying = false;

    bCheckPositionLightLevel = CZMFireGlowSystem::WaitMapStart();
}

FireGlow_t::~FireGlow_t()
{
    // We should've decayed!
    if ( pLight )
    {
        pLight->die = gpGlobals->curtime;
        pLight = nullptr;
    }
}

void FireGlow_t::Update( float flUpdateInterval )
{
    const float flFlickerInterval = 1.0f / zm_cl_fireglow_flickerrate.GetFloat();

    float curtime = gpGlobals->curtime;

    if ( bDying )
        return;


    Assert( vpEnts.Count() > 0 );
    if ( vpEnts.Count() <= 0 )
        return;

    //auto* pEnt = vpEnts[0];

    //if ( bCheckPositionLightLevel && FireGlow_t::IsWorldLightSufficient( pEnt, glowType ) )
    //{
    //    Kill();
    //    return;
    //}

    //bCheckPositionLightLevel = false;

    if ( flNextUpdate > curtime )
        return;

    flNextUpdate = gpGlobals->curtime + flUpdateInterval;

    

    if ( !pLight )
    {
        pLight = effects->CL_AllocDlight( iLightIndex );

        if ( !pLight )
        {
            Warning( "Failed to allocate a dynamic light for fileglow system!\n" );
            return;
        }


        int clr[4];
        UTIL_ParseColorFromString( zm_cl_fireglow_color.GetString(), clr, ARRAYSIZE( clr ) );

	    pLight->color.r = (unsigned char)clr[0];
	    pLight->color.g = (unsigned char)clr[1];
	    pLight->color.b = (unsigned char)clr[2];
        pLight->color.exponent = (unsigned char)clr[3];
        pLight->decay = 0.0f;
    }
    
    if ( flNextFlicker <= curtime )
    {
        float ratio = RandomFloat();
        pLight->radius = (zm_cl_fireglow_radius_base.GetFloat() + zm_cl_fireglow_radius_random.GetFloat() * ratio);

        flNextFlicker = curtime + flFlickerInterval;
    }
	

	vecLastPos = ComputePosition();
	pLight->origin = vecLastPos;
	pLight->die = curtime + MAX( flUpdateInterval, 0.1f );
    

    if ( CZMFireGlowSystem::IsDebugging() )
    {
        NDebugOverlay::Axis( vecLastPos, vec3_angle, 16.0f, true, flUpdateInterval );
    }
}

void FireGlow_t::Kill()
{
    if ( pLight )
    {
        pLight->die = gpGlobals->curtime;
        pLight = nullptr;
    }

    vpEnts.RemoveAll();

    bDying = true;
}

void FireGlow_t::Decay( float tim )
{
    if ( pLight )
    {
        pLight->decay = pLight->radius / tim;
        pLight->die = gpGlobals->curtime + tim;
        pLight = nullptr;
    }

    vpEnts.RemoveAll();

    bDying = true;
}

bool FireGlow_t::DecaySingleEntity( float tim, FireGlowEnt_t* pEnt )
{
    bool removed = vpEnts.FindAndRemove( pEnt );

    if ( removed )
    {
        // Create a new light at the new center.
        if ( pLight )
        {
            pLight->decay = pLight->radius / tim;
            pLight->die = gpGlobals->curtime + tim;
            pLight = nullptr;
        }
    }

    if ( vpEnts.Count() <= 0 )
    {
        bDying = true;
    }

    return removed;
}

Vector FireGlow_t::ComputePosition()
{
    int count = vpEnts.Count();

    if ( !count )
    {
        return vecLastPos;
    }

    Vector avg;
    avg.Init();

    for ( int i = 0; i < count; i++ )
    {
        auto* pEnt = vpEnts[i];

        avg += pEnt->GetAbsOrigin();
    }

    avg.z += 16.0f * count;

    return avg / count;
}

bool FireGlow_t::ShouldCombine( const Vector& pos ) const
{
    if ( glowType == FireGlowType_t::GLOW_ENTITY_FLAME )
        return false;


    float dist = zm_cl_fireglow_combine_dist.GetFloat();

     return pos.DistToSqr( vecLastPos ) <= (dist*dist);
}

//
// Check if the world light near us will be sufficient "glow" for this fire.
//
bool FireGlow_t::IsWorldLightSufficient( FireGlowEnt_t* pEnt, FireGlowType_t glowType )
{
    if ( glowType != FireGlowType_t::GLOW_GENERIC_FIRE )
        return false;


    auto pos = pEnt->GetAbsOrigin();
    pos.z += 16.0f;

    
    Vector clr;

    //clr = engine->GetLightForPoint( pos, false ); // These are the same thing?
    engine->ComputeLighting( pos, nullptr, false, clr, nullptr );

    clr.NormalizeInPlace();

    if ( CZMFireGlowSystem::IsDebugging() )
    {
        DevMsg( "Ambient color at pos (%.1f, %.1f, %.1f): %.2f, %.2f, %.2f\n",
            pos.x, pos.y, pos.z,
            clr.x, clr.y, clr.z );
    }
        

    bool bIsInRed = clr.x > clr.y && clr.x > clr.z;

    if ( bIsInRed )
    {
        if ( CZMFireGlowSystem::IsDebugging() )
        {
            DevMsg( "Fire entity %i is already in a fire like light!\n", pEnt->index );
        }

        return true;
    }

    return false;
}

CZMFireGlowSystem::CZMFireGlowSystem() : CAutoGameSystemPerFrame( "ZMFireGlowSystem" )
{
}

CZMFireGlowSystem::~CZMFireGlowSystem()
{
}

void CZMFireGlowSystem::PostInit()
{
    ListenForGameEvent( "round_restart_post" );
}

void CZMFireGlowSystem::LevelInitPostEntity()
{
    DevMsg( "CZMFireGlowSystem::LevelInitPostEntity()\n" );

    // Any fire added within this time will be considered a map specific fire.
    m_MapStartTimer.Start( 2.0f );
}

void CZMFireGlowSystem::FireGameEvent( IGameEvent* pEvent )
{
    if ( Q_strcmp( pEvent->GetName(), "round_restart_post" ) == 0 )
    {
        // Any fire added within this time will be considered a map specific fire.
        m_MapStartTimer.Start( 2.0f );
    }
}

void CZMFireGlowSystem::Update( float frametime )
{
    const float updateInterval = 1.0f / zm_cl_fireglow_updaterate.GetFloat();


    const int nMaxCount = GetMaxCount();

    // Remove dying lights.
    for ( int i = FindDying(); i != m_vFireEntities.InvalidIndex(); i = FindDying() )
    {
        m_vFireEntities.Remove( i );
    }

    // Remove fires going over the limit.
    while ( m_vFireEntities.Count() > nMaxCount )
    {
        delete m_vFireEntities[0];
        m_vFireEntities[0] = nullptr;

        m_vFireEntities.Remove( 0 );
    }


    // SUPER HACK: This is here to assure we have lightmaps loaded to perform the worldlight check.
    // Apparently lightmaps are loaded AFTER the map env_fires spawn on client.
    if ( CZMFireGlowSystem::WaitMapStart() )
    {
        return;
    }


    FOR_EACH_VEC( m_vFireEntities, i )
    {
        auto* pFireData = m_vFireEntities[i];
        auto type = m_vFireEntities[i]->glowType;

        switch ( type )
        {
        case FireGlowType_t::GLOW_GENERIC_FIRE :
        default :
            pFireData->Update( updateInterval );
            break;
        }
    }
}

int CZMFireGlowSystem::AddFireEntity( FireGlowEnt_t* pEnt, FireGlowType_t glowType )
{
    const int nMaxCount = GetMaxCount();

    if ( nMaxCount <= 0 )
        return m_vFireEntities.InvalidIndex();


    if ( !pEnt || FindFireEntity( pEnt ) != m_vFireEntities.InvalidIndex() )
    {
        return m_vFireEntities.InvalidIndex();
    }


    int iCombined = AttemptCombine( pEnt, glowType );

    if ( iCombined != m_vFireEntities.InvalidIndex() )
    {
        return iCombined;
    }



    if ( m_vFireEntities.Count() >= nMaxCount )
    {
        int iReplace = FindSuitableReplacement();

        if ( IsDebugging() )
        {
            Msg( "Too many fireglow entities! Removing fire index %i...\n", iReplace );
        }

        delete m_vFireEntities[iReplace];
        m_vFireEntities[iReplace] = nullptr;

        m_vFireEntities.Remove( iReplace );
    }


    if ( IsDebugging() )
    {
        Msg( "Adding new fireglow entity %i to index %i!\n", pEnt->index, m_vFireEntities.Count() );
    }

    auto* pData = new FireGlow_t( pEnt, glowType );

    return m_vFireEntities.AddToTail( pData );
}

bool CZMFireGlowSystem::RemoveFireEntity( FireGlowEnt_t* pEnt )
{
    int i = FindFireEntity( pEnt );
    if ( i != m_vFireEntities.InvalidIndex() )
    {
        if ( IsDebugging() )
        {
            Msg( "Removing entity %i from fireglow index %i!\n", pEnt->index, i );
        }

        m_vFireEntities[i]->DecaySingleEntity( zm_cl_fireglow_decay.GetFloat(), pEnt );

        if ( m_vFireEntities[i]->bDying )
        {
            if ( IsDebugging() )
            {
                Msg( "Removing fireglow index %i!\n", i );
            }

            delete m_vFireEntities[i];
            m_vFireEntities[i] = nullptr;

            m_vFireEntities.Remove( i );
        }

        return true;
    }

    return false;
}

int CZMFireGlowSystem::GetMaxCount() const
{
    return abs( zm_cl_fireglow_max.GetInt() );
}

int CZMFireGlowSystem::AttemptCombine( FireGlowEnt_t* pEnt, FireGlowType_t glowType ) const
{
    Vector origin = pEnt->GetAbsOrigin();

    FOR_EACH_VEC( m_vFireEntities, i )
    {
        if ( m_vFireEntities[i]->glowType != glowType )
            continue;

        if ( m_vFireEntities[i]->ShouldCombine( origin ) )
        {
            if ( IsDebugging() )
            {
                Msg( "Combining fire entity %i to index %i!\n", pEnt->index, i );
            }

            m_vFireEntities[i]->vpEnts.AddToTail( pEnt );
            return i;
        }
    }

    return m_vFireEntities.InvalidIndex();
}

int CZMFireGlowSystem::FindFireEntity( FireGlowEnt_t* pEnt ) const
{
    FOR_EACH_VEC( m_vFireEntities, i )
    {
        FOR_EACH_VEC( m_vFireEntities[i]->vpEnts, j )
        {
            if ( m_vFireEntities[i]->vpEnts[j] == pEnt )
            {
                return i;
            }
        }
    }

    return m_vFireEntities.InvalidIndex();
}

int CZMFireGlowSystem::FindDying() const
{
    if ( m_vFireEntities.Count() > 0 )
    {
        FOR_EACH_VEC( m_vFireEntities, i )
        {
            if ( m_vFireEntities[i]->bDying )
            {
                return i;
            }
        }
    }

    return m_vFireEntities.InvalidIndex();
}

int CZMFireGlowSystem::FindSuitableReplacement() const
{
    Assert( m_vFireEntities.Count() > 0 );

    int iDying = FindDying();
    if ( iDying != m_vFireEntities.InvalidIndex() )
        return iDying;


    return 0;
}

bool CZMFireGlowSystem::IsDebugging()
{
    return zm_cl_fireglow_debug.GetBool();
}

bool CZMFireGlowSystem::WaitMapStart()
{
    return m_MapStartTimer.HasStarted() && !m_MapStartTimer.IsElapsed();
}


CZMFireGlowSystem g_ZMFireGlowSystem;
