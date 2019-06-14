#pragma once



#include "weapon_parse.h"


namespace ZMWeaponConfig
{
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
    class CZMBaseWeaponConfig
    {
    public:
        CZMBaseWeaponConfig( const char* wepname );
        ~CZMBaseWeaponConfig();


        virtual void LoadFromConfig( KeyValues* kv );


        static bool IsValidFirerate( float rate ) { return rate >= 0.0f; }

        virtual const char* GetDebugName() const;

        int GetPrimaryAmmoIndex() const;


        virtual bool OverrideFromConfig( KeyValues* kv );


        char* pszWeaponName;
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
        //

    
        //
        // Primary Attack
        //
        float flPrimaryDamage;

        Vector vecPrimaryViewPunch_Min;
        Vector vecPrimaryViewPunch_Max;

        Vector vecPrimarySpread;

        float flPrimaryFireRate;

        float flPrimaryRange;
        //


        //
        // Secondary Attack
        //
        float flSecondaryDamage;

        Vector vecSecondaryViewPunch_Min;
        Vector vecSecondaryViewPunch_Max;

        Vector vecSecondarySpread;

        float flSecondaryFireRate;

        float flSecondaryRange;
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
#endif
        //


        //
        bool bUseHands;
        //

    protected:
        virtual void LoadPrimaryAttack( KeyValues* kv );
        virtual void LoadSecondaryAttack( KeyValues* kv );

        void ComputeSpread( const char* data, Vector& spread );
        void CopyAllocString( const char* str, char** out );
        void CopyAmmoIndex( const char* str, int& ammo );
        void CopyVector( const char* str, Vector& vec );
#ifdef CLIENT_DLL
        void CopyIcon( KeyValues* kv, CHudTexture** icon );
#endif
    };
    //



    //
#define REGISTER_WEAPON_CONFIG(classname, slot, configclass) static CZMWepConfigReg reg_##className##( #classname, slot, []( const char* wepname ) { return (CZMBaseWeaponConfig*)(new configclass( wepname )); } );
    //


    //
    typedef CZMBaseWeaponConfig* (CreateWeaponConfigFn)( const char* wepname );
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
        CZMWepConfigReg( const char* classname, WeaponConfigSlot_t slot, CreateWeaponConfigFn fn );
    };
    //


    //
    class CZMWeaponConfigSystem : public CAutoGameSystem
    {
    public:
        CZMWeaponConfigSystem();
        ~CZMWeaponConfigSystem();


        virtual void PostInit() OVERRIDE;
        virtual void LevelInitPreEntity() OVERRIDE;


        bool IsSlotRegistered( WeaponConfigSlot_t slot ) const;

    
        WeaponConfigSlot_t RegisterBareBonesWeapon( const char* classname );
        void RegisterConfig( const char* classname, WeaponConfigSlot_t slot, CreateWeaponConfigFn fn );

        const CZMBaseWeaponConfig* GetConfigBySlot( WeaponConfigSlot_t slot );

    private:
        void InitConfigs();

        CZMBaseWeaponConfig* LoadConfigFromFile( const char* szWeaponName, CreateWeaponConfigFn fn );


        CZMBaseWeaponConfig* m_pConfigs[ZMCONFIGSLOT_MAX];


        CreateWeaponConfig_t m_ConfigRegisters[ZMCONFIGSLOT_REGISTERED_END];
    };
    //


    extern CZMWeaponConfigSystem* GetWeaponConfigSystem();
}
