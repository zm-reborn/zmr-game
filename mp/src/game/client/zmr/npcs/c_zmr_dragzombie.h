#pragma once


class C_ZMBaseZombie;

class C_ZMDragZombie : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMDragZombie, C_ZMBaseZombie )
    DECLARE_CLIENTCLASS()


    C_ZMDragZombie();
    ~C_ZMDragZombie();


    virtual const char* GetZombieLocalization() OVERRIDE { return "#ZMClassDrifter"; };
};
