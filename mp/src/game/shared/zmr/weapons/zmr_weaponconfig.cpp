#include "cbase.h"
#include "ammodef.h"
#include "filesystem.h"
#ifdef CLIENT_DLL
#include "history_resource.h"
#include "hud.h"


#include <vgui/ISurface.h>
#endif

#include "zmr_weaponconfig.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define DEFAULT_MAX_RANGE       1024.0f

#define CONFIG_DIR      "scripts/zmweapons"


using namespace ZMWeaponConfig;

CZMWeaponConfigSystem* ZMWeaponConfig::GetWeaponConfigSystem()
{
    static CZMWeaponConfigSystem system;
    return &system;
}




CZMBaseWeaponConfig::CZMBaseWeaponConfig( const char* wepname )
{
    CopyAllocString( wepname, &pszWeaponName );


    pszPrintName = nullptr;

    pszAnimPrefix = nullptr;
    pszModel_View = nullptr;
    pszModel_World = nullptr;

    pszAmmoName = nullptr;

    for ( int i = 0; i < ARRAYSIZE( pszSounds ); i++ )
        pszSounds[i] = nullptr;


#ifdef CLIENT_DLL
    pIconActive = nullptr;
    pIconInactive = nullptr;
    pIconAmmo = nullptr;
#endif
}

CZMBaseWeaponConfig::~CZMBaseWeaponConfig()
{
    delete[] pszWeaponName;
    delete[] pszAnimPrefix;
    delete[] pszModel_View;
    delete[] pszModel_World;
    delete[] pszAmmoName;
    delete[] pszPrintName;

#ifdef CLIENT_DLL
    delete pIconActive;
    delete pIconInactive;
    delete pIconAmmo;
#endif
}

void CZMBaseWeaponConfig::LoadFromConfig( KeyValues* kv )
{
    CopyAllocString( kv->GetString( "printname" ), &pszPrintName );

    CopyAllocString( kv->GetString( "anim_prefix" ), &pszAnimPrefix );
    CopyAllocString( kv->GetString( "viewmodel" ), &pszModel_View );
    CopyAllocString( kv->GetString( "worldmodel" ), &pszModel_World );

    CopyAllocString( kv->GetString( "primary_ammo" ), &pszAmmoName );





    nClipSize = kv->GetInt( "clip_size", WEAPON_NOCLIP );
    nDefaultClip = kv->GetInt( "default_clip", 0 );


    auto* primattack = kv->FindKey( "PrimaryAttack" );
    if ( primattack )
    {
        LoadPrimaryAttack( primattack );
    }


    auto* secattack = kv->FindKey( "SecondaryAttack" );
    if ( secattack )
    {
        LoadSecondaryAttack( secattack );
    }



    flAccuracyIncrease = kv->GetFloat( "accuracy_increase", 2.0f );
    flAccuracyDecrease = kv->GetFloat( "accuracy_decrease", 2.0f );

    flPenetrationDmgMult = kv->GetFloat( "penetration_dmgmult", 1.0f );
    flMaxPenetrationDist = kv->GetFloat( "penetration_maxdist", 16.0f );
    nMaxPenetrations = kv->GetInt( "penetration_max", 0 );


    iSlot = kv->GetInt( "bucket", 0 );
    iPosition = kv->GetInt( "bucket_position", 0 );
    nWeight = kv->GetInt( "importance", 0 );


    // Sounds
    auto* snddata = kv->FindKey( "SoundData" );
    if ( snddata )
    {
        CopyAllocString( snddata->GetString( "empty" ), &pszSounds[WeaponSound_t::EMPTY] );
        CopyAllocString( snddata->GetString( "single_shot" ), &pszSounds[WeaponSound_t::SINGLE] );
        CopyAllocString( snddata->GetString( "reload" ), &pszSounds[WeaponSound_t::RELOAD] );
        CopyAllocString( snddata->GetString( "melee_hit" ), &pszSounds[WeaponSound_t::MELEE_HIT] );
        CopyAllocString( snddata->GetString( "deploy" ), &pszSounds[WeaponSound_t::DEPLOY] );
    }

#ifdef CLIENT_DLL
    auto* texdata = kv->FindKey( "TextureData" );
    if ( texdata )
    {
        CopyIcon( texdata->FindKey( "weapon_s" ), &pIconActive );
        CopyIcon( texdata->FindKey( "weapon" ), &pIconInactive );
        CopyIcon( texdata->FindKey( "ammo" ), &pIconAmmo );

        if ( pIconActive )
        {
            auto* pHudHR = GET_HUDELEMENT( CHudHistoryResource );
            if ( pHudHR )
            {
                pHudHR->SetHistoryGap( pIconActive->Height() );
            }
        }
    }
#endif



    bUseHands = kv->GetBool( "usenewhands", true );
}

