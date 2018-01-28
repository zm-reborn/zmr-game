#pragma once


class C_ZMBaseZombie;

class C_ZMFastZombie : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMFastZombie, C_ZMBaseZombie )
    DECLARE_CLIENTCLASS()


    C_ZMFastZombie();
    ~C_ZMFastZombie();
};
