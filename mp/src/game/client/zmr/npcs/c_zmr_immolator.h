#pragma once


class C_ZMBaseZombie;

class C_ZMImmolator : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMImmolator, C_ZMBaseZombie )
    DECLARE_CLIENTCLASS()


    C_ZMImmolator();
    ~C_ZMImmolator();


    virtual const char* GetZombieLocalization() const OVERRIDE { return "#ZMClassImmolator"; }
};
