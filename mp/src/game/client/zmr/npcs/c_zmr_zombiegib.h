#pragma once

class C_ZMBaseZombie;

class C_ZMZombieGib : public C_BaseAnimating
{
public:
    DECLARE_CLASS( C_ZMZombieGib, C_BaseAnimating );


    C_ZMZombieGib();
    ~C_ZMZombieGib();


    virtual void ClientThink() OVERRIDE;

    void FadeOut();


    static C_ZMZombieGib* Create( C_ZMBaseZombie* pZombie, ZMZombieGibType_t type );


private:
    float m_flRemoveTime;
};
