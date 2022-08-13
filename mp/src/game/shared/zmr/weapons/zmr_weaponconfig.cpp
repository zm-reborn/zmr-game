#include "cbase.h"
#include "ammodef.h"
#include "filesystem.h"
#ifdef CLIENT_DLL
#include "ui/zmr_hud_itemhistory.h"
#include "hud.h"


#include <vgui/ISurface.h>

#include "c_zmr_crosshair.h"
#endif

#include "zmr_weaponconfig.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_sv_debug_weaponconfig( "zm_sv_debug_weaponconfig", "0", FCVAR_REPLICATED );


#define DEFAULT_MAX_RANGE       1024.0f

#define CONFIG_DIR      "scripts"


using namespace ZMWeaponConfig;

CZMWeaponConfigSystem* ZMWeaponConfig::GetWeaponConfigSystem()
{
    static CZMWeaponConfigSystem system;
    return &system;
}

#ifdef GAME_DLL
CON_COMMAND( zm_reload_weaponconfig, "" )
{
    GetWeaponConfigSystem()->ReloadConfigs();
}
#endif

//
//
//
void ZMAttackConfig_t::Reset()
{
    vecSpread = vec3_origin;
    vecViewPunch_Min = vec3_origin;
    vecViewPunch_Max = vec3_origin;

    flDamage = 0.0f;
    flRange = DEFAULT_MAX_RANGE;
    flFireRate = 1.0f;
    nBulletsPerShot = 1;
    bFireSingly = false;
}

void ZMAttackConfig_t::ToKeyValues( KeyValues* kv ) const
{
    kv->SetFloat( "damage", flDamage );
    kv->SetFloat( "firerate", flFireRate );
    kv->SetFloat( "range", flRange );
    kv->SetInt( "bulletspershot", nBulletsPerShot );
    kv->SetBool( "firesingly", bFireSingly );

    
    // Radius in radians to diameter in degrees
    Vector correctspread;
    for ( int i = 0; i < 3; i++ )
        correctspread[i] = RAD2DEG( asinf( vecSpread[i] ) ) * 2.0f;


    CZMBaseWeaponConfig::VectorToKv( kv, "spread", correctspread );
    CZMBaseWeaponConfig::VectorToKv( kv, "viewpunch_min", vecViewPunch_Min );
    CZMBaseWeaponConfig::VectorToKv( kv, "viewpunch_max", vecViewPunch_Max );
}

void ZMAttackConfig_t::FromKeyValues( KeyValues* kv )
{
    flDamage = kv->GetFloat( "damage", 0.0f );
    flFireRate = kv->GetFloat( "firerate", -1.0f );
    flRange = kv->GetFloat( "range", DEFAULT_MAX_RANGE );
    nBulletsPerShot = kv->GetInt( "bulletspershot", 1 );
    bFireSingly = kv->GetBool( "firesingly", false );


    CZMBaseWeaponConfig::ComputeSpread( kv->GetString( "spread" ), vecSpread );
    CZMBaseWeaponConfig::CopyVector( kv->GetString( "viewpunch_min" ), vecViewPunch_Min );
    CZMBaseWeaponConfig::CopyVector( kv->GetString( "viewpunch_max" ), vecViewPunch_Max );
}



//
//
//
CZMBaseWeaponConfig::CZMBaseWeaponConfig( const char* wepname, const char* configpath )
{
    pszWeaponName = nullptr;
    pszConfigFilePath = nullptr;


    CopyAllocString( wepname, &pszWeaponName );
    CopyAllocString( configpath, &pszConfigFilePath );
    

    pszPrintName = nullptr;

    pszPlayerAnimsName = nullptr;
    pszModel_View = nullptr;
    pszModel_World = nullptr;

    pszAmmoName = nullptr;

    for ( int i = 0; i < ARRAYSIZE( pszSounds ); i++ )
        pszSounds[i] = nullptr;


#ifdef CLIENT_DLL
    pIconActive = nullptr;
    pIconInactive = nullptr;
    pIconAmmo = nullptr;

    iCrosshair = -1;
#endif


    primary.Reset();
    secondary.Reset();

    bOverriddenViewmodel = false;
    bOverriddenWorldmodel = false;


    fFlags = WEPFLAG_NONE;
}

