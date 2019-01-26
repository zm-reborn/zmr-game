#pragma once


class C_ZMBaseZombie;

class C_ZMHulk : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMHulk, C_ZMBaseZombie )
    DECLARE_CLIENTCLASS()


    C_ZMHulk();
    ~C_ZMHulk();

    virtual void FootstepSound( bool bRightFoot = false ) OVERRIDE;
    virtual void AttackSound() OVERRIDE;

    virtual const char* GetZombieLocalization() const OVERRIDE { return "#ZMClassHulk"; }


    // Shared
    virtual bool    CanBePenetrated() const OVERRIDE;


protected:
    virtual bool        IsAffectedByEvent( HappyZombieEvent_t iEvent ) const OVERRIDE;
    virtual const char* GetEventHatModel( HappyZombieEvent_t iEvent ) const OVERRIDE;
};
