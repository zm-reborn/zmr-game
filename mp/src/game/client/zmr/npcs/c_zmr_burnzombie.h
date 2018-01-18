#pragma once


class C_ZMBaseZombie;

class C_ZMBurnZombie : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMBurnZombie, C_ZMBaseZombie )
    DECLARE_CLIENTCLASS()


    C_ZMBurnZombie();
    ~C_ZMBurnZombie();
};
