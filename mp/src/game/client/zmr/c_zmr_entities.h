#pragma once

#include "cbase.h"


abstract_class C_ZMEntBaseSimple : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ZMEntBaseSimple, C_BaseAnimating )
    DECLARE_CLIENTCLASS()


    virtual bool ShouldDraw() OVERRIDE;
    virtual ShadowType_t ShadowCastType() OVERRIDE { return SHADOWS_NONE; };

    // Never go dormant, so we can render it even outside the world.
    virtual bool IsDormant() OVERRIDE { return false; };
};

abstract_class C_ZMEntBaseUsable : public C_ZMEntBaseSimple
{
public:
	DECLARE_CLASS( C_ZMEntBaseUsable, C_ZMEntBaseSimple )
    DECLARE_CLIENTCLASS()


    C_ZMEntBaseUsable();


    virtual bool ShouldDraw() OVERRIDE;
    virtual int DrawModel( int ) OVERRIDE;

protected:
    virtual void InitSpriteMat() {};

    CMaterialReference m_SpriteMat;
};

class C_ZMEntZombieSpawn : public C_ZMEntBaseUsable
{
public:
	DECLARE_CLASS( C_ZMEntZombieSpawn, C_ZMEntBaseUsable )
    DECLARE_CLIENTCLASS()
    DECLARE_DATADESC()
    

    C_ZMEntZombieSpawn();


    void Precache() OVERRIDE;

    int GetZombieFlags() { return m_fZombieFlags; };

protected:
    virtual void InitSpriteMat() OVERRIDE;

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


    void Precache() OVERRIDE;

    inline int GetCost() { return m_nCost; };
    inline int GetTrapCost() { return m_nTrapCost; };

protected:
    virtual void InitSpriteMat() OVERRIDE;

private:
    CNetworkVar( int, m_nCost );
    CNetworkVar( int, m_nTrapCost );
};