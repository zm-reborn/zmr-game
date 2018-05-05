#pragma once


class C_ZMBaseZombie;

class C_ZMHulk : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMHulk, C_ZMBaseZombie )
    DECLARE_CLIENTCLASS()


    C_ZMHulk();
    ~C_ZMHulk();


    virtual const char* GetZombieLocalization() const OVERRIDE { return "#ZMClassHulk"; }
};
