#pragma once



#include "zmr_zombiebase.h"

class CZMShambler : public CZMBaseZombie
{
public:
    typedef CZMShambler ThisClass;
    typedef CZMBaseZombie BaseClass;
    //DECLARE_CLASS( CZMShambler, CZMBaseZombie );
    DECLARE_SERVERCLASS();

    CZMShambler();
    ~CZMShambler();

    virtual void Precache() OVERRIDE;
    virtual void Spawn() OVERRIDE;

    virtual void    HandleAnimEvent( animevent_t* pEvent ) OVERRIDE;

    virtual bool        CanBreakObject( CBaseEntity* pEnt, bool bSwat = false ) const OVERRIDE;
    virtual bool        CanSwatPhysicsObjects() const OVERRIDE { return true; }
    virtual Activity    GetSwatActivity( CBaseEntity* pEnt, bool bBreak = true ) const OVERRIDE;


    virtual bool ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const OVERRIDE;


    virtual void AlertSound() OVERRIDE;
    virtual void AttackSound() OVERRIDE;
    virtual void DeathSound() OVERRIDE;
    virtual void FootstepSound( bool bRightFoot = false ) OVERRIDE;
    virtual void FootscuffSound( bool bRightFoot = false ) OVERRIDE;


    static Activity ACT_ZOM_SWATLEFTMID;
    static Activity ACT_ZOM_SWATLEFTLOW;
    static Activity ACT_ZOM_SWATRIGHTMID;
    static Activity ACT_ZOM_SWATRIGHTLOW;
};