CZMBaseWeaponConfig::~CZMBaseWeaponConfig()
{
    delete[] pszWeaponName;
    delete[] pszConfigFilePath;
    delete[] pszPlayerAnimsName;
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

    // Fallback to anim_prefix
    CopyAllocString( kv->GetString( "player_anims", kv->GetString( "anim_prefix" ) ), &pszPlayerAnimsName );

    CopyAllocString( kv->GetString( "viewmodel" ), &pszModel_View );
    CopyAllocString( kv->GetString( "worldmodel" ), &pszModel_World );

    CopyAllocString( kv->GetString( "primary_ammo" ), &pszAmmoName );


    flReloadTime = kv->GetFloat( "reloadtime", -1.0f );


    nClipSize = kv->GetInt( "clip_size", WEAPON_NOCLIP );
    nDefaultClip = kv->GetInt( "default_clip", 0 );


    auto* primattack = kv->FindKey( "PrimaryAttack" );
    if ( primattack )
    {
        primary.FromKeyValues( primattack );
    }


    auto* secattack = kv->FindKey( "SecondaryAttack" );
    if ( secattack )
    {
        secondary.FromKeyValues( secattack );
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
        CopyAllocString( snddata->GetString( "double_shot" ), &pszSounds[WeaponSound_t::WPN_DOUBLE] );
        CopyAllocString( snddata->GetString( "reload" ), &pszSounds[WeaponSound_t::RELOAD] );
        CopyAllocString( snddata->GetString( "melee_hit" ), &pszSounds[WeaponSound_t::MELEE_HIT] );
        CopyAllocString( snddata->GetString( "deploy" ), &pszSounds[WeaponSound_t::DEPLOY] );
        CopyAllocString( snddata->GetString( "special1" ), &pszSounds[WeaponSound_t::SPECIAL1] );
    }


    // Client
#ifdef CLIENT_DLL
    // Texture data
    auto* texdata = kv->FindKey( "TextureData" );
    if ( texdata )
    {
        CopyIcon( texdata->FindKey( "weapon_s" ), &pIconActive );
        CopyIcon( texdata->FindKey( "weapon" ), &pIconInactive );
        CopyIcon( texdata->FindKey( "ammo" ), &pIconAmmo );

        if ( pIconActive )
        {
            auto* pHudHR = CZMHudItemHistory::GetInstance();
            if ( pHudHR )
            {
                pHudHR->SetHistoryGap( pIconActive->Height() );
            }
        }
    }


    iCrosshair = g_ZMCrosshairs.FindCrosshairByName( kv->GetString( "crosshair" ) );
#endif // CLIENT_DLL



    bUseHands = kv->GetBool( "usenewhands", true );




    if ( kv->GetBool( "attackonladder", true ) )
        fFlags |= WEPFLAG_ATTACK_ONLADDER;
    if ( kv->GetBool( "attackinwater", true ) )
        fFlags |= WEPFLAG_ATTACK_INWATER;
    if ( kv->GetBool( "reloadonladder", true ) )
        fFlags |= WEPFLAG_RELOAD_ONLADDER;
}