void CZMBaseWeaponConfig::LoadPrimaryAttack( KeyValues* kv )
{
    flPrimaryDamage = kv->GetFloat( "damage", 0.0f );
    flPrimaryFireRate = kv->GetFloat( "firerate", 0.0f );
    flPrimaryRange = kv->GetFloat( "range", DEFAULT_MAX_RANGE );

    ComputeSpread( kv->GetString( "spread" ), vecPrimarySpread );
    CopyVector( kv->GetString( "viewpunch_min" ), vecPrimaryViewPunch_Min );
    CopyVector( kv->GetString( "viewpunch_max" ), vecPrimaryViewPunch_Max );
}

void CZMBaseWeaponConfig::LoadSecondaryAttack( KeyValues* kv )
{
    flSecondaryDamage = kv->GetFloat( "damage", 0.0f );
    flSecondaryFireRate = kv->GetFloat( "firerate", 0.0f );
    flSecondaryRange = kv->GetFloat( "range", DEFAULT_MAX_RANGE );

    ComputeSpread( kv->GetString( "spread" ), vecSecondarySpread );
    CopyVector( kv->GetString( "viewpunch_min" ), vecSecondaryViewPunch_Min );
    CopyVector( kv->GetString( "viewpunch_max" ), vecSecondaryViewPunch_Max );
}

int CZMBaseWeaponConfig::GetPrimaryAmmoIndex() const
{
    return GetAmmoDef()->Index( pszAmmoName );
}

const char* CZMBaseWeaponConfig::GetDebugName() const
{
    return (pszPrintName && *pszPrintName) ? pszPrintName : "(Unknown)";
}

void CZMBaseWeaponConfig::ComputeSpread( const char* data, Vector& spread )
{
    Vector temp;
    CopyVector( data, temp );

    temp /= 2.0f;

    spread.x = sinf( DEG2RAD( temp.x ) );
    spread.y = sinf( DEG2RAD( temp.y ) );
    spread.z = sinf( DEG2RAD( temp.z ) );
}

void CZMBaseWeaponConfig::CopyAllocString( const char* str, char** out )
{
    delete[] *out;


    int len = Q_strlen( str ) + 1;
    *out = new char[len];

    Q_strncpy( *out, str, len );
}

void CZMBaseWeaponConfig::CopyAmmoIndex( const char* str, int& ammo )
{
    ammo = GetAmmoDef()->Index( str );

    if ( ammo <= -1 )
    {
        Warning( "Weapon %s has valid ammo index %i!\n", GetDebugName(), ammo );
    }
}

void CZMBaseWeaponConfig::CopyVector( const char* str, Vector& vec )
{
    vec = vec3_origin;
    sscanf( str, "%f %f %f", &vec.x, &vec.y, &vec.z );
}

