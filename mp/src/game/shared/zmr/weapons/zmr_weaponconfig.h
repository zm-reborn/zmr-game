#pragma once



#include "weapon_parse.h"

namespace ZMWeaponConfig
{
    //
    // Add new weapons here. Also modify the CZMWeaponConfigSystem constructor.
    //
    enum WeaponConfigSlot_t
    {
        ZMCONFIGSLOT_INVALID = -1,


        ZMCONFIGSLOT_FISTSCARRY,

        ZMCONFIGSLOT_IMPROVISED,
        ZMCONFIGSLOT_SLEDGE,

        ZMCONFIGSLOT_PISTOL,
        ZMCONFIGSLOT_REVOLVER,

        ZMCONFIGSLOT_SHOTGUN,
        ZMCONFIGSLOT_SHOTGUNSPORTING,

        ZMCONFIGSLOT_MAC10,

        ZMCONFIGSLOT_RIFLE,

        ZMCONFIGSLOT_MOLOTOV,


        ZMCONFIGSLOT_REGISTERED_END,
        ZMCONFIGSLOT_CUSTOM_START = ZMCONFIGSLOT_REGISTERED_END,



        ZMCONFIGSLOT_MAX = 64
    };


    //
    struct ZMAttackConfig_t
    {
        void Reset();
        void ToKeyValues( KeyValues* kv ) const;
        void FromKeyValues( KeyValues* kv );

        float flDamage;

        Vector vecViewPunch_Min;
        Vector vecViewPunch_Max;

        Vector vecSpread;

        float flFireRate;

        float flRange;

        int nBulletsPerShot;
    };
    //
    
    //
    enum WeaponFlags_t
    {
        WEPFLAG_NONE                = 0,

        WEPFLAG_ATTACK_ONLADDER     = ( 1 << 0 ),
        WEPFLAG_ATTACK_INWATER      = ( 1 << 1 ),

        WEPFLAG_RELOAD_ONLADDER     = ( 1 << 2 ),
    };
    //

    //
    class CZMBaseWeaponConfig
    {
    public:
        CZMBaseWeaponConfig( const char* wepname, const char* configpath );
        ~CZMBaseWeaponConfig();


        virtual void LoadFromConfig( KeyValues* kv );
        virtual KeyValues* ToKeyValues() const;

        virtual void OnCustomConfigLoaded();


        static bool IsValidFirerate( float rate ) { return rate >= 0.0f; }

        virtual const char* GetDebugName() const;

        int GetPrimaryAmmoIndex() const;


        virtual bool OverrideFromConfig( KeyValues* kv );
        void OverrideAttack( KeyValues* kv, ZMAttackConfig_t& attack );


        char* pszWeaponName;
        char* pszConfigFilePath;
        char* pszPrintName;


        //
        int iSlot;
        int iPosition;

        int nWeight; // Weight, as in "importance".
        //


        //
        char* pszAnimPrefix;
        char* pszModel_View;
        char* pszModel_World;

        bool bOverriddenViewmodel;
        bool bOverriddenWorldmodel;
        //


        //
        int fFlags; // WeaponFlags_t

        float flReloadTime;
        //

    
        //
        // Attacks
        //
        ZMAttackConfig_t primary;
        ZMAttackConfig_t secondary;
        //


        //
        char* pszAmmoName;

        int nClipSize;
        int nDefaultClip;
        //


        //
        float flAccuracyIncrease;
        float flAccuracyDecrease;

        float flPenetrationDmgMult;
        int nMaxPenetrations;
        int flMaxPenetrationDist;
        //


        //
        char* pszSounds[WeaponSound_t::NUM_SHOOT_SOUND_TYPES];
        //


        //
#ifdef CLIENT_DLL
        CHudTexture* pIconActive;
        CHudTexture* pIconInactive;
        CHudTexture* pIconAmmo;

        int iCrosshair;
#endif
        //


        //
        bool bUseHands;
        //


        static void ComputeSpread( const char* data, Vector& spread );
        static void CopyAllocString( const char* str, char** out );
        void CopyAmmoIndex( const char* str, int& ammo );

        static void CopyVector( const char* str, Vector& vec );
        static void VectorToKv( KeyValues* kv, const char* name, const Vector& vec );


    protected:

#ifdef CLIENT_DLL
        static void CopyIcon( KeyValues* kv, CHudTexture** icon );
        static void IconToKv( KeyValues* kv, CHudTexture* icon );
#endif
    };
    //



    //
#define REGISTER_WEAPON_CONFIG(slot, configclass) static ZMWeaponConfig::CZMWepConfigReg reg_##classname( slot, []( const char* wepname, const char* configpath ) { return (ZMWeaponConfig::CZMBaseWeaponConfig*)(new configclass( wepname, configpath )); } );
    //


    //
    typedef CZMBaseWeaponConfig* (CreateWeaponConfigFn)( const char* wepname, const char* configpath );
    //

    //
    struct CreateWeaponConfig_t
    {
        CreateWeaponConfig_t()
        {
            pszWeaponName = "";
            fn = nullptr;
        }

        const char* pszWeaponName;
        CreateWeaponConfigFn* fn;
    };
    //



    //
    class CZMWepConfigReg
    {
    public:
        CZMWepConfigReg( WeaponConfigSlot_t slot, CreateWeaponConfigFn fn );
    };
    //


    //
    class CZMWeaponConfigSystem : public CAutoGameSystem
#ifdef CLIENT_DLL
        ,public CGameEventListener
#endif
    {
    public:
        CZMWeaponConfigSystem();
        ~CZMWeaponConfigSystem();


        static bool IsDebugging();


        virtual void PostInit() OVERRIDE;
        virtual void LevelInitPreEntity() OVERRIDE;

#ifdef CLIENT_DLL
        virtual void FireGameEvent( IGameEvent* event ) OVERRIDE;
#endif

        bool IsSlotRegistered( WeaponConfigSlot_t slot ) const;

        void ReloadConfigs();

    
        WeaponConfigSlot_t RegisterBareBonesWeapon( const char* classname );

        WeaponConfigSlot_t RegisterCustomWeapon( WeaponConfigSlot_t baseslot, const char* filename );

        const CZMBaseWeaponConfig* GetConfigBySlot( WeaponConfigSlot_t slot );

    protected:
        void RegisterBaseConfig( WeaponConfigSlot_t slot, CreateWeaponConfigFn fn );
        
        WeaponConfigSlot_t FindCustomConfigByConfigPath( const char* configpath ) const;
        WeaponConfigSlot_t FindEmptyCustomConfigSlot() const;

        WeaponConfigSlot_t FindBaseSlotByClassname( const char* classname ) const;
        const char* GetBaseClassname( WeaponConfigSlot_t baseslot ) const;

        void ClearCustomConfigs();

    private:
        void InitBaseConfigs();

        CZMBaseWeaponConfig* LoadConfigFromFile( const char* szWeaponName, CreateWeaponConfigFn fn );
        CZMBaseWeaponConfig* LoadCustomConfigFromFile( WeaponConfigSlot_t baseslot, const char* filepath );


        CZMBaseWeaponConfig* m_pConfigs[ZMCONFIGSLOT_MAX];


        CreateWeaponConfig_t m_ConfigRegisters[ZMCONFIGSLOT_REGISTERED_END];


        friend class CZMWepConfigReg;
    };
    //


    extern CZMWeaponConfigSystem* GetWeaponConfigSystem();
}