KeyValues* CZMBaseWeaponConfig::ToKeyValues() const
{
    KeyValues* kv = new KeyValues( "WeaponData" );




    kv->SetString( "printname", pszPrintName );
    kv->SetString( "player_anims", pszPlayerAnimsName );
    kv->SetString( "viewmodel", pszModel_View );
    kv->SetString( "worldmodel", pszModel_World );

    kv->SetString( "primary_ammo", pszAmmoName );


    kv->SetFloat( "reloadtime", flReloadTime );

    kv->SetInt( "clip_size", nClipSize );
    kv->SetInt( "default_clip", nDefaultClip );


    auto* primattack = kv->FindKey( "PrimaryAttack", true );
    if ( primattack )
    {
        primary.ToKeyValues( primattack );
    }


    auto* secattack = kv->FindKey( "SecondaryAttack", true );
    if ( secattack )
    {
        secondary.ToKeyValues( secattack );
    }


    kv->SetFloat( "accuracy_increase", flAccuracyIncrease );
    kv->SetFloat( "accuracy_decrease", flAccuracyDecrease );
    kv->SetFloat( "penetration_dmgmult", flPenetrationDmgMult );
    kv->SetFloat( "penetration_maxdist", flMaxPenetrationDist );
    kv->SetFloat( "penetration_max", nMaxPenetrations );
    
    
    kv->SetInt( "bucket", iSlot );
    kv->SetInt( "bucket_position", iPosition );
    kv->SetInt( "importance", nWeight );


    //
    // Sounds
    //
    auto* snddata = kv->FindKey( "SoundData", true );

    snddata->SetString( "empty", pszSounds[WeaponSound_t::EMPTY] );
    snddata->SetString( "single_shot", pszSounds[WeaponSound_t::SINGLE] );
    snddata->SetString( "double_shot", pszSounds[WeaponSound_t::WPN_DOUBLE] );
    snddata->SetString( "reload", pszSounds[WeaponSound_t::RELOAD] );
    snddata->SetString( "melee_hit", pszSounds[WeaponSound_t::MELEE_HIT] );
    snddata->SetString( "deploy", pszSounds[WeaponSound_t::DEPLOY] );
    snddata->SetString( "special1", pszSounds[WeaponSound_t::SPECIAL1] );


    //
    // Client stuff
    //
#ifdef CLIENT_DLL
    // Texture stuff
    auto* texdata = kv->FindKey( "TextureData", true );

    if ( pIconActive )
    {
        IconToKv( texdata->FindKey( "weapon_s", true ), pIconActive );
    }

    if ( pIconInactive )
    {
        IconToKv( texdata->FindKey( "weapon", true ), pIconInactive );
    }

    if ( pIconAmmo )
    {
        IconToKv( texdata->FindKey( "ammo", true ), pIconAmmo );
    }


    // Crosshair
    auto* pCrosshair = g_ZMCrosshairs.GetCrosshairByIndex( iCrosshair );
    if ( pCrosshair )
    {
        kv->SetString( "crosshair", pCrosshair->GetName() );
    }
#endif // CLIENT_DLL
    


    kv->SetBool( "usenewhands", bUseHands );

    kv->SetBool( "attackonladder", (fFlags & WEPFLAG_ATTACK_ONLADDER) ? true : false );
    kv->SetBool( "reloadonladder", (fFlags & WEPFLAG_RELOAD_ONLADDER) ? true : false );

    kv->SetBool( "attackinwater", (fFlags & WEPFLAG_ATTACK_INWATER) ? true : false );

    return kv;
}

#ifdef CLIENT_DLL
void CZMBaseWeaponConfig::IconToKv( KeyValues* kv, CHudTexture* icon )
{
    char c[2];
    c[0] = icon->cCharacterInFont;
    c[1] = NULL;

    kv->SetString( "character", c );
    kv->SetString( "font", icon->szTextureFile );
}
#endif

