#pragma once

#include "hl2/npc_BaseZombie.h"


#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_shareddefs.h"


/*
    NOTE: You have to remove files from the project:

    npc_zombie.cpp
    npc_PoisonZombie.cpp
*/

struct LayerRecordNPC
{
	int m_sequence;
	float m_cycle;
	float m_weight;
	int m_order;
 
	LayerRecordNPC()
	{
		m_sequence = 0;
		m_cycle = 0;
		m_weight = 0;
		m_order = 0;
	}
 
	LayerRecordNPC( const LayerRecordNPC& src )
	{
		m_sequence = src.m_sequence;
		m_cycle = src.m_cycle;
		m_weight = src.m_weight;
		m_order = src.m_order;
	}
};
 
struct LagRecordNPC
{
public:
	LagRecordNPC()
	{
		m_fFlags = 0;
		m_vecOrigin.Init();
		m_vecAngles.Init();
		m_vecMins.Init();
		m_vecMaxs.Init();
		m_flSimulationTime = -1;
		m_masterSequence = 0;
		m_masterCycle = 0;
	}
 
	LagRecordNPC( const LagRecordNPC& src )
	{
		m_fFlags = src.m_fFlags;
		m_vecOrigin = src.m_vecOrigin;
		m_vecAngles = src.m_vecAngles;
		m_vecMins = src.m_vecMins;
		m_vecMaxs = src.m_vecMaxs;
		m_flSimulationTime = src.m_flSimulationTime;
		for( int layerIndex = 0; layerIndex < CBaseAnimatingOverlay::MAX_OVERLAYS; ++layerIndex )
		{
			m_layerRecords[layerIndex] = src.m_layerRecords[layerIndex];
		}
		m_masterSequence = src.m_masterSequence;
		m_masterCycle = src.m_masterCycle;
	}
 
	// Did player die this frame
	int						m_fFlags;
 
	// Player position, orientation and bbox
	Vector					m_vecOrigin;
	QAngle					m_vecAngles;
	Vector					m_vecMins;
	Vector					m_vecMaxs;
 
	float					m_flSimulationTime;	
 
	// Player animation details, so we can get the legs in the right spot.
	LayerRecordNPC			m_layerRecords[CBaseAnimatingOverlay::MAX_OVERLAYS];
	int						m_masterSequence;
	float					m_masterCycle;
};

class CZMBaseZombie : public CNPC_BaseZombie
{
public:
    DECLARE_CLASS( CZMBaseZombie, CNPC_BaseZombie )
    DECLARE_SERVERCLASS()
    DECLARE_DATADESC()

    CZMBaseZombie();
    ~CZMBaseZombie();
    

    virtual void Spawn() OVERRIDE;

    virtual void HandleAnimEvent( animevent_t* ) OVERRIDE;
    
    virtual bool IsValidEnemy( CBaseEntity* ) OVERRIDE;
    virtual bool IsChopped( const CTakeDamageInfo &info ) OVERRIDE;
    virtual bool CanBecomeLiveTorso() OVERRIDE;
	virtual bool ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold ) OVERRIDE;
    virtual HeadcrabRelease_t ShouldReleaseHeadcrab( const CTakeDamageInfo &info, float flDamageThreshold ) OVERRIDE;

    virtual const char *GetLegsModel( void ) OVERRIDE { return ""; };
	virtual const char *GetTorsoModel( void ) OVERRIDE { return ""; };
	virtual const char *GetHeadcrabClassname( void ) OVERRIDE { return ""; };
	virtual const char *GetHeadcrabModel( void ) OVERRIDE { return ""; };

    virtual void SetZombieModel( void ) OVERRIDE;

    virtual bool ShouldGib( const CTakeDamageInfo& ) OVERRIDE;


    // By default don't swat objects.
    virtual bool CanSwatPhysicsObjects() OVERRIDE { return false; };

    // Always classify as a zombie. The AI relationships depend on it.
    virtual Class_T Classify( void ) OVERRIDE { return CLASS_ZOMBIE; };

    virtual bool MustCloseToAttack() OVERRIDE { return true; };

    // Our stuff...
    int GetPopCost() { return CZMBaseZombie::GetPopCost( GetZombieClass() ); };
    int GetCost() { return CZMBaseZombie::GetCost( GetZombieClass() ); };
    ZombieClass_t GetZombieClass() { return m_iZombieClass; };



    virtual void Command( const Vector& );
    virtual bool Swat( CBaseEntity* );
    void SwatObject( IPhysicsObject*, Vector& );
    virtual bool TargetEnemy( CBaseEntity* );


    bool CanSpawn( const Vector& );


    // Implemented in zmr_zombiebase_shared
    static bool IsValidClass( ZombieClass_t );
    static ZombieClass_t NameToClass( const char* );
    static const char* ClassToName( ZombieClass_t );
    static int GetPopCost( ZombieClass_t );
    static int GetCost( ZombieClass_t );
    static bool HasEnoughPopToSpawn( ZombieClass_t );
    int GetSelectorIndex();
    CZMPlayer* GetSelector();
    void SetSelector( CZMPlayer* );
    void SetSelector( int );

private:
    ZombieClass_t m_iZombieClass;
    
protected:
    inline void SetZombieClass( ZombieClass_t zclass ) { m_iZombieClass = zclass; };


    CNetworkVar( int, m_iSelectorIndex );



    // Lag compensation.
public:
	CUtlFixedLinkedList<LagRecordNPC>* GetLagTrack() { return m_LagTrack; }
	LagRecordNPC*	GetLagRestoreData() { if ( m_RestoreData != NULL ) return m_RestoreData; else return new LagRecordNPC(); }
	LagRecordNPC*	GetLagChangeData() { if ( m_ChangeData != NULL ) return m_ChangeData; else return new LagRecordNPC(); }
	void		SetLagRestoreData(LagRecordNPC* l) { if ( m_RestoreData != NULL ) delete m_RestoreData; m_RestoreData = l; }
	void		SetLagChangeData(LagRecordNPC* l) { if ( m_ChangeData != NULL ) delete m_ChangeData; m_ChangeData = l; }
	void		FlagForLagCompensation( bool tempValue ) { m_bFlaggedForLagCompensation = tempValue; }
	bool		IsLagFlagged() { return m_bFlaggedForLagCompensation; }
 
private:
	CUtlFixedLinkedList<LagRecordNPC>* m_LagTrack;
	LagRecordNPC*	m_RestoreData;
	LagRecordNPC*	m_ChangeData;
	bool		m_bFlaggedForLagCompensation;
};