#pragma once


class C_ZMBaseZombie;

class C_ZMBanshee : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMBanshee, C_ZMBaseZombie )
    DECLARE_CLIENTCLASS()


    C_ZMBanshee();
    ~C_ZMBanshee();


    virtual const char* GetZombieLocalization() const override { return "#ZMClassBanshee"; }


    virtual void AttackSound() override;

    virtual void HandleAnimEvent( animevent_t* pEvent ) override;
};