void CZMBaseWeaponConfig::VectorToKv( KeyValues* kv, const char* name, const Vector& vec )
{
    char buffer[128];
    Q_snprintf( buffer, sizeof( buffer ), "%f %f %f", vec.x, vec.y, vec.z );

    kv->SetString( name, buffer );
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
    // Spread is diameter in degrees in the config
    // We want to transfer this to radius in radians.
    Vector temp = vec3_origin;
    CopyVector( data, temp );

    for ( int i = 0; i < 3; i++ )
        spread[i] = sinf( DEG2RAD( temp[i] / 2.0f ) );
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
#endif // CLIENT_DLL

//
// Overrides only specific members, we don't want to override everything.
//
bool CZMBaseWeaponConfig::OverrideFromConfig( KeyValues* kv )
{
    const char* pszTmp;
    
#ifdef GAME_DLL
    OVERRIDE_FROM_WEPCONFIG( kv, clip_size, nClipSize );


    auto* primattack = kv->FindKey( "PrimaryAttack" );
    if ( primattack )
    {
        OverrideAttack( primattack, primary );
    }


    auto* secattack = kv->FindKey( "SecondaryAttack" );
    if ( secattack )
    {
        OverrideAttack( secattack, secondary );
    }
#endif // GAME_DLL


    //
    // Sounds
    //
    auto* snddata = kv->FindKey( "SoundData" );
    if ( snddata )
    {
        pszTmp = snddata->GetString( "empty", nullptr );
        if ( pszTmp )
            CopyAllocString( pszTmp, &pszSounds[WeaponSound_t::EMPTY] );

        pszTmp = snddata->GetString( "single_shot", nullptr );
        if ( pszTmp )
            CopyAllocString( pszTmp, &pszSounds[WeaponSound_t::SINGLE] );

        pszTmp = snddata->GetString( "double_shot", nullptr );
        if ( pszTmp )
            CopyAllocString( pszTmp, &pszSounds[WeaponSound_t::WPN_DOUBLE] );

        pszTmp = snddata->GetString( "reload", nullptr );
        if ( pszTmp )
            CopyAllocString( pszTmp, &pszSounds[WeaponSound_t::RELOAD] );

        pszTmp = snddata->GetString( "melee_hit", nullptr );
        if ( pszTmp )
            CopyAllocString( pszTmp, &pszSounds[WeaponSound_t::MELEE_HIT] );

        pszTmp = snddata->GetString( "deploy", nullptr );
        if ( pszTmp )
            CopyAllocString( pszTmp, &pszSounds[WeaponSound_t::DEPLOY] );

        pszTmp = snddata->GetString( "special1", nullptr );
        if ( pszTmp )
            CopyAllocString( pszTmp, &pszSounds[WeaponSound_t::SPECIAL1] );
    }


    // Fallback to anim_prefix value
    pszTmp = kv->GetString( "player_anims", kv->GetString( "anim_prefix", nullptr ) );
    if ( pszTmp )
        CopyAllocString( pszTmp, &pszPlayerAnimsName );

    pszTmp = kv->GetString( "primary_ammo", nullptr );
    if ( pszTmp )
        CopyAllocString( pszTmp, &pszAmmoName );


    

#ifdef CLIENT_DLL
    pszTmp = kv->GetString( "printname", nullptr );
    if ( pszTmp )
        CopyAllocString( pszTmp, &pszPrintName );

    int hands = kv->GetInt( "usenewhands", -1 );
    if ( hands != -1 )
        bUseHands = ( hands > 0 ) ? true : false;
#endif


    // Shared
    pszTmp = kv->GetString( "viewmodel", nullptr );
    if ( pszTmp )
    {
        CopyAllocString( pszTmp, &pszModel_View );
        bOverriddenViewmodel = true;
    }

    pszTmp = kv->GetString( "worldmodel", nullptr );
    if ( pszTmp )
    {
        CopyAllocString( pszTmp, &pszModel_World );
        bOverriddenWorldmodel = true;
    }

    return true;
}

void CZMBaseWeaponConfig::OverrideAttack( KeyValues* kv, ZMAttackConfig_t& attack )
{
    const float invalidf =    -1337.0f;


    const char* pszTemp;
    Vector vecTemp;
    Vector vecInvalid( invalidf, invalidf, invalidf );

    OVERRIDE_FROM_WEPCONFIG_F( kv, damage, attack.flDamage );
    OVERRIDE_FROM_WEPCONFIG_F( kv, firerate, attack.flFireRate );
    OVERRIDE_FROM_WEPCONFIG_F( kv, range, attack.flRange );

    pszTemp = kv->GetString( "spread", nullptr );
    if ( pszTemp )
        ComputeSpread( pszTemp, attack.vecSpread );


    vecTemp = vecInvalid;
    CopyVector( kv->GetString( "viewpunch_min" ), vecTemp );
    if ( vecTemp != vecInvalid )
        attack.vecViewPunch_Min = vecTemp;

    vecTemp = vecInvalid;
    CopyVector( kv->GetString( "viewpunch_max" ), vecTemp );
    if ( vecTemp != vecInvalid )
        attack.vecViewPunch_Max = vecTemp;
}

void CZMBaseWeaponConfig::OnCustomConfigLoaded()
{
    // Custom weapons aren't "overridden" as they are shared.
    bOverriddenViewmodel = false;
    bOverriddenWorldmodel = false;
}

//
//
//
CZMWepConfigReg::CZMWepConfigReg( WeaponConfigSlot_t slot, CreateWeaponConfigFn fn )
{
    auto* pSystem = GetWeaponConfigSystem();
    pSystem->RegisterBaseConfig( slot, fn );
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
        m_ConfigRegisters[i].fn = []( const char* wepname, const char* configpath ) { return new CZMBaseWeaponConfig( wepname, configpath ); };
    }


    //
    // Add new weapons here.
    //
    m_ConfigRegisters[ZMCONFIGSLOT_FISTSCARRY].pszWeaponName = "weapon_zm_fistscarry";
    m_ConfigRegisters[ZMCONFIGSLOT_IMPROVISED].pszWeaponName = "weapon_zm_improvised";
    m_ConfigRegisters[ZMCONFIGSLOT_SLEDGE].pszWeaponName = "weapon_zm_sledge";
    m_ConfigRegisters[ZMCONFIGSLOT_FIREAXE].pszWeaponName = "weapon_zm_fireaxe";
    m_ConfigRegisters[ZMCONFIGSLOT_PISTOL].pszWeaponName = "weapon_zm_pistol";
    m_ConfigRegisters[ZMCONFIGSLOT_PISTOL_GLOCK].pszWeaponName = "weapon_zm_glock";
    m_ConfigRegisters[ZMCONFIGSLOT_REVOLVER].pszWeaponName = "weapon_zm_revolver";
    m_ConfigRegisters[ZMCONFIGSLOT_SNUBNOSE].pszWeaponName = "weapon_zm_snubnose";
    m_ConfigRegisters[ZMCONFIGSLOT_SHOTGUN].pszWeaponName = "weapon_zm_shotgun";
    m_ConfigRegisters[ZMCONFIGSLOT_SHOTGUNSPORTING].pszWeaponName = "weapon_zm_shotgun_sporting";
    m_ConfigRegisters[ZMCONFIGSLOT_MAC10].pszWeaponName = "weapon_zm_mac10";
    m_ConfigRegisters[ZMCONFIGSLOT_AR9MM].pszWeaponName = "weapon_zm_ar9mm";
    m_ConfigRegisters[ZMCONFIGSLOT_RIFLE].pszWeaponName = "weapon_zm_rifle";
    m_ConfigRegisters[ZMCONFIGSLOT_R700].pszWeaponName = "weapon_zm_r700";
    m_ConfigRegisters[ZMCONFIGSLOT_MOLOTOV].pszWeaponName = "weapon_zm_molotov";
    m_ConfigRegisters[ZMCONFIGSLOT_PIPEBOMB].pszWeaponName = "weapon_zm_pipebomb";


    InitPlayerAnimMap();
}

