#pragma once

#include "c_ai_basenpc.h"



#include "zmr/c_zmr_charcircle.h"

#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_shareddefs.h"


#define MAX_GROUP_INDEX             9
#define INVALID_GROUP_INDEX         -1


// ZMRTODO: Predict selector index.
class C_ZMBaseZombie : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_ZMBaseZombie, C_AI_BaseNPC )
	DECLARE_CLIENTCLASS()
	DECLARE_PREDICTABLE();
    DECLARE_DATADESC()

    C_ZMBaseZombie();
    ~C_ZMBaseZombie();

    virtual void Spawn( void ) OVERRIDE;

    virtual int     DrawModel( int flags ) OVERRIDE;
    int             DrawModelAndEffects( int flags );

    virtual Vector          GetObserverCamOrigin() OVERRIDE { return WorldSpaceCenter(); };
    virtual const QAngle&   EyeAngles() OVERRIDE;
    virtual Vector          EyePosition() OVERRIDE;

    virtual const char* GetZombieLocalization() { return ""; };
    
    //virtual void TraceAttack( const CTakeDamageInfo&, const Vector&, trace_t*,CDmgAccumulator* ) OVERRIDE;
    
    // Implemented in zmr_zombiebase_shared
    static bool             IsValidClass( ZombieClass_t zclass );
    static ZombieClass_t    NameToClass( const char* name );
    static const char*      ClassToName( ZombieClass_t zclass );
    static int              GetPopCost( ZombieClass_t zclass );
    static int              GetCost( ZombieClass_t zclass );
    static bool             HasEnoughPopToSpawn( ZombieClass_t zclass );
    int                     GetSelectorIndex();
    C_ZMPlayer*             GetSelector();
    void                    SetSelector( C_ZMPlayer* pPlayer );
    void                    SetSelector( int index );
    ZombieClass_t           GetZombieClass();
protected:
    void                    SetZombieClass( ZombieClass_t zclass );
public:


    inline int  GetGroup() { return m_iGroup; };
    inline void SetGroup( int group ) { m_iGroup = group; };

protected:
    CZMCharCircle* m_fxHealth;
    CZMCharCircle* m_fxInner;

private:
    CNetworkVar( int, m_iSelectorIndex );
    CNetworkVar( float, m_flHealthRatio );

    int m_iGroup;
    ZombieClass_t m_iZombieClass;

    QAngle m_angEyeAttachment;
};

inline C_ZMBaseZombie* ToZMBaseZombie( C_BaseEntity* pEnt )
{
    if ( !pEnt || !pEnt->IsNPC() )
        return nullptr;

    // We have to dynamic cast due to npc_crow, etc.
    return dynamic_cast<C_ZMBaseZombie*>( pEnt );
}

inline C_ZMBaseZombie* ToZMBaseZombie( C_AI_BaseNPC* pEnt )
{
    // We have to dynamic cast due to npc_crow, etc.
    return dynamic_cast<C_ZMBaseZombie*>( pEnt );
}

