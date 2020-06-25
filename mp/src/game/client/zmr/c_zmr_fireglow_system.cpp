#include "cbase.h"
#include "iefx.h"
#include "debugoverlay_shared.h"

#include "c_zmr_fireglow_system.h"


void UTIL_ParseColorFromString( const char* str, int clr[], int nColors );

ConVar zm_cl_fireglow_debug( "zm_cl_fireglow_debug", "0", FCVAR_CHEAT );
ConVar zm_cl_fireglow_max( "zm_cl_fireglow_max", "4" );
ConVar zm_cl_fireglow_decay( "zm_cl_fireglow_decay", "1.0" );
ConVar zm_cl_fireglow_updaterate( "zm_cl_fireglow_updaterate", "10" );
ConVar zm_cl_fireglow_radius_base( "zm_cl_fireglow_radius_base", "120" );
ConVar zm_cl_fireglow_radius_random( "zm_cl_fireglow_radius_random", "32" );
ConVar zm_cl_fireglow_color( "zm_cl_fireglow_color", "254 95 10 2", 0, "Fourth number is exponent." );
ConVar zm_cl_fireglow_combine_dist( "zm_cl_fireglow_combine_dist", "65", 0, "Fourth number is exponent." );


CON_COMMAND_F( zm_cl_debug_getlightatpos, "", FCVAR_CHEAT )
{
    auto* pLocal = CBasePlayer::GetLocalPlayer();
    if ( !pLocal ) return;



    auto pos = pLocal->EyePosition();

    //auto clr = engine->GetLightForPoint( pos, false );
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

FireData_t::FireData_t( FireGlowEnt_t* pEnt, FireGlowType_t glowType ) : glowType( glowType )
{
    pLight = nullptr;
    flNextUpdate = 0.0f;

    vpEnts.AddToTail( pEnt );

    vecLastPos = ComputePosition();

    bDying = false;
}

FireData_t::~FireData_t()
{
    // We should've decayed!
    if ( pLight )
    {
        pLight->die = gpGlobals->curtime;
        pLight = nullptr;
    }
}

void FireData_t::Update( float flUpdateInterval )
{
    float curtime = gpGlobals->curtime;

    if ( bDying )
        return;


    Assert( vpEnts.Count() > 0 );
    if ( vpEnts.Count() <= 0 )
        return;

    if ( flNextUpdate > curtime )
        return;

    flNextUpdate = gpGlobals->curtime + flUpdateInterval;

    auto* pEnt = vpEnts[0];

    float clrMod = RandomFloat();

    int clr[4];
    UTIL_ParseColorFromString( zm_cl_fireglow_color.GetString(), clr, ARRAYSIZE( clr ) );

    if ( !pLight )
    {
        pLight = effects->CL_AllocDlight( pEnt->index );

        if ( !pLight )
        {
            Warning( "Failed to allocate a dynamic light for fileglow system!\n" );
            return;
        }
    }
    

	vecLastPos = ComputePosition();
	pLight->origin = vecLastPos;
	pLight->color.r = (unsigned char)clr[0];
	pLight->color.g = (unsigned char)clr[1];
	pLight->color.b = (unsigned char)clr[2];
    pLight->color.exponent = (unsigned char)clr[3];
	pLight->radius = (zm_cl_fireglow_radius_base.GetFloat() + zm_cl_fireglow_radius_random.GetFloat() * clrMod);
	pLight->die = gpGlobals->curtime + 1.0f;
    pLight->decay = 0.0f;

    if ( CZMFireGlowSystem::IsDebugging() )
    {
        NDebugOverlay::Axis( vecLastPos, vec3_angle, 16.0f, true, flUpdateInterval );
    }
}

void FireData_t::Kill()
{
    if ( pLight )
    {
        pLight->die = gpGlobals->curtime;
        pLight = nullptr;
    }

    vpEnts.RemoveAll();

    bDying = true;
}

void FireData_t::Decay( float tim )
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

bool FireData_t::DecaySingleEntity( float tim, FireGlowEnt_t* pEnt )
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

Vector FireData_t::ComputePosition()
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

bool FireData_t::ShouldCombine( const Vector& pos ) const
{
    if ( glowType == FireGlowType_t::GLOW_IMMOLATOR )
        return false;


    float dist = zm_cl_fireglow_combine_dist.GetFloat();

     return pos.DistToSqr( vecLastPos ) <= (dist*dist);
}

bool FireData_t::ShouldBeRemoved( FireGlowEnt_t* pEnt, FireGlowType_t glowType )
{
    if ( glowType != FireGlowType_t::GLOW_GENERIC_FIRE )
        return false;


    auto pos = pEnt->GetAbsOrigin();
    pos.z += 16.0f;

    //auto clr = engine->GetLightForPoint( pos, false );
    Vector clr;

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

void CZMFireGlowSystem::Update( float frametime )
{
    const float updateInterval = 1.0f / zm_cl_fireglow_updaterate.GetFloat();


    const int nMaxCount = GetMaxCount();

    while ( m_vFireEntities.Count() > nMaxCount )
    {
        delete m_vFireEntities[0];
        m_vFireEntities[0] = nullptr;

        m_vFireEntities.Remove( 0 );
    }

    for ( int i = FindDying(); i != m_vFireEntities.InvalidIndex(); i = FindDying() )
    {
        m_vFireEntities.Remove( i );
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

void CZMFireGlowSystem::LevelInitPostEntity()
{
}

int CZMFireGlowSystem::AddFireEntity( FireGlowEnt_t* pEnt )
{
    const int nMaxCount = GetMaxCount();

    if ( nMaxCount <= 0 )
        return m_vFireEntities.InvalidIndex();


    FireGlowType_t glowType = FireGlowType_t::GLOW_GENERIC_FIRE;

    if ( !pEnt || FindFireEntity( pEnt ) != m_vFireEntities.InvalidIndex() )
    {
        return m_vFireEntities.InvalidIndex();
    }



    if ( FireData_t::ShouldBeRemoved( pEnt, glowType ) )
    {
        return m_vFireEntities.InvalidIndex();
    }


    int iCombined = AttemptCombine( pEnt );

    if ( iCombined != m_vFireEntities.InvalidIndex() )
    {
        return iCombined;
    }



    if ( m_vFireEntities.Count() >= nMaxCount )
    {
        if ( IsDebugging() )
        {
            Msg( "Too many fireglow entities! Removing...\n" );
        }

        delete m_vFireEntities[0];
        m_vFireEntities[0] = nullptr;

        m_vFireEntities.Remove( 0 );
    }


    if ( IsDebugging() )
    {
        Msg( "Adding new fireglow entity %i to slot %i!\n", pEnt->index, m_vFireEntities.Count() );
    }

    auto* pData = new FireData_t( pEnt, FireGlowType_t::GLOW_GENERIC_FIRE );

    return m_vFireEntities.AddToTail( pData );
}

bool CZMFireGlowSystem::RemoveFireEntity( FireGlowEnt_t* pEnt )
{
    int i = FindFireEntity( pEnt );
    if ( i != m_vFireEntities.InvalidIndex() )
    {
        if ( IsDebugging() )
        {
            Msg( "Removing entity %i from fireglow slot %i!\n", pEnt->index, i );
        }

        m_vFireEntities[i]->DecaySingleEntity( zm_cl_fireglow_decay.GetFloat(), pEnt );

        if ( m_vFireEntities[i]->bDying )
        {
            if ( IsDebugging() )
            {
                Msg( "Removing fireglow slot %i!\n", i );
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

int CZMFireGlowSystem::AttemptCombine( FireGlowEnt_t* pEnt ) const
{
    Vector origin = pEnt->GetAbsOrigin();

    FOR_EACH_VEC( m_vFireEntities, i )
    {
        if ( m_vFireEntities[i]->ShouldCombine( origin ) )
        {
            if ( IsDebugging() )
            {
                Msg( "Combining fire entity %i to slot %i!\n", pEnt->index, i );
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

bool CZMFireGlowSystem::IsDebugging()
{
    return zm_cl_fireglow_debug.GetBool();
}


CZMFireGlowSystem g_ZMFireGlowSystem;
