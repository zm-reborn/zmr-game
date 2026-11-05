#pragma once


#include "triggers.h"
#include "physobj.h"

//#include "zmr_player.h"
#//include "npcs/zmr_zombiebase.h"
#include "zmr_shareddefs.h"



class CZMPlayer;
class CZMBaseZombie;
class CZMEntSpawnNode;
class CZMEntRallyPoint;


/*
    func_win
*/
class CZMEntWin : public CServerOnlyPointEntity
{
public:
	DECLARE_CLASS( CZMEntWin, CServerOnlyPointEntity )
    DECLARE_DATADESC()


    static void OnRoundEnd( ZMRoundEndReason_t reason );

    void Spawn() override;

    void InputHumanWin( inputdata_t &inputdata );
    void InputHumanLose( inputdata_t &inputdata );


    COutputEvent m_OnWin;
    COutputEvent m_OnLose;
    COutputEvent m_OnSubmit;
};

/*
    ZMRTODO: Make networking these ents lighter?

    Base Simple
*/
abstract_class CZMEntBaseSimple : public CBaseAnimating
{
public:
	DECLARE_CLASS( CZMEntBaseSimple, CBaseAnimating )
    DECLARE_SERVERCLASS()

    virtual int ShouldTransmit( const CCheckTransmitInfo* pInfo ) override;
    virtual int UpdateTransmitState() override;
};


/*
    BaseUsable
*/
abstract_class CZMEntBaseUsable : public CZMEntBaseSimple
{
public:
	DECLARE_CLASS( CZMEntBaseUsable, CZMEntBaseSimple )
    DECLARE_SERVERCLASS()
    DECLARE_DATADESC()

    CZMEntBaseUsable();

    void Spawn() override;


    void UpdateUsable();

    void InputToggle( inputdata_t &inputdata );
    void InputHide( inputdata_t &inputdata );
    void InputUnhide( inputdata_t &inputdata );


    bool IsActive() const { return m_bActive; }

    virtual void SendMenuUpdate() = 0;

private:
    bool m_bActive;
};


/*
    Zombie spawn
*/
struct queue_info_t
{
    ZombieClass_t m_zclass;
    uint8 m_nCount;
    int m_iSpawnerIndex;
};

class CZMEntSpawnNode;
class CZMEntRallyPoint;

class CZMEntZombieSpawn : public CZMEntBaseUsable
{
public:
	DECLARE_CLASS( CZMEntZombieSpawn, CZMEntBaseUsable )
    DECLARE_SERVERCLASS()
    DECLARE_DATADESC()

    CZMEntZombieSpawn();


    void Spawn() override;
    void Precache() override;

    bool AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID ) override;

    void InputToggle( inputdata_t &inputdata );
    void InputHide( inputdata_t &inputdata );
    void InputUnhide( inputdata_t &inputdata );
    void InputSetZombieFlags( inputdata_t &inputdata ) { m_fZombieFlags = inputdata.value.Int(); }
    void InputAddZombieFlags( inputdata_t &inputdata ) { m_fZombieFlags |= inputdata.value.Int(); }
    void InputRemoveZombieFlags( inputdata_t &inputdata ) { m_fZombieFlags &= ~inputdata.value.Int(); }
    void InputEnableZombie0( inputdata_t& inputdata ) { m_fZombieFlags |= 1 << 0; }
    void InputEnableZombie1( inputdata_t& inputdata ) { m_fZombieFlags |= 1 << 1; }
    void InputEnableZombie2( inputdata_t& inputdata ) { m_fZombieFlags |= 1 << 2; }
    void InputEnableZombie3( inputdata_t& inputdata ) { m_fZombieFlags |= 1 << 3; }
    void InputEnableZombie4( inputdata_t& inputdata ) { m_fZombieFlags |= 1 << 4; }
    void InputDisableZombie0( inputdata_t& inputdata ) { m_fZombieFlags &= ~(1 << 0); }
    void InputDisableZombie1( inputdata_t& inputdata ) { m_fZombieFlags &= ~(1 << 1); }
    void InputDisableZombie2( inputdata_t& inputdata ) { m_fZombieFlags &= ~(1 << 2); }
    void InputDisableZombie3( inputdata_t& inputdata ) { m_fZombieFlags &= ~(1 << 3); }
    void InputDisableZombie4( inputdata_t& inputdata ) { m_fZombieFlags &= ~(1 << 4); }
    void InputSetZombieCost0( inputdata_t &inputdata ) { m_iZombieCosts.Set( 0, inputdata.value.Int() ); }
    void InputSetZombieCost1( inputdata_t &inputdata ) { m_iZombieCosts.Set( 1, inputdata.value.Int() ); }
    void InputSetZombieCost2( inputdata_t &inputdata ) { m_iZombieCosts.Set( 2, inputdata.value.Int() ); }
    void InputSetZombieCost3( inputdata_t &inputdata ) { m_iZombieCosts.Set( 3, inputdata.value.Int() ); }
    void InputSetZombieCost4( inputdata_t &inputdata ) { m_iZombieCosts.Set( 4, inputdata.value.Int() ); }


    void SpawnThink();


    CZMBaseZombie* CreateZombie( ZombieClass_t zclass, CZMPlayer* pSpawner );

    bool QueueUnit( CZMPlayer* pPlayer, ZombieClass_t zclass, int amount );
    void QueueClear( int inamount = -1, int inpos = -1 );
    bool CanSpawn( ZombieClass_t zclass ) const;

    void SendMenuUpdate() override;

    void SetRallyPoint( const Vector& pos );


    inline int GetZombieFlags() const { return m_fZombieFlags; }
    const char* GetZombieModelGroup() const { return STRING( m_sZombieModelGroup ); }