CZMWeaponConfigSystem::~CZMWeaponConfigSystem()
{
    for ( int i = 0; i < ARRAYSIZE( m_pConfigs ); i++ )
    {
        m_pConfigs[i] = nullptr;
    }
}

bool CZMWeaponConfigSystem::IsDebugging()
{
    return zm_sv_debug_weaponconfig.GetBool();
}

void CZMWeaponConfigSystem::PostInit()
{
#ifdef CLIENT_DLL
    ListenForGameEvent( "reload_weapon_config" );
#endif

    InitBaseConfigs();
}

void CZMWeaponConfigSystem::LevelInitPreEntity()
{
    ClearCustomConfigs();
}

#ifdef CLIENT_DLL
void CZMWeaponConfigSystem::FireGameEvent( IGameEvent* event )
{
    ReloadConfigs();
}
#endif

void CZMWeaponConfigSystem::InitBaseConfigs()
{
    for ( int i = 0; i < ARRAYSIZE( m_ConfigRegisters ); i++ )
    {
        m_pConfigs[i] = LoadConfigFromFile( m_ConfigRegisters[i].pszWeaponName, m_ConfigRegisters[i].fn );
    }
}

void CZMWeaponConfigSystem::ClearCustomConfigs()
{
    for ( int i = ZMCONFIGSLOT_CUSTOM_START; i < ARRAYSIZE( m_pConfigs ); i++ )
    {
        delete m_pConfigs[i];
        m_pConfigs[i] = nullptr;
    }
}

