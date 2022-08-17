#include "cbase.h"
#include "basegrenade_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define PIPEBOMB_MODEL      "models/weapons/w_pipebomb.mdl"

#define RINGTONE_SND        "Weapon_Pipebomb_ZM.Ringtone"

class CZMProjectilePipebomb : public CBaseGrenade
{
public:
    DECLARE_CLASS( CZMProjectilePipebomb, CBaseGrenade );


    CZMProjectilePipebomb();
    ~CZMProjectilePipebomb();

    virtual void Spawn() OVERRIDE;
    virtual void Precache() OVERRIDE;

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
    PrecacheScriptSound( RINGTONE_SND );
}

void CZMProjectilePipebomb::Spawn()
{
    Precache();

    SetModel( PIPEBOMB_MODEL );


    VPhysicsInitNormal( SOLID_BBOX, 0, false );

    
    SetThink( &CZMProjectilePipebomb::PipebombThink );
    SetNextThink( gpGlobals->curtime + 3.0f );

    BaseClass::Spawn();

    EmitSound( RINGTONE_SND );
}

void CZMProjectilePipebomb::PipebombThink()
{
    Detonate();

    UTIL_Remove( this );

    StopSound( RINGTONE_SND );
}

bool CZMProjectilePipebomb::IsCombatItem() const
{
    return true; // Return true to disable swatting of this object.
}
