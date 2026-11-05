#include "cbase.h"

#include "zmr_baseprojectile.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define PIPEBOMB_MODEL      "models/weapons/w_pipebomb.mdl"

#define RINGTONE_SND        "Weapon_Pipebomb_ZM.Ringtone"

class CZMProjectilePipebomb : public CZMBaseProjectile
{
public:
    DECLARE_CLASS( CZMProjectilePipebomb, CZMBaseProjectile );


    CZMProjectilePipebomb();
    ~CZMProjectilePipebomb();

    virtual void Spawn() override;
    virtual void Precache() override;

    virtual void OnThrow() override;

    virtual bool IsCombatItem() const override;

    virtual void Detonate() override;
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

void CZMProjectilePipebomb::OnThrow()
{
    VPhysicsInitNormal( SOLID_BBOX, 0, false );
}

void CZMProjectilePipebomb::Spawn()
{
    Precache();

    SetModel( PIPEBOMB_MODEL );

    OnThrow();

    BaseClass::Spawn();

    EmitSound( RINGTONE_SND );
}

void CZMProjectilePipebomb::Detonate()
{
    StopSound( RINGTONE_SND );

    BaseClass::Detonate();
}

bool CZMProjectilePipebomb::IsCombatItem() const
{
    return true; // Return true to disable swatting of this object.
}
