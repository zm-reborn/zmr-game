#pragma once

#include "c_zmr_zombiebase.h"

class C_ZMShambler : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMShambler, C_ZMBaseZombie )
	DECLARE_CLIENTCLASS()


    C_ZMShambler();
    ~C_ZMShambler();

    virtual void FootstepSound( bool bRightFoot = false ) OVERRIDE;
    virtual void FootscuffSound( bool bRightFoot = false ) OVERRIDE;
    virtual void AttackSound() OVERRIDE;

    virtual const char* GetZombieLocalization() const OVERRIDE { return "#ZMClassShambler"; }


    LocalFlexController_t GetFlexControllerNumByName( const char* pszName );
    void SetFlexWeightSafe( const char* pszName, float value );

protected:
    virtual bool        CreateEventAccessories() OVERRIDE;
    virtual bool        IsAffectedByEvent( HappyZombieEvent_t iEvent ) const OVERRIDE;
    virtual const char* GetEventHatModel( HappyZombieEvent_t iEvent ) const OVERRIDE;
    void                MakeHappy();
};