private:
    void StartSpawning();
    void StopSpawning();

    void InitSpawnNodes();
    CBaseEntity* FindSpawnPoint( CZMBaseZombie* pZombie, Vector& output, QAngle& outang );

    void SetNextSpawnThink();
    float GetSpawnDelay() const;

    CUtlVector<queue_info_t> m_vSpawnQueue;

    CUtlVector<CHandle<CZMEntSpawnNode>> m_vSpawnNodes;
    CHandle<CZMEntRallyPoint> m_hRallyPoint;


    //int m_nSpawnQueueCapacity;
    string_t m_sZombieModelGroup;
    string_t m_sRallyName;
    string_t m_sFirstNodeName;

    CNetworkArray( int, m_iZombieCosts, ZMCLASS_MAX );
    COutputEHANDLE m_OnSpawnZMClass[ZMCLASS_MAX];

    COutputEHANDLE m_OnSpawnNPC;

    //int m_fZombieFlags;
    CNetworkVar( int, m_fZombieFlags );

    int m_iZombieNodeCounter;
};


/*
    Spawnnode
*/
class CZMEntSpawnNode : public CZMEntBaseSimple
{
public:
	DECLARE_CLASS( CZMEntSpawnNode, CZMEntBaseSimple )
    DECLARE_DATADESC()

    CZMEntSpawnNode();

    void Spawn() override;
    void Precache() override;

    string_t m_sNextNodeName;

    // This is fired (with its parameter) from info_zombiespawn right now
    COutputEvent m_OnSpawnNPC;
};


/*
    Rallypoint
*/
class CZMEntRallyPoint : public CZMEntBaseSimple
{
public:
	DECLARE_CLASS( CZMEntRallyPoint, CZMEntBaseSimple )
    DECLARE_DATADESC()

    void Spawn() override;
    void Precache() override;
};


/*
    Trap trigger
*/
class CZMEntManipulateTrigger : public CZMEntBaseSimple
{
public:
	DECLARE_CLASS( CZMEntManipulateTrigger, CZMEntBaseSimple )
    //DECLARE_SERVERCLASS()
    DECLARE_DATADESC()

    CZMEntManipulateTrigger();
    ~CZMEntManipulateTrigger();


    void Spawn() override;
    void Precache() override;


    void ScanThink();
};


/*
    Ambush trigger
*/
class CZMEntAmbushTrigger : public CZMEntBaseSimple
{
public:
	DECLARE_CLASS( CZMEntAmbushTrigger, CZMEntBaseSimple )
    //DECLARE_SERVERCLASS()
    DECLARE_DATADESC()

    CZMEntAmbushTrigger();
    ~CZMEntAmbushTrigger();


