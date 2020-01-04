#pragma once

#include "cbase.h"
#include "precipitation_shared.h"

#include "zmr/zmr_shareddefs.h"

class CZMBuildMenuBase;
class CZMManiMenuBase;


abstract_class C_ZMEntBaseSimple : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ZMEntBaseSimple, C_BaseAnimating )
    DECLARE_CLIENTCLASS()


    virtual bool ShouldDraw() OVERRIDE;
    virtual ShadowType_t ShadowCastType() OVERRIDE { return SHADOWS_NONE; };
#ifdef _DEBUG
    virtual void OnDataChanged( DataUpdateType_t type ) OVERRIDE;
#endif

    // Never go dormant, so we can render it even outside the world.
    // For now use dormant until we get this rolling. Disabled spawns/traps would show.
    //virtual bool IsDormant() OVERRIDE { return false; };
};

abstract_class C_ZMEntBaseUsable : public C_ZMEntBaseSimple
{
public:
	DECLARE_CLASS( C_ZMEntBaseUsable, C_ZMEntBaseSimple )
    DECLARE_CLIENTCLASS()


    C_ZMEntBaseUsable();


    virtual bool ShouldDraw() OVERRIDE;
    virtual int DrawModel( int flags ) OVERRIDE;

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

	void OnDataChanged( DataUpdateType_t type ) OVERRIDE;

    int GetZombieFlags() { return m_fZombieFlags; };
    const int* GetZombieCosts() { return m_iZombieCosts.Base(); };

	void SetMenu( CZMBuildMenuBase* pMenu ) { m_pBuildMenu = pMenu; };
	CZMBuildMenuBase* GetMenu() { return m_pBuildMenu; };

protected:
    virtual void InitSpriteMat() OVERRIDE;

private:
    CNetworkVar( int, m_fZombieFlags );
    CNetworkArray( int, m_iZombieCosts, ZMCLASS_MAX );

	CZMBuildMenuBase* m_pBuildMenu;
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

    inline const char* GetDescription() { return m_sDescription; };

	void OnDataChanged( DataUpdateType_t type ) OVERRIDE;

	void SetMenu( CZMManiMenuBase* pMenu ) { m_pManiMenu = pMenu; };
	CZMManiMenuBase* GetMenu() { return m_pManiMenu; };

protected:
    virtual void InitSpriteMat() OVERRIDE;

    char m_sDescription[ZM_MAX_MANI_DESC];

private:
    CNetworkVar( int, m_nCost );
    CNetworkVar( int, m_nTrapCost );

	CZMManiMenuBase* m_pManiMenu;
};

class C_ZMEntPrecipitation : public C_BaseEntity
{
public:
    DECLARE_CLASS( C_ZMEntPrecipitation, C_BaseEntity );
    DECLARE_CLIENTCLASS();

    C_ZMEntPrecipitation();
    ~C_ZMEntPrecipitation();

    virtual void PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;

    float GetDensity() const { return m_flDensity; }
    PrecipitationType_t GetPrecipitationType() const { return m_nPrecipType; }

protected:
    PrecipitationType_t m_nPrecipType;
    float m_flDensity;
};
