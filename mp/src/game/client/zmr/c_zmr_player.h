#pragma once

//#include "c_baseplayer.h"
#include "hl2mp/c_hl2mp_player.h"


#include "zmr/c_zmr_charcircle.h"
#include "zmr/zmr_shareddefs.h"

#include "zmr/zmr_playerlocaldata.h"
#include "zmr/zmr_player_shared.h"


class C_ZMPlayer : public CHL2MP_Player
{
public:
    DECLARE_CLASS( C_ZMPlayer, CHL2MP_Player );
    DECLARE_CLIENTCLASS();
    //DECLARE_PREDICTABLE(); 
    DECLARE_INTERPOLATION();


    C_ZMPlayer();
    ~C_ZMPlayer();

    
    virtual void TeamChange( int ) OVERRIDE;

    virtual int DrawModel( int ) OVERRIDE;

    // Custom...
    inline bool IsZM() { return GetTeamNumber() == ZMTEAM_ZM; };
    inline bool IsHuman() { return GetTeamNumber() == ZMTEAM_HUMAN; };

    static C_ZMPlayer* GetLocalPlayer();

    // Implemented in zm_player_shared
    bool HasEnoughResToSpawn( ZombieClass_t );
    bool HasEnoughRes( int );
    int GetResources();
    void SetResources( int );
    float GetFlashlightBattery();
    void SetFlashlightBattery( float );
    bool Weapon_CanSwitchTo( C_BaseCombatWeapon* ) OVERRIDE;
    Participation_t GetParticipation();
    static Participation_t GetLocalParticipation();
    static void SetLocalParticipation( Participation_t );
    virtual void PlayStepSound( Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force ) OVERRIDE;

protected:
    CZMCharCircle* m_fxInner;
    CZMCharCircle* m_fxHealth;

private:
    CNetworkVarEmbedded( CZMPlayerLocalData, m_ZMLocal );
};

inline CZMPlayer* ToZMPlayer( CBaseEntity* pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CZMPlayer*>( pEntity );
}

inline CZMPlayer* ToZMPlayer( CBasePlayer* pPlayer )
{
	if ( !pPlayer || !pPlayer->IsPlayer() )
		return NULL;

	return static_cast<CZMPlayer*>( pPlayer );
}