#include "cbase.h"
#include "dlight.h"
#include "iefx.h"

#include "c_zmr_fireglow_system.h"


CZMFireGlowSystem::CZMFireGlowSystem() : CAutoGameSystemPerFrame( "ZMFireGlowSystem" )
{
}

CZMFireGlowSystem::~CZMFireGlowSystem()
{
}

ConVar zm_cl_fireglow_max( "zm_cl_fireglow_max", "4" );
ConVar zm_cl_fireglow_decay( "zm_cl_fireglow_decay", "0" );
ConVar zm_cl_fireglow_updaterate( "zm_cl_fireglow_updaterate", "10" );
ConVar zm_cl_fireglow_radius_base( "zm_cl_fireglow_radius_base", "400" );
ConVar zm_cl_fireglow_radius_random( "zm_cl_fireglow_radius_random", "32" );

void CZMFireGlowSystem::Update( float frametime )
{
    float curtime = gpGlobals->curtime;

    const float updateInterval = 1.0f / zm_cl_fireglow_updaterate.GetFloat();


    FOR_EACH_VEC( m_vFireEntities, i )
    {
        auto& fireData = m_vFireEntities[i];
        auto* pEnt = m_vFireEntities[i].pEnt;
        auto type = m_vFireEntities[i].glowType;


        if ( !pEnt->IsVisible() )
            continue;

        if ( fireData.flNextUpdate > curtime )
            continue;

        fireData.flNextUpdate = curtime + updateInterval;

        switch ( type )
        {
        case FireGlowType_t::GLOW_GENERIC_FIRE :
        default :
            float clrMod = RandomFloat();

		    dlight_t* dl = effects->CL_AllocDlight( pEnt->index );
		    dl->origin = pEnt->GetAbsOrigin();
		    dl->origin.z += 16.0f;
		    dl->color.r = 254;
		    dl->color.g = 134;
		    dl->color.b = 10;
            dl->color.exponent = 2;
		    dl->radius = (zm_cl_fireglow_radius_base.GetFloat() + zm_cl_fireglow_radius_random.GetFloat() * clrMod);
		    dl->die = fireData.flNextUpdate + frametime;
            dl->decay = zm_cl_fireglow_decay.GetFloat();

            break;
        }
    }
}

int CZMFireGlowSystem::AddFireEntity( FireGlowEnt_t* pEnt )
{
    if ( !pEnt || FindFireEntity( pEnt ) != m_vFireEntities.InvalidIndex() ) return m_vFireEntities.InvalidIndex();


    const int nMaxCount = GetMaxCount();

    if ( nMaxCount <= 0 )
        return m_vFireEntities.InvalidIndex();


    if ( m_vFireEntities.Count() >= nMaxCount )
    {
        m_vFireEntities.RemoveMultipleFromHead( m_vFireEntities.Count() - nMaxCount + 1 );
    }

    FireData_t data = { pEnt, FireGlowType_t::GLOW_GENERIC_FIRE, 0.0f };

    return m_vFireEntities.AddToTail( data );
}

bool CZMFireGlowSystem::RemoveFireEntity( FireGlowEnt_t* pEnt )
{
    int i = FindFireEntity( pEnt );
    if ( i != m_vFireEntities.InvalidIndex() )
    {
        m_vFireEntities.Remove( i );
        return true;
    }

    return false;
}

int CZMFireGlowSystem::GetMaxCount() const
{
    return zm_cl_fireglow_max.GetInt();
}

int CZMFireGlowSystem::FindFireEntity( FireGlowEnt_t* pEnt ) const
{
    FOR_EACH_VEC( m_vFireEntities, i )
    {
        if ( m_vFireEntities[i].pEnt == pEnt )
        {
            return i;
        }
    }

    return m_vFireEntities.InvalidIndex();
}

CZMFireGlowSystem g_ZMFireGlowSystem;
