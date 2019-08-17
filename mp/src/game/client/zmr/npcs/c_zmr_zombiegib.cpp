#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_zombiegib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define SKULL_MODEL     "models/gibs/hgibs.mdl"


int UTIL_CreateClientModel( const char* pszModel );


C_ZMZombieGib::C_ZMZombieGib()
{
    m_flRemoveTime = 0.0f;
}

C_ZMZombieGib::~C_ZMZombieGib()
{
}

void C_ZMZombieGib::FadeOut()
{
    const float flRemoveTime = 1.0f;

    if ( m_flRemoveTime == 0.0f )
    {
        SetRenderMode( RenderMode_t::kRenderTransAdd );
        m_flRemoveTime = gpGlobals->curtime + flRemoveTime;
    }

    
    float t = m_flRemoveTime - gpGlobals->curtime;
    

    float ratio = t / flRemoveTime;
    ratio = clamp( ratio, 0.0f, 1.0f );
    

    SetRenderColorA( 255 * ratio );
    
    if ( ratio <= 0.0f )
    {
        // ZMRTODO: This is most likely not safe. Find out.
        //
        Remove();
    }
    else
    {
        SetNextClientThink( CLIENT_THINK_ALWAYS );
    }
}

void C_ZMZombieGib::ClientThink()
{
    FadeOut();
}

C_ZMZombieGib* C_ZMZombieGib::Create( C_ZMBaseZombie* pZombie, ZMZombieGibType_t type )
{
    int modelIndex = UTIL_CreateClientModel( SKULL_MODEL );
    if ( modelIndex == -1 )
        return nullptr;


    C_ZMZombieGib* pGib = new C_ZMZombieGib();

    if ( !pGib->InitializeAsClientEntityByIndex( modelIndex, RENDER_GROUP_OPAQUE_ENTITY ) )
    {
        delete pGib;
        return nullptr;
    }


    pGib->SetAbsOrigin( pZombie->EyePosition() );


    Vector vel = Vector(
        random->RandomFloat( -100.0f, 100.0f ),
        random->RandomFloat( -100.0f, 100.0f ),
        random->RandomFloat( 200.0f, 300.0f ) );

    AngularImpulse angvel = AngularImpulse(
        random->RandomFloat( 100.0f, 200.0f ),
        random->RandomFloat( 100.0f, 300.0f ),
        0.0f );


    pGib->VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_SOLID, false );
    pGib->VPhysicsGetObject()->SetVelocity( &vel, &angvel );

    pGib->SetNextClientThink( gpGlobals->curtime + 6.0f );


    trace_t tr;
    UTIL_TraceLine(
        pZombie->GetAbsOrigin() + Vector( 0, 0, 4 ),
        pZombie->GetAbsOrigin() - Vector( 0, 0, 4 ),
        MASK_SOLID, pZombie, COLLISION_GROUP_NONE, &tr );
    UTIL_DecalTrace( &tr, "Blood" );

    return pGib;
}