    void Spawn() override;
    void Precache() override;


    void SetAmbushZombies( int count );
    void RemoveZombieFromAmbush();

    void Trigger( CBaseEntity* pActivator );

    void ScanThink();

private:
    int m_nAmbushZombies;
};


/*
    ZMRTODO: Use string tables.

    Trap
*/
class CZMEntManipulateTrigger;

class CZMEntManipulate : public CZMEntBaseUsable
{
public:
	DECLARE_CLASS( CZMEntManipulate, CZMEntBaseUsable )
    DECLARE_SERVERCLASS()
    DECLARE_DATADESC()

    CZMEntManipulate();
    ~CZMEntManipulate();


    void Spawn() override;
    void Precache() override;

    bool AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID ) override;

    void InputToggle( inputdata_t &inputdata );
    void InputHide( inputdata_t &inputdata );
    void InputUnhide( inputdata_t &inputdata );
    void InputPress( inputdata_t &inputdata );
	void InputSetDescription( inputdata_t &inputdata ) { m_sDescription = inputdata.value.StringID(); }
	void InputSetTrapCost( inputdata_t &inputdata );
	void InputSetCost( inputdata_t &inputdata );


    void Trigger( CBaseEntity* pActivator );

    void CreateTrigger( const Vector& pos );
    void RemoveTriggers();
    void RemoveTrigger( CZMEntManipulateTrigger* pTrigger );
    inline int GetTriggerCount() const { return m_vTriggers.Count(); }

    COutputEvent m_OnPressed;

	void SendMenuUpdate() override;


    inline const char* GetDescription() const { return m_sDescription.Get().ToCStr(); }
    inline int GetTrapCost() const { return m_nTrapCost; }
    inline int GetCost() const { return m_nCost; }

private:
    CNetworkVar( string_t, m_sDescription );

    CNetworkVar( int, m_nCost );
    CNetworkVar( int, m_nTrapCost );


    bool m_bRemoveOnTrigger;

    CUtlVector<CHandle<CZMEntManipulateTrigger>> m_vTriggers;
};


/*
    Loadout ent
*/
class CZMEntLoadout : public CServerOnlyPointEntity
{
public:
    enum LoadoutWeapon_t
    {
        LOADOUTWEAPON_START = 0,

        LOADOUTWEAPON_PISTOL = 0,
        LOADOUTWEAPON_SHOTGUN,
        LOADOUTWEAPON_RIFLE,
        LOADOUTWEAPON_MAC10,
        LOADOUTWEAPON_MOLOTOV,
        LOADOUTWEAPON_SLEDGE,
        LOADOUTWEAPON_IMPROVISED,
        LOADOUTWEAPON_REVOLVER,

        LOADOUTWEAPON_MAX
    };

    enum LoadoutMethod_t
    {
        LOADOUTMETHOD_INVALID = 0,

        LOADOUTMETHOD_CATEGORY,
        LOADOUTMETHOD_RANDOM,

        LOADOUTMETHOD_MAX
    };

    enum LoadoutLocation_t
    {
        LOADOUTLOCATION_START = 0,

        LOADOUTLOCATION_MELEE = 0,
        LOADOUTLOCATION_PISTOL,
        LOADOUTLOCATION_LARGE,
        LOADOUTLOCATION_EQUIPMENT,

        LOADOUTLOCATION_MAX
    };

	DECLARE_CLASS( CZMEntLoadout, CPointEntity )
    DECLARE_DATADESC()

    CZMEntLoadout();
    ~CZMEntLoadout();


    void Spawn() override;



    void Reset();
    void DistributeToPlayer( CZMPlayer* pPlayer );
    

    inline LoadoutMethod_t GetMethod() const;

private:
    void GiveWeapon( CZMPlayer* pPlayer, LoadoutWeapon_t loadout_wep );


    LoadoutMethod_t m_iMethod;

    int m_iCounts[LOADOUTWEAPON_MAX];

    int m_iCurRandom[LOADOUTWEAPON_MAX];
    CUtlVector<LoadoutWeapon_t> m_vCurCat[LOADOUTLOCATION_MAX];
};

