#pragma once

#include "cbase.h"


class C_ZMEntBaseUsable : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ZMEntBaseUsable, C_BaseAnimating )
    DECLARE_CLIENTCLASS()
    DECLARE_DATADESC()


    // ZMRTODO: Should draw doesn't seem to work?
    //virtual bool ShouldDraw() OVERRIDE;
    virtual ShadowType_t ShadowCastType() OVERRIDE { return SHADOWS_NONE; };
    virtual int DrawModel( int ) OVERRIDE;
};

class C_ZMEntBaseSimple : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ZMEntBaseSimple, C_BaseAnimating )
    DECLARE_CLIENTCLASS()
    DECLARE_DATADESC()

    
    virtual ShadowType_t ShadowCastType() OVERRIDE { return SHADOWS_NONE; };
    virtual int DrawModel( int ) OVERRIDE;
};

class C_ZMEntZombieSpawn : public C_ZMEntBaseUsable
{
public:
	DECLARE_CLASS( C_ZMEntZombieSpawn, C_ZMEntBaseUsable )
    DECLARE_CLIENTCLASS()
    DECLARE_DATADESC()
    

    int GetZombieFlags() { return m_fZombieFlags; };
private:
    CNetworkVar( int, m_fZombieFlags );
};

class C_ZMEntManipulate : public C_ZMEntBaseUsable
{
public:
	DECLARE_CLASS( C_ZMEntManipulate, C_ZMEntBaseUsable )
    DECLARE_CLIENTCLASS()
    DECLARE_DATADESC()

    C_ZMEntManipulate();


    inline int GetCost() { return m_nCost; };
    inline int GetTrapCost() { return m_nTrapCost; };
private:
    CNetworkVar( int, m_nCost );
    CNetworkVar( int, m_nTrapCost );
};