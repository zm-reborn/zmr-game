#pragma once


class C_ZMBaseZombie;

class C_ZMHulk : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMHulk, C_ZMBaseZombie )
    DECLARE_CLIENTCLASS()


    C_ZMHulk();
    ~C_ZMHulk();

    virtual void FootstepSound( bool bRightFoot = false ) override;
    virtual void AttackSound() override;

    virtual const char* GetZombieLocalization() const override { return "#ZMClassHulk"; }


    // Shared
    virtual bool    CanBePenetrated() const override;


protected:
    virtual bool        IsAffectedByEvent( HappyZombieEvent_t iEvent ) const override;
    virtual const char* GetEventHatModel( HappyZombieEvent_t iEvent ) const override;
};
