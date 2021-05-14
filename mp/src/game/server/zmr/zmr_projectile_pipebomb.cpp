#include "cbase.h"
#include "basegrenade_shared.h"


#define PIPEBOMB_MODEL      "models/weapons/w_pipebomb.mdl"

class CZMProjectilePipebomb : public CBaseGrenade
{
public:
    DECLARE_CLASS( CZMProjectilePipebomb, CBaseGrenade );


    CZMProjectilePipebomb();
    ~CZMProjectilePipebomb();

    void Spawn() OVERRIDE;
    void Precache() OVERRIDE;

    void PipebombThink();

    virtual bool IsCombatItem() const OVERRIDE;
};

LINK_ENTITY_TO_CLASS( grenade_pipebomb, CZMProjectilePipebomb );


CZMProjectilePipebomb::CZMProjectilePipebomb()
{
}

CZMProjectilePipebomb::~CZMProjectilePipebomb()
{
}

void CZMProjectilePipebomb::Precache()
{
    PrecacheModel( PIPEBOMB_MODEL );
}

void CZMProjectilePipebomb::Spawn()
{
    Precache();

    SetModel( PIPEBOMB_MODEL );


    VPhysicsInitNormal( SOLID_BBOX, 0, false );

    
    SetThink( &CZMProjectilePipebomb::PipebombThink );
    SetNextThink( gpGlobals->curtime + 3.0f );

    BaseClass::Spawn();
}

void CZMProjectilePipebomb::PipebombThink()
{
    Detonate();

    UTIL_Remove( this );
}

bool CZMProjectilePipebomb::IsCombatItem() const
{
    return true; // Return true to disable swatting of this object.
}