bool CZMWeaponConfigSystem::IsSlotRegistered( WeaponConfigSlot_t slot ) const
{
    return m_pConfigs[slot] != nullptr;
}

void CZMWeaponConfigSystem::ReloadConfigs()
{
#ifdef GAME_DLL
    // Tell clients to reload theirs as well.
    IGameEvent* pEvent = gameeventmanager->CreateEvent( "reload_weapon_config", true );
    if ( pEvent )
    {
        gameeventmanager->FireEvent( pEvent, true );
    }
#endif


    // Reload base configs.
    for ( int i = 0; i < ARRAYSIZE( m_ConfigRegisters ); i++ )
    {
        delete m_pConfigs[i];
    }

    InitBaseConfigs();


    // Reload custom configs.
    for ( int i = ZMCONFIGSLOT_CUSTOM_START; i < ARRAYSIZE( m_pConfigs ); i++ )
    {
        auto* pOldConfig = m_pConfigs[i];

        if ( !pOldConfig )
        {
            continue;
        }


        m_pConfigs[i] = nullptr;


        auto slot = FindBaseSlotByClassname( pOldConfig->pszWeaponName );
        if ( slot != ZMCONFIGSLOT_INVALID )
        {
            m_pConfigs[i] = LoadCustomConfigFromFile( slot, pOldConfig->pszConfigFilePath );
        }

        if ( m_pConfigs[i] != nullptr )
        {
            // Remove old config because we we're loaded successfully.
            delete pOldConfig;
        }
        else
        {
            // Didn't work, reset back to old!
            m_pConfigs[i] = pOldConfig;

            DevWarning( "Failed to reload custom config '%s'!\n", pOldConfig->pszConfigFilePath );
        }
    }
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

void CZMWeaponConfigSystem::RegisterBaseConfig( WeaponConfigSlot_t slot, CreateWeaponConfigFn fn )
{
    m_ConfigRegisters[slot].fn = fn;
}

WeaponConfigSlot_t CZMWeaponConfigSystem::RegisterCustomWeapon( WeaponConfigSlot_t baseslot, const char* filename )
{
    if ( baseslot <= ZMCONFIGSLOT_INVALID || baseslot >= ZMCONFIGSLOT_MAX )
    {
        return baseslot;
    }


    Assert( baseslot < ZMCONFIGSLOT_REGISTERED_END );


    auto slot = FindEmptyCustomConfigSlot();
    // No more slots open!
    if ( slot == ZMCONFIGSLOT_INVALID )
    {
        return baseslot;
    }


    char file[256];
    Q_snprintf( file, sizeof( file ), CONFIG_DIR"/%s.txt", filename );


    // We don't need to load this config if we have it loaded already!
    auto existingslot = FindCustomConfigByConfigPath( file );
    if ( existingslot != ZMCONFIGSLOT_INVALID )
    {
        return existingslot;
    }


    auto* pConfig = LoadCustomConfigFromFile( baseslot, file );

    m_pConfigs[slot] = pConfig;

    return pConfig ? slot : baseslot;
}

const CZMBaseWeaponConfig* CZMWeaponConfigSystem::GetConfigBySlot( WeaponConfigSlot_t slot )
{
    if ( slot <= ZMCONFIGSLOT_INVALID || slot >= ZMCONFIGSLOT_MAX )
    {
        static CZMBaseWeaponConfig config( "", "" );

        return &config;
    }

    return m_pConfigs[slot];
}

WeaponConfigSlot_t CZMWeaponConfigSystem::FindCustomConfigByConfigPath( const char* configpath ) const
{
    for ( int i = ZMCONFIGSLOT_CUSTOM_START; i < ZMCONFIGSLOT_MAX; i++ )
    {
        if ( m_pConfigs[i] != nullptr && Q_stricmp( configpath, m_pConfigs[i]->pszConfigFilePath ) == 0 )
        {
            return (WeaponConfigSlot_t)i;
        }
    }

    return ZMCONFIGSLOT_INVALID;
}

WeaponConfigSlot_t CZMWeaponConfigSystem::FindEmptyCustomConfigSlot() const
{
    for ( int i = ZMCONFIGSLOT_CUSTOM_START; i < ZMCONFIGSLOT_MAX; i++ )
    {
        if ( !m_pConfigs[i] )
            return (WeaponConfigSlot_t)i;
    }

    return ZMCONFIGSLOT_INVALID;
}

WeaponConfigSlot_t CZMWeaponConfigSystem::FindBaseSlotByClassname( const char* classname ) const
{
    for ( int i = 0; i < ARRAYSIZE( m_ConfigRegisters ); i++ )
    {
        if ( Q_stricmp( classname, m_ConfigRegisters[i].pszWeaponName ) == 0 )
        {
            return (WeaponConfigSlot_t)i;
        }
    }

    return ZMCONFIGSLOT_INVALID;
}

const char* CZMWeaponConfigSystem::GetBaseClassname( WeaponConfigSlot_t baseslot ) const
{
    return m_ConfigRegisters[baseslot].pszWeaponName;
}

//CreateWeaponConfigFn* CZMWeaponConfigSystem::FindWeaponCreator( const char* classname ) const
//{
//    for ( int i = 0; i < ARRAYSIZE( m_ConfigRegisters ); i++ )
//    {
//        if ( Q_stricmp( classname, m_ConfigRegisters[i].pszWeaponName ) == 0 )
//        {
//            return m_ConfigRegisters[i].fn;
//        }
//    }
//
//    return nullptr;
//}

CZMBaseWeaponConfig* CZMWeaponConfigSystem::LoadConfigFromFile( const char* szWeaponName, CreateWeaponConfigFn fn )
{
    DevMsg( "Loading weapon config '%s' (%s)...\n", szWeaponName, gpGlobals->IsClient() ? "CLIENT" : "SERVER" );


    char file[256];
    KeyValues* kv;
    
    
    Q_snprintf( file, sizeof( file ), CONFIG_DIR"/%s.txt", szWeaponName );

    kv = new KeyValues( "WeaponData" );
    if ( !kv->LoadFromFile( filesystem, file ) )
    {
        Warning( "Couldn't load weapon config '%s'!\n", file );

        kv->deleteThis();
        return nullptr;
    }


    auto* pConfig = ( fn != nullptr ) ? (fn( szWeaponName, file )) : (new CZMBaseWeaponConfig( szWeaponName, file ));
    pConfig->LoadFromConfig( kv );

    kv->deleteThis();

    //
    // Override
    //

    // Allow client and server to separate their files.
    // This allows users to change client and server options separately.
    const char* postfixes[] = {
        "_custom",
#ifdef GAME_DLL
        "_custom_server"
#else
        "_custom_client"
#endif
    };

    for ( int i = 0; i < ARRAYSIZE( postfixes ); i++ )
    {
        Q_snprintf( file, sizeof( file ), CONFIG_DIR"/%s%s.txt", szWeaponName, postfixes[i] );

        kv = new KeyValues( "WeaponData" );
        if ( kv->LoadFromFile( filesystem, file, "MOD" ) )
        {
            pConfig->OverrideFromConfig( kv );

            i = ARRAYSIZE( postfixes ); // Break after this.
        }

        kv->deleteThis();
    }


    return pConfig;
}

CZMBaseWeaponConfig* CZMWeaponConfigSystem::LoadCustomConfigFromFile( WeaponConfigSlot_t baseslot, const char* filepath )
{
    DevMsg( "Loading custom weapon config '%s'...\n", filepath );


    auto* kv = new KeyValues( "WeaponData" );
    if ( !kv->LoadFromFile( filesystem, filepath ) )
    {
        kv->deleteThis();

        Warning( "Couldn't load weapon config '%s'!\n", filepath );
        return nullptr;
    }



    auto* pBaseConfig = m_pConfigs[baseslot];
    Assert( pBaseConfig );

    auto* basekv = pBaseConfig->ToKeyValues();


    const char* wepname = m_ConfigRegisters[baseslot].pszWeaponName;

    auto* pConfig = m_ConfigRegisters[baseslot].fn( wepname, filepath );


    pConfig->LoadFromConfig( basekv );

    pConfig->OverrideFromConfig( kv );

    pConfig->OnCustomConfigLoaded();


    basekv->deleteThis();
    kv->deleteThis();


    return pConfig;
}
