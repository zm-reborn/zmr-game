#pragma once

//#include "c_baseplayer.h"
#include "hl2mp/c_hl2mp_player.h"


#include "zmr/c_zmr_charcircle.h"
#include "zmr/zmr_shareddefs.h"

#include "zmr/zmr_playerlocaldata.h"
#include "zmr/zmr_player_shared.h"


class C_ZMPlayer : public C_HL2MP_Player
{
public:
    DECLARE_CLASS( C_ZMPlayer, C_HL2MP_Player );
    DECLARE_CLIENTCLASS();
    //DECLARE_PREDICTABLE(); 
    DECLARE_INTERPOLATION();


    C_ZMPlayer();
    ~C_ZMPlayer();

    
    virtual void ClientThink() OVERRIDE;
    virtual void TeamChange( int ) OVERRIDE;
    virtual bool CreateMove( float delta, CUserCmd* cmd ) OVERRIDE;
    virtual int DrawModel( int ) OVERRIDE;

    // Custom...
    inline bool IsZM() { return GetTeamNumber() == ZMTEAM_ZM; };
    inline bool IsHuman() { return GetTeamNumber() == ZMTEAM_HUMAN; };

    static C_ZMPlayer* GetLocalPlayer();

    // Implemented in zm_player_shared
    bool HasEnoughResToSpawn( ZombieClass_t );
    bool HasEnoughRes( int );
    int GetResources();
    void IncResources( int, bool bLimit = false );
    void SetResources( int );
    float GetFlashlightBattery();
    void SetFlashlightBattery( float );
    bool Weapon_CanSwitchTo( C_BaseCombatWeapon* ) OVERRIDE;
    Participation_t GetParticipation();
    static Participation_t GetLocalParticipation();
    static void SetLocalParticipation( Participation_t );
    virtual void PlayStepSound( Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force ) OVERRIDE;


    void SetMouseWheelMove( float dir );

protected:
    CZMCharCircle* m_fxInner;
    CZMCharCircle* m_fxHealth;

private:
    CNetworkVarEmbedded( CZMPlayerLocalData, m_ZMLocal );

    // Only used locally.
    float m_flUpMove;
    float m_flNextUpMove;
};

inline C_ZMPlayer* ToZMPlayer( C_BaseEntity* pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return nullptr;

	return dynamic_cast<C_ZMPlayer*>( pEntity );
}

inline C_ZMPlayer* ToZMPlayer( C_BasePlayer* pPlayer )
{
	return static_cast<C_ZMPlayer*>( pPlayer );
}