#ifdef CLIENT_DLL
void CZMBaseWeaponConfig::CopyIcon( KeyValues* kv, CHudTexture** icon )
{
    if ( !kv )
    {
        *icon = nullptr;
        return;
    }


    *icon = new CHudTexture();

    CHudTexture* pTex = *icon;

    // Key Name is the sprite name
    Q_strncpy( pTex->szShortName, kv->GetName(), sizeof( pTex->szShortName ) );

    // it's a font-based icon
    pTex->bRenderUsingFont = true;
    pTex->cCharacterInFont = *(kv->GetString( "character", "" ));
    Q_strncpy( pTex->szTextureFile, kv->GetString( "font" ), sizeof( pTex->szTextureFile ) );


    vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
    pTex->hFont = vgui::scheme()->GetIScheme( scheme )->GetFont( pTex->szTextureFile, true );
    pTex->rc.top = 0;
    pTex->rc.left = 0;
    pTex->rc.right = vgui::surface()->GetCharacterWidth( pTex->hFont, pTex->cCharacterInFont );
    pTex->rc.bottom = vgui::surface()->GetFontTall( pTex->hFont );


    pTex->Precache();
}
#endif

bool CZMBaseWeaponConfig::OverrideFromConfig( KeyValues* kv )
{
#define INVALID_VALUE_F          -1337.0f


    auto* primattack = kv->FindKey( "PrimaryAttack" );
    if ( primattack )
    {
        float temp;

        temp = primattack->GetFloat( "damage", INVALID_VALUE_F );
        if ( temp != INVALID_VALUE_F )
        {
            flPrimaryDamage = temp;
        }

        temp = primattack->GetFloat( "firerate", INVALID_VALUE_F );
        if ( temp != INVALID_VALUE_F )
        {
            flPrimaryFireRate = temp;
        }

        temp = primattack->GetFloat( "range", INVALID_VALUE_F );
        if ( temp != INVALID_VALUE_F )
        {
            flPrimaryRange = temp;
        }
    }


    auto* secattack = kv->FindKey( "SecondaryAttack" );
    if ( secattack )
    {
        float temp;

        temp = secattack->GetFloat( "damage", INVALID_VALUE_F );
        if ( temp != INVALID_VALUE_F )
        {
            flSecondaryDamage = temp;
        }

        temp = secattack->GetFloat( "firerate", INVALID_VALUE_F );
        if ( temp != INVALID_VALUE_F )
        {
            flSecondaryFireRate = temp;
        }

        temp = secattack->GetFloat( "range", INVALID_VALUE_F );
        if ( temp != INVALID_VALUE_F )
        {
            flSecondaryRange = temp;
        }
    }





    const char* mdl;
    
    
    mdl = kv->GetString( "viewmodel", nullptr );
    if ( mdl )
    {
        CopyAllocString( mdl, &pszModel_View );
    }

    mdl = kv->GetString( "worldmodel", nullptr );
    if ( mdl )
    {
        CopyAllocString( mdl, &pszModel_World );
    }


    int hands = kv->GetInt( "usenewhands", -1 );
    if ( hands != -1 )
    {
        bUseHands = ( hands > 0 ) ? true : false;
    }


    return true;
}




//
//
//
CZMWepConfigReg::CZMWepConfigReg( const char* classname, WeaponConfigSlot_t slot, CreateWeaponConfigFn fn )
{
    GetWeaponConfigSystem()->RegisterConfig( classname, slot, fn );
}


//
//
//
CZMWeaponConfigSystem::CZMWeaponConfigSystem() : CAutoGameSystem( "ZMWeaponConfigSystem" )
{
    for ( int i = 0; i < ARRAYSIZE( m_pConfigs ); i++ )
    {
        m_pConfigs[i] = nullptr;
    }


    for ( int i = 0; i < ARRAYSIZE( m_ConfigRegisters ); i++ )
    {
        m_ConfigRegisters[i].fn = []( const char* wepname ) { return new CZMBaseWeaponConfig( wepname ); };
    }


    m_ConfigRegisters[ZMCONFIGSLOT_FISTSCARRY].pszWeaponName = "weapon_zm_fistscarry";
    m_ConfigRegisters[ZMCONFIGSLOT_IMPROVISED].pszWeaponName = "weapon_zm_improvised";
    m_ConfigRegisters[ZMCONFIGSLOT_SLEDGE].pszWeaponName = "weapon_zm_sledge";
    m_ConfigRegisters[ZMCONFIGSLOT_PISTOL].pszWeaponName = "weapon_zm_pistol";
    m_ConfigRegisters[ZMCONFIGSLOT_REVOLVER].pszWeaponName = "weapon_zm_revolver";
    m_ConfigRegisters[ZMCONFIGSLOT_SHOTGUN].pszWeaponName = "weapon_zm_shotgun";
    m_ConfigRegisters[ZMCONFIGSLOT_SHOTGUNSPORTING].pszWeaponName = "weapon_zm_shotgun_sporting";
    m_ConfigRegisters[ZMCONFIGSLOT_MAC10].pszWeaponName = "weapon_zm_mac10";
    m_ConfigRegisters[ZMCONFIGSLOT_RIFLE].pszWeaponName = "weapon_zm_rifle";
    m_ConfigRegisters[ZMCONFIGSLOT_MOLOTOV].pszWeaponName = "weapon_zm_molotov";
}

