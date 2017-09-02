#pragma once

#include "c_ai_basenpc.h"



#include "zmr/c_zmr_charcircle.h"

#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_shareddefs.h"




// ZMRTODO: Predict selector index.
class C_ZMBaseZombie : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_ZMBaseZombie, C_AI_BaseNPC )
	DECLARE_CLIENTCLASS()
	//DECLARE_PREDICTABLE()
    DECLARE_DATADESC()

    C_ZMBaseZombie();
    ~C_ZMBaseZombie();

    virtual void Spawn( void ) OVERRIDE;

    virtual int DrawModel( int ) OVERRIDE;
    
    //virtual void TraceAttack( const CTakeDamageInfo&, const Vector&, trace_t*,CDmgAccumulator* ) OVERRIDE;
    
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

protected:
    CZMCharCircle* m_fxHealth;
    CZMCharCircle* m_fxInner;

private:
    CNetworkVar( int, m_iSelectorIndex );
    CNetworkVar( float, m_flHealthRatio );
};
