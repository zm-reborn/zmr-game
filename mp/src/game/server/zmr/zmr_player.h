#pragma once


#include "hl2mp/hl2mp_player.h"

#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_entities.h"
#include "zmr/weapons/zmr_base.h"
#include "zmr/zmr_shareddefs.h"
#include "zmr/zmr_playerlocaldata.h"


/*
    NOTE: You have to:
    
    Remove LINK_ENTITY_TO_CLASS in hl2mp_player.cpp

    Override CanSprint manually


*/

extern ConVar zm_sv_antiafk;


class CZMBaseZombie;
class CZMBaseWeapon;

class CZMPlayer : public CHL2MP_Player
{
public:
    DECLARE_CLASS( CZMPlayer, CHL2MP_Player )
    DECLARE_SERVERCLASS()
    //DECLARE_PREDICTABLE()
    DECLARE_DATADESC()
    
    CZMPlayer();
    ~CZMPlayer( void );

    
    static CZMPlayer* CreatePlayer( const char* className, edict_t* ed )
    {
        CZMPlayer::s_PlayerEdict = ed;
        return static_cast<CZMPlayer*>( CreateEntityByName( className ) );
    }


    virtual void Precache( void ) OVERRIDE;
    virtual void Spawn() OVERRIDE;
    virtual void PreThink( void ) OVERRIDE;
    void PickDefaultSpawnTeam();
    virtual void ChangeTeam( int iTeam ) OVERRIDE;

    void SetPlayerModel( void );
    bool ValidatePlayerModel( const char* );

    virtual void FlashlightTurnOn() OVERRIDE;

    virtual bool Weapon_Lower() OVERRIDE { return false; };
    
    virtual void PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize ) OVERRIDE;
    virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon ) OVERRIDE;

    // Lag compensation stuff...
    void FireBullets( const FireBulletsInfo_t& ) OVERRIDE;
    bool WantsLagCompensationOnNPC( const CZMBaseZombie* a, const CUserCmd* b, const CBitVec<MAX_EDICTS>* c ) const;

	virtual void CommitSuicide( bool bExplode = false, bool bForce = false ) OVERRIDE;
	virtual void CommitSuicide( const Vector &vecForce, bool bExplode = false, bool bForce = false ) OVERRIDE;

    virtual CBaseEntity* EntSelectSpawnPoint( void ) OVERRIDE;

    void GiveDefaultItems( void );
    virtual void EquipSuit( bool = false ) OVERRIDE;
    virtual void RemoveAllItems( bool removeSuit ) OVERRIDE;

    virtual void PlayerUse( void ) OVERRIDE;
    //virtual void PlayUseDenySound() OVERRIDE;

    
    inline bool IsZM() { return GetTeamNumber() == ZMTEAM_ZM; };
    inline bool IsHuman() { return GetTeamNumber() == ZMTEAM_HUMAN; };

    float m_flNextResourceInc;


    void SetBuildSpawn( CZMEntZombieSpawn* pSpawn )
    {
        int index = 0;
        if ( pSpawn )
            index = pSpawn->entindex();


        m_iBuildSpawnIndex = index;
    }

    void SetBuildSpawn( int index )
    {
        m_iBuildSpawnIndex = index;
    }

    inline CZMEntZombieSpawn* GetBuildSpawn() { return dynamic_cast<CZMEntZombieSpawn*>( UTIL_EntityByIndex( m_iBuildSpawnIndex ) ); };
    inline int GetBuildSpawnIndex() { return m_iBuildSpawnIndex; };

    void DeselectAllZombies();


    void SetTeamSpecificProps();


    // Implemented in zm_player_shared
    bool HasEnoughResToSpawn( ZombieClass_t );
    bool HasEnoughRes( int );
    int GetResources();
    void SetResources( int );
    float GetFlashlightBattery();
    void SetFlashlightBattery( float );
    bool Weapon_CanSwitchTo( CBaseCombatWeapon* ) OVERRIDE;
    Participation_t GetParticipation();
    virtual void PlayStepSound( Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force ) OVERRIDE;

    
    int GetWeaponSlotFlags() { return m_fWeaponSlotFlags; };
    void SetWeaponSlotFlags( int flags ) { m_fWeaponSlotFlags = flags; };
    void AddWeaponSlotFlag( int flag ) { m_fWeaponSlotFlags |= flag; };
    void RemoveWeaponSlotFlag( int flag ) { m_fWeaponSlotFlags &= ~flag; };

    inline int GetPickPriority() { return m_nPickPriority; };
    inline void SetPickPriority( int i ) { m_nPickPriority = i; };
    
    inline float GetLastActivity() { return m_flLastActivity; };
    inline bool IsCloseToAFK() { return zm_sv_antiafk.GetInt() > 0 && (gpGlobals->curtime - GetLastActivity()) > (zm_sv_antiafk.GetFloat() * 0.8f); };
    inline bool IsAFK() { return zm_sv_antiafk.GetInt() > 0 && (gpGlobals->curtime - GetLastActivity()) > zm_sv_antiafk.GetFloat(); };

private:
    // Since I didn't get this at first either, this is only sent to THIS player.
    CNetworkVarEmbedded( CZMPlayerLocalData, m_ZMLocal );

    int m_iBuildSpawnIndex; // To update build menu.
    //Participation_t m_iParticipation;
    int m_fWeaponSlotFlags;
    int m_nPickPriority;
    float m_flLastActivity;
    float m_flLastActivityWarning;
};

inline CZMPlayer* ToZMPlayer( CBaseEntity* pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return nullptr;

	return dynamic_cast<CZMPlayer*>( pEntity );
}

inline CZMPlayer* ToZMPlayer( CBasePlayer* pPlayer )
{
	return static_cast<CZMPlayer*>( pPlayer );
}