/*
    Block hidden create
*/
class CZMEntTriggerBlockHidden : public CBaseTrigger
{
public:
    DECLARE_CLASS( CZMEntTriggerBlockHidden, CBaseTrigger );
    DECLARE_DATADESC();


    CZMEntTriggerBlockHidden();
    ~CZMEntTriggerBlockHidden();

    void Spawn() override;


    inline bool IsActive() const { return m_bActive; }


    void InputToggle( inputdata_t &inputdata );
    void InputEnable( inputdata_t &inputdata );
    void InputDisable( inputdata_t &inputdata );

private:
    bool m_bActive;
};

/*
    Block phys explosion
*/
class CZMEntTriggerBlockPhysExp : public CBaseTrigger
{
public:
    DECLARE_CLASS( CZMEntTriggerBlockPhysExp, CBaseTrigger );
    DECLARE_DATADESC();


    CZMEntTriggerBlockPhysExp();
    ~CZMEntTriggerBlockPhysExp();

    void Spawn() override;


    inline bool IsActive() const { return m_bActive; }


    void InputToggle( inputdata_t &inputdata );
    void InputEnable( inputdata_t &inputdata );
    void InputDisable( inputdata_t &inputdata );

private:
    bool m_bActive;
};

/*
    Phys explosion
*/
class CEnvSpark;

class CZMPhysExplosion : public CServerOnlyPointEntity
{
public:
    DECLARE_CLASS( CZMPhysExplosion, CServerOnlyPointEntity );
    DECLARE_DATADESC();


    CZMPhysExplosion();
    ~CZMPhysExplosion();


    static CZMPhysExplosion* CreatePhysExplosion( const Vector& pos, float delay, float magnitude, float radius );

    void Spawn() override;
    void Precache() override;


    float GetMagnitude() const { return m_flMagnitude; }
    float GetRadius() const { return m_flRadius; }

    void SetMagnitude( float val ) { m_flMagnitude = val; }
    void SetRadius( float val ) { m_flRadius = val; }

private:
    void DelayThink();
    void CreateEffects( float delay );
    void Push();


    void DelayedExplode( float delay );

    CHandle<CEnvSpark> m_hSpark;


    float m_flMagnitude;
    float m_flRadius;
};

/*
    Spawn points
*/
class CZMEntSpawnPoint : public CServerOnlyPointEntity
{
public:
    DECLARE_CLASS( CZMEntSpawnPoint, CServerOnlyPointEntity );
    DECLARE_DATADESC();


    CZMEntSpawnPoint();


    void InputEnable( inputdata_t& inputData );
    void InputDisable( inputdata_t& inputData );
    void InputToggle( inputdata_t& inputData );

    inline bool IsEnabled() const { return m_bIsEnabled; }
    inline void SetEnabled( bool b ) { m_bIsEnabled = b; }

private:
    bool m_bIsEnabled;
};

/*
    ZM specific fog controller
*/
class CZMEntFogController : public CFogController
{
public:
    DECLARE_CLASS( CZMEntFogController, CFogController );
    DECLARE_DATADESC();

    CZMEntFogController();

    static bool IsEnabled();

    void InitFog();


    float m_flSkyboxFarZ;

    // If we're game created, we can't use the cvars.
    void SetGameCreated() { m_bNeedsInit = true; }
    bool IsGameCreated() const { return m_bNeedsInit; }

private:
    bool m_bNeedsInit;
};

/*
    Brush spawn volume
*/
class CZMEntTriggerSpawnVolume : public CBaseTrigger
{
public:
    DECLARE_CLASS( CZMEntTriggerSpawnVolume, CBaseTrigger );
    DECLARE_DATADESC();


    CZMEntTriggerSpawnVolume();
    ~CZMEntTriggerSpawnVolume();

    void Spawn() override;


    static void GetPositionWithin( const CBaseEntity* pEnt, Vector& pos );
    void GetPositionWithin( Vector& pos ) const;


    inline bool IsActive() const { return m_bActive; }


    void InputToggle( inputdata_t &inputdata );
    void InputEnable( inputdata_t &inputdata );
    void InputDisable( inputdata_t &inputdata );

private:
    bool m_bActive;

    // This is fired (with its parameter) from info_zombiespawn right now
    COutputEvent m_OnSpawnNPC;
};
