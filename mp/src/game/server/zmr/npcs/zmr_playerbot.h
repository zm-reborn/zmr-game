#pragma once

#include "npcr/npcr_senses.h"

#include "zmr_player.h"
#include "npcr/npcr_player.h"


enum ZMBotWeaponTypeRange_t
{
    BOTWEPRANGE_INVALID = 0,

    BOTWEPRANGE_CLOSERANGE,
    BOTWEPRANGE_LONGRANGE,
    BOTWEPRANGE_THROWABLE,
    BOTWEPRANGE_MELEE,

    BOTWEPRANGE_MAX
};

class CZMPlayerBot : public NPCR::CPlayer<CZMPlayer>
{
public:
    DECLARE_CLASS( CZMPlayerBot, CZMPlayer );


    CZMPlayerBot();
    ~CZMPlayerBot();

    //
    virtual NPCR::CScheduleInterface*   CreateScheduleInterface() OVERRIDE;
    virtual NPCR::CBaseSenses*          CreateSenses() OVERRIDE;

    // HACK
    virtual void SetEyeAngles( const QAngle& ang ) OVERRIDE { SetAngles( ang ); pl.v_angle = ang; }
    
    virtual bool ShouldUpdate() const OVERRIDE;    
    //

    virtual void Spawn() OVERRIDE;

    //
    static CZMPlayer* CreateZMBot( const char* playername = "" );
    // Called from NPCR::CPlayer to create the player entity
    static CBasePlayer* BotPutInServer( edict_t* pEdict, const char* playername );
    //


    virtual bool OverrideUserCmd( CUserCmd& cmd ) OVERRIDE;
    

    //
    CZMBaseWeapon*  GetActiveWeapon() const { return ToZMBaseWeapon( m_hActiveWeapon.Get() ); }

    static ZMBotWeaponTypeRange_t GetWeaponType( CZMBaseWeapon* pWeapon );
    static ZMBotWeaponTypeRange_t GetWeaponType( const char* classname );
    bool            HasWeaponOfType( ZMBotWeaponTypeRange_t wepType ) const;
    bool            HasEquippedWeaponOfType( ZMBotWeaponTypeRange_t wepType ) const;
    bool            EquipWeaponOfType( ZMBotWeaponTypeRange_t wepType );
    ZMBotWeaponTypeRange_t GetCurrentWeaponType() const;
    bool            WeaponHasAmmo( CZMBaseWeapon* pWeapon ) const;
    bool            ShouldReload() const;
    bool            HasAnyEffectiveRangeWeapons() const;
    bool            CanReload() const;
    bool            CanAttack() const;
    bool            MustStopToShoot() const;
    float           GetOptimalAttackDistance() const;
    float           GetMaxAttackDistance() const;
protected:
    CZMBaseWeapon*  FindWeaponOfType( ZMBotWeaponTypeRange_t wepType ) const;
public:
    //


    CBasePlayer*    GetFollowTarget() const { return m_hFollowTarget.Get(); }
    void            SetFollowTarget( CBasePlayer* pPlayer ) { m_hFollowTarget.Set( pPlayer ); }

private:
    CHandle<CBasePlayer> m_hFollowTarget;
};
