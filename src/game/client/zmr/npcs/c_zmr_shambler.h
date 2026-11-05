#pragma once

#include "c_zmr_zombiebase.h"

class C_ZMShambler : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMShambler, C_ZMBaseZombie )
	DECLARE_CLIENTCLASS()


    C_ZMShambler();
    ~C_ZMShambler();

    virtual void FootstepSound( bool bRightFoot = false ) override;
    virtual void FootscuffSound( bool bRightFoot = false ) override;
    virtual void AttackSound() override;

    virtual const char* GetZombieLocalization() const override { return "#ZMClassShambler"; }


    LocalFlexController_t GetFlexControllerNumByName( const char* pszName );
    void SetFlexWeightSafe( const char* pszName, float value );

protected:
    virtual bool        CreateEventAccessories() override;
    virtual bool        IsAffectedByEvent( HappyZombieEvent_t iEvent ) const override;
    virtual const char* GetEventHatModel( HappyZombieEvent_t iEvent ) const override;
    void                MakeHappy();
};
