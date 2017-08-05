#pragma once

#include "c_ai_basenpc.h"
#include "fx_quad.h"


#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_shareddefs.h"


class CZMEasyQuad
{
public:
    CZMEasyQuad( const FXQuadData_t* data )
    {
        m_QuadData = *data;
    }

    void Draw();
    void SetPos( Vector origin ) { m_QuadData.SetOrigin( origin ); };
    void SetColor( float r, float g, float b ) { m_QuadData.SetColor( r, g, b ); };
    void SetAlpha( float a ) { m_QuadData.SetAlpha( a, a ); };
    
protected:
    FXQuadData_t m_QuadData;
};

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


    void CreateHealthFX();
    void CreateInnerFX();

protected:
    void RemoveFXs();


    CZMEasyQuad* m_fxHealth;
    CZMEasyQuad* m_fxInner;

private:
    CNetworkVar( int, m_iSelectorIndex );
};