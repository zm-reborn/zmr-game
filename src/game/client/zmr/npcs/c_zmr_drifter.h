#pragma once


class C_ZMBaseZombie;

class C_ZMDrifter : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMDrifter, C_ZMBaseZombie )
    DECLARE_CLIENTCLASS()


    C_ZMDrifter();
    ~C_ZMDrifter();


    virtual const char* GetZombieLocalization() const override { return "#ZMClassDrifter"; }

protected:
    virtual bool        IsAffectedByEvent( HappyZombieEvent_t iEvent ) const override;
    virtual const char* GetEventHatModel( HappyZombieEvent_t iEvent ) const override;
};
