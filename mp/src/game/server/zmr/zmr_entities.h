#pragma once


class CZMEntRallyPoint;
class CZMEntSpawnNode;
class CZMPlayer;
class CZMBaseZombie;

#include "triggers.h"
#include "physobj.h"

#include "zmr_player.h"
#include "npcs/zmr_zombiebase.h"
#include "zmr/zmr_shareddefs.h"


/*
    ZMRTODO: Make networking these ents lighter?

    Base Simple
*/
abstract_class CZMEntBaseSimple : public CBaseAnimating
{
public:
	DECLARE_CLASS( CZMEntBaseSimple, CBaseAnimating )
    DECLARE_SERVERCLASS()
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


    void Spawn( void ) OVERRIDE;


    void UpdateUsable();

    void InputToggle( inputdata_t &inputdata );
    void InputHide( inputdata_t &inputdata );
    void InputUnhide( inputdata_t &inputdata );


    bool IsActive() { return m_bActive; };

private:
    bool m_bActive;
};


/*
    Zombie spawn
*/
struct queue_info_t
{
    ZombieClass_t m_zclass;
    int m_iSpawnerIndex;
};

class CZMEntZombieSpawn : public CZMEntBaseUsable
{
public:
	DECLARE_CLASS( CZMEntZombieSpawn, CZMEntBaseUsable )
    DECLARE_SERVERCLASS()
    DECLARE_DATADESC()

    CZMEntZombieSpawn();


    void Spawn( void ) OVERRIDE;
    void Precache( void ) OVERRIDE;

    void InputToggle( inputdata_t &inputdata );
    void InputHide( inputdata_t &inputdata );
    void InputUnhide( inputdata_t &inputdata );


    void SpawnThink();

    bool QueueUnit( CZMPlayer*, ZombieClass_t, int );
    void QueueClear( int = -1, int = -1 );
    bool CanSpawn( ZombieClass_t );

    void SendMenuUpdate();

    void SetRallyPoint( const Vector& );


    inline int GetZombieFlags() { return m_fZombieFlags; };

private:
    void StartSpawning();
    void StopSpawning();

    bool CreateZombie( ZombieClass_t );
    bool FindSpawnPoint( CZMBaseZombie*, Vector&, QAngle& );

    void SetNextSpawnThink();

    CUtlVector<queue_info_t> m_vSpawnQueue;

    CUtlVector<CZMEntSpawnNode*> m_vSpawnNodes;
    CZMEntRallyPoint* m_pRallyPoint;


    //int m_nSpawnQueueCapacity;
    string_t m_sRallyName;
    string_t m_sFirstNodeName;

    //int m_fZombieFlags;
    CNetworkVar( int, m_fZombieFlags );
};


/*
    Spawnnode
*/
class CZMEntSpawnNode : public CZMEntBaseSimple
{
public:
	DECLARE_CLASS( CZMEntSpawnNode, CZMEntBaseSimple )
    DECLARE_DATADESC()

    void Spawn( void ) OVERRIDE;
    void Precache( void ) OVERRIDE;

    string_t m_sNextNodeName;
};


/*
    Rallypoint
*/
class CZMEntRallyPoint : public CZMEntBaseSimple
{
public:
	DECLARE_CLASS( CZMEntRallyPoint, CZMEntBaseSimple )
    DECLARE_DATADESC()

    void Spawn( void ) OVERRIDE;
    void Precache( void ) OVERRIDE;
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


    void Spawn( void ) OVERRIDE;
    void Precache( void ) OVERRIDE;


    void ScanThink( void );
};

/*
    ZMRTODO: Use string tables.

    Trap
*/
class CZMEntManipulate : public CZMEntBaseUsable
{
public:
	DECLARE_CLASS( CZMEntManipulate, CZMEntBaseUsable )
    DECLARE_SERVERCLASS()
    DECLARE_DATADESC()

    CZMEntManipulate();
    ~CZMEntManipulate();


    void Spawn( void ) OVERRIDE;
    void Precache( void ) OVERRIDE;

    void InputToggle( inputdata_t &inputdata );
    void InputHide( inputdata_t &inputdata );
    void InputUnhide( inputdata_t &inputdata );
    //void InputTrigger( inputdata_t &inputdata );


    void Trigger( CBaseEntity* pActivator );

    void CreateTrigger( const Vector& pos );
    void RemoveTriggers();
    void RemoveTrigger( CZMEntManipulateTrigger* );
    inline int GetTriggerCount() { return m_vTriggers.Count(); };

    COutputEvent m_OnPressed;


    inline const char* GetDescription() { return m_sDescription.ToCStr(); };
    inline int GetTrapCost() { return m_nTrapCost; };
    inline int GetCost() { return m_nCost; };

private:
    string_t m_sDescription;

    CNetworkVar( int, m_nCost );
    CNetworkVar( int, m_nTrapCost );


    bool m_bRemoveOnTrigger;

    CUtlVector<CZMEntManipulateTrigger*> m_vTriggers;
};


/*
    Loadout ent
*/
class CZMEntLoadout : public CPointEntity
{
public:
    enum
    {
        LO_PISTOL = 0,
        LO_SHOTGUN,
        LO_RIFLE,
        LO_MAC10,
        LO_MOLOTOV,
        LO_SLEDGE,
        LO_IMPROVISED,
        LO_REVOLVER,

        LO_MAX
    };

    enum LoadOutMethod_t
    {
        LOMETHOD_CATEGORY = 0,
        LOMETHOD_RANDOM,
    };

    enum
    {
        LOCAT_MELEE = 0,
        LOCAT_PISTOL,
        LOCAT_LARGE,
        LOCAT_EQUIPMENT,

        LOCAT_MAX
    };

	DECLARE_CLASS( CZMEntLoadout, CPointEntity )
    DECLARE_DATADESC()

    CZMEntLoadout();
    ~CZMEntLoadout();


    void Spawn( void ) OVERRIDE;



    void Reset();
    void DistributeToPlayer( CZMPlayer* );
    

    inline LoadOutMethod_t GetMethod() { return m_iMethod; };

private:
    void GiveWeapon( CZMPlayer*, int );


    LoadOutMethod_t m_iMethod;

    int m_iCounts[LO_MAX];

    int m_iCurRandom[LO_MAX];
    CUtlVector<int> m_vCurCat[LOCAT_MAX];
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

    void Spawn( void ) OVERRIDE;


    inline bool IsActive() { return m_bActive; };


    void InputToggle( inputdata_t &inputdata );
    void InputEnable( inputdata_t &inputdata );
    void InputDisable( inputdata_t &inputdata );

private:
    bool m_bActive;
};

/*
    Phys explosion
*/
class CZMPhysExplosion : public CPhysExplosion
{
public:
    DECLARE_CLASS( CZMPhysExplosion, CPhysExplosion );
    DECLARE_DATADESC();


    CZMPhysExplosion();
    ~CZMPhysExplosion();

    void Spawn( void ) OVERRIDE;
    void Precache( void ) OVERRIDE;
    
    void DelayedExplode( float );

private:
    void DelayThink();
    void CreateEffects( float );

    EHANDLE m_hSpark;
};