CZMWeaponConfigSystem::~CZMWeaponConfigSystem()
{
}

void CZMWeaponConfigSystem::PostInit()
{
    InitConfigs();
}

void CZMWeaponConfigSystem::LevelInitPreEntity()
{

}

void CZMWeaponConfigSystem::InitConfigs()
{
    for ( int i = 0; i < ARRAYSIZE( m_ConfigRegisters ); i++ )
    {
        m_pConfigs[i] = LoadConfigFromFile( m_ConfigRegisters[i].pszWeaponName, m_ConfigRegisters[i].fn );
    }
}

bool CZMWeaponConfigSystem::IsSlotRegistered( WeaponConfigSlot_t slot ) const
{
    return m_pConfigs[slot] != nullptr;
}

WeaponConfigSlot_t CZMWeaponConfigSystem::RegisterBareBonesWeapon( const char* classname )
{
    WeaponConfigSlot_t slot = ZMCONFIGSLOT_INVALID;
    for ( int i = 0; i < ARRAYSIZE( m_pConfigs ); i++ )
    {
        if ( !m_pConfigs[i] || Q_stricmp( m_pConfigs[i]->pszWeaponName, classname ) == 0 )
        {
            slot = (WeaponConfigSlot_t)i;
        }
    }

    if ( !m_pConfigs[slot] )
    {
        m_pConfigs[slot] = LoadConfigFromFile( classname, nullptr );
    }

    return slot;
}

void CZMWeaponConfigSystem::RegisterConfig( const char* classname, WeaponConfigSlot_t slot, CreateWeaponConfigFn fn )
{
    m_ConfigRegisters[slot].fn = fn;
    m_ConfigRegisters[slot].pszWeaponName = classname;
}

const CZMBaseWeaponConfig* CZMWeaponConfigSystem::GetConfigBySlot( WeaponConfigSlot_t slot )
{
    if ( slot <= ZMCONFIGSLOT_INVALID )
    {
        static CZMBaseWeaponConfig config( "" );

        return &config;
    }

    return m_pConfigs[slot];
}

CZMBaseWeaponConfig* CZMWeaponConfigSystem::LoadConfigFromFile( const char* szWeaponName, CreateWeaponConfigFn fn )
{
    DevMsg( "Loading weapon config '%s'...\n", szWeaponName );


    char file[256];
    KeyValues* kv;
    
    
    Q_snprintf( file, sizeof( file ), CONFIG_DIR"/%s.txt", szWeaponName );

    kv = new KeyValues( "WeaponData" );
    if ( !kv->LoadFromFile( filesystem, file ) )
    {
        Warning( "Couldn't load weapon config '%s'!\n", file );
        return nullptr;
    }


    auto* pConfig = ( fn != nullptr ) ? (fn( szWeaponName )) : (new CZMBaseWeaponConfig( szWeaponName ));
    pConfig->LoadFromConfig( kv );

    kv->deleteThis();


    Q_snprintf( file, sizeof( file ), CONFIG_DIR"/%s_custom.txt", szWeaponName );
    
    kv = new KeyValues( "WeaponData" );
    if ( kv->LoadFromFile( filesystem, file, "MOD" ) )
    {
        pConfig->OverrideFromConfig( kv );
    }


    kv->deleteThis();


    return pConfig;
}
