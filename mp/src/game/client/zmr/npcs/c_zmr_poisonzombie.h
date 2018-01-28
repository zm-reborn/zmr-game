#pragma once


class C_ZMBaseZombie;

class C_ZMPoisonZombie : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMPoisonZombie, C_ZMBaseZombie )
    DECLARE_CLIENTCLASS()


    C_ZMPoisonZombie();
    ~C_ZMPoisonZombie();
};
