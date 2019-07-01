#include "cbase.h"

#include "filesystem.h"
#include "IGameUIFuncs.h"

#include <vgui/ILocalize.h>
#include <igameresources.h>
#include <vgui_controls/Panel.h>


#include "c_zmr_tips.h"


// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


extern IGameUIFuncs* gameuifuncs; // for key binding details


#define DEF_TIP_TEAM            0
#define DEF_TIP_TIME            5.0f
#define DEF_TIP_PRIORITY        0




CON_COMMAND( zm_hudtooltipsaveshown, "Saves shown tool-tips to a file." )
{
    g_ZMTipSystem.SaveUsed();
}


//
CZMTip::CZMTip( KeyValues* kv, int index )
{
    m_iIndex = index;


    m_pszName = nullptr;
    m_pszMessage = nullptr;
    m_pszIcon = nullptr;
    Reset();


    m_iTeam = kv->GetInt( "team", DEF_TIP_TEAM );
    m_flTime = kv->GetFloat( "time", DEF_TIP_TIME );
    m_iPriority = kv->GetInt( "priority", DEF_TIP_PRIORITY );
    m_nLimit = kv->GetInt( "limit" );
    m_nLimitPerGame = kv->GetInt( "limitpergame" );
    m_bQueue = kv->GetBool( "canbequeued", false );
    m_bRandom = kv->GetBool( "random", false );
    m_bPulse = kv->GetBool( "pulse", false );
    m_bSound = kv->GetBool( "sound", false );
    SetName( kv->GetName() );
    SetMessage( kv->GetString( "msg" ) );
    SetIcon( kv->GetString( "icon" ) );
    
    
    char key[32];
    for ( int i = 1;; i++ )
    {
        Q_snprintf( key, sizeof( key ), "param%i", i );

        const char* value = kv->GetString( key );

        if ( value && *value )
        {
            AddParam( value );
            continue;
        }

        break;
    }
}

void CZMTip::WriteUsed( KeyValues* kv )
{
    if ( m_nLimit < 1 ) return;


    KeyValues* pKey = kv->FindKey( m_pszName, true );

    pKey->SetInt( "shown", m_nShown );
}

void CZMTip::LoadUsed( KeyValues* kv )
{
    m_nShown = kv->GetInt( "shown" );
}

CZMTip::~CZMTip()
{
    delete[] m_pszName;
    delete[] m_pszMessage;
    delete[] m_pszIcon;
    
    m_vParams.PurgeAndDeleteElements();
}

void CZMTip::Reset()
{
    m_iTeam = DEF_TIP_TEAM;
    m_flTime = DEF_TIP_TIME;
    m_iPriority = DEF_TIP_PRIORITY;
    m_bQueue = false;
    m_bRandom = false;
    m_bPulse = false;
    m_bSound = false;

    m_nShown = 0;
    m_nShownPerGame = 0;
}

bool CZMTip::ShouldShow() const
{
    if ( m_nLimit > 0 && m_nShown >= m_nLimit ) return false;

    if ( m_nLimitPerGame > 0 && m_nShownPerGame >= m_nLimitPerGame ) return false;
    

    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();

    if ( m_iTeam != DEF_TIP_TEAM )
    {
        if ( !pLocal || pLocal->GetTeamNumber() != m_iTeam )
            return false;
    }


    return true;
}

const char* CZMTip::GetName() const
{
    return m_pszName;
}

void CZMTip::SetName( const char* name )
{
    delete[] m_pszName;

    if ( !name ) return;


    int len = strlen( name ) + 1;
    m_pszName = new char[len];
    Q_strncpy( m_pszName, name, len );
}

void CZMTip::FormatMessage( char* buffer, int len ) const
{
    if ( !m_pszMessage ) return;


    char szParam[128];
    char format[1024];
    bool formatted = false;

    const char* pFormat = m_pszMessage;

    if ( pFormat[0] == '#' )
    {
        pFormat = g_pVGuiLocalize->FindAsUTF8( m_pszMessage );
    }

    if ( !pFormat || !(*pFormat) )
        pFormat = m_pszMessage;

    Q_strncpy( format, pFormat, sizeof( format ) );

    // We can't have any '@' characters.
    Assert( strchr( format, '@' ) == nullptr );

    int nParams = m_vParams.Count();

    for ( int i = 0; i < nParams; i++ )
    {
        szParam[0] = NULL;

        zmtip_param_t* param = m_vParams[i];

        switch ( param->iParamType )
        {
        case TIPPARAMTYPE_KEY :
        {
            const char* key = vgui::Panel::KeyCodeToString( gameuifuncs->GetButtonCodeForBind( param->pszParam ) );

            Q_strncpy( szParam, ( key && *key ) ? &key[4] : "(UNBOUND)", sizeof( szParam ) );

            break;
        }
        case TIPPARAMTYPE_CVAR :
        {
            ConVar* pCon = cvar->FindVar( param->pszParam );

            if ( pCon )
                Q_strncpy( szParam, pCon->GetString(), sizeof( szParam ) );

            break;
        }
        default : break;
        }

        if ( *szParam )
        {
            char* c;
            bool bPerformedSafety = false;

            // If we have more than one parameter, we have to "hide" other '%s' or otherwise
            // shit will hit the fan.

            // Hide '%' past the first one with '@' character.
            int iParam = 0;
            for ( c = format; (c = strchr( c, '%' )) != nullptr; ++c, ++iParam )
            {
                if ( iParam == 0 )
                    continue;

                *c = '@';
                bPerformedSafety = true;
            }

            
            Q_snprintf( buffer, len, format, szParam );
            Q_strncpy( format, buffer, sizeof( format ) );


            if ( bPerformedSafety )
            {
                // Reset the '@' back to '%'
                for ( c = format; (c = strchr( c, '@' )) != nullptr; ++c )
                {
                    *c = '%';
                }
            }


            formatted = true;
        }
    }

    if ( !formatted )
        Q_strncpy( buffer, format, len );
}

const char* CZMTip::GetMessage() const
{
    return m_pszMessage;
}

void CZMTip::SetMessage( const char* msg )
{
    delete[] m_pszMessage;

    if ( !msg ) return;


    int len = strlen( msg ) + 1;
    m_pszMessage = new char[len];
    Q_strncpy( m_pszMessage, msg, len );
}

const char* CZMTip::GetIcon() const
{
    return m_pszIcon;
}

void CZMTip::SetIcon( const char* icon )
{
    delete[] m_pszIcon;

    if ( !icon ) return;


    int len = strlen( icon ) + 1;
    m_pszIcon = new char[len];
    Q_strncpy( m_pszIcon, icon, len );
}

void CZMTip::AddParam( const char* invalue )
{
    zmtip_param_t* param = new zmtip_param_t();

    char buffer[128];
    Q_strncpy( buffer, invalue, sizeof( buffer ) );


    char* del = strchr( buffer, '@' );

    if ( !del ) return;


    *del = NULL;

    char* paramvalue = del + 1;


    param->iParamType = TipNameToType( buffer );

    int len = strlen( paramvalue ) + 1;
    param->pszParam = new char[len];
    Q_strncpy( param->pszParam, paramvalue, len );

    m_vParams.AddToTail( param );
}

TipParamType_t CZMTip::TipNameToType( const char* type )
{
    if ( Q_stricmp( type, "key" ) == 0 )
    {
        return TIPPARAMTYPE_KEY;
    }
    else if ( Q_stricmp( type, "cvar" ) == 0 )
    {
        return TIPPARAMTYPE_CVAR;
    }

    return TIPPARAMTYPE_NONE;
}

void CZMTip::OnShownInGame()
{
    m_nShown++;
    m_nShownPerGame++;
}

void CZMTip::OnShownInLoadingScreen()
{
    m_nShown++;
    m_nShownPerGame++;
}
//



//
CZMTipManager::CZMTipManager( const char* name )
{
    m_pszName = name;


    LoadTips();
    LoadUsed();
}

CZMTipManager::~CZMTipManager()
{
    m_vTips.PurgeAndDeleteElements();
}

void CZMTipManager::GetTips( ZMConstTips_t& vec ) const
{
    vec.CopyArray( m_vTips.Base(), m_vTips.Count() );
}

void CZMTipManager::AddTip( KeyValues* kv )
{
    static int index = 1;

    auto* pTip = new CZMTip( kv, index );

    m_vTips.AddToTail( pTip );


    ++index;
}

void CZMTipManager::LoadTips()
{
    m_vTips.PurgeAndDeleteElements();


    char filepath[128];
    FormatFilePath( filepath, sizeof( filepath ) );


    auto* kv = new KeyValues( "Tips" );
    kv->UsesEscapeSequences( true );


    if ( kv->LoadFromFile( filesystem, filepath, "MOD" ) )
    {
        for ( auto* pKey = kv->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey() )
        {
            AddTip( pKey );
        }
    }
    else
    {
        Warning( "Couldn't load tips from file!\n" );
    }

    kv->deleteThis();
}

void CZMTipManager::LoadUsed()
{
    DevMsg( "Loading used tips...\n" );


    char filepath[128];
    FormatFilePathUsed( filepath, sizeof( filepath ) );


    auto* kv = new KeyValues( "Tips" );

    if ( kv->LoadFromFile( filesystem, filepath, "MOD" ) )
    {
        for ( auto* pKey = kv->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey() )
        {
            auto* pTip = FindTipByName( pKey->GetName() );

            if ( pTip )
            {
                pTip->LoadUsed( pKey );
            }
        }
    }

    kv->deleteThis();
}

void CZMTipManager::SaveUsed() const
{
    DevMsg( "Saving used tips...\n" );


    auto* kv = new KeyValues( "Tips" );


    for ( int i = 0; i < m_vTips.Count(); i++ )
    {
        m_vTips[i]->WriteUsed( kv );
    }


    char filepath[128];
    FormatFilePathUsed( filepath, sizeof( filepath ) );


    if ( m_vTips.Count() )
    {
        kv->SaveToFile( filesystem, filepath, "MOD" );
    }
    

    kv->deleteThis();
}

void CZMTipManager::FormatFilePath( char* buffer, int len ) const
{
    Q_snprintf( buffer, len, "resource/zmtips_%s.txt", m_pszName );
}

void CZMTipManager::FormatFilePathUsed( char* buffer, int len ) const
{
    Q_snprintf( buffer, len, "resource/zmtips_%s_used.txt", m_pszName );
}

CZMTip* CZMTipManager::FindTipByName( const char* name ) const
{
    int len = m_vTips.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( Q_stricmp( name, m_vTips[i]->GetName() ) == 0 )
        {
            return m_vTips[i];
        }
    }

    return nullptr;
}

CZMTip* CZMTipManager::FindTipByIndex( int index ) const
{
    int len = m_vTips.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( index == m_vTips[i]->GetIndex() )
        {
            return m_vTips[i];
        }
    }

    return nullptr;
}
//



//
CZMTipSystem::CZMTipSystem()
{
}

CZMTipSystem::~CZMTipSystem()
{
    m_vManagers.PurgeAndDeleteElements();
}

void CZMTipSystem::PostInit()
{
    m_vManagers.AddToTail( new CZMTipManager( "hudtips" ) );
    m_vManagers.AddToTail( new CZMTipManager( "loadingscreen" ) );
}

void CZMTipSystem::LevelShutdownPostEntity()
{
    SaveUsed();
}

CZMTipManager* CZMTipSystem::GetManagerByName( const char* name ) const
{
    int len = m_vManagers.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( Q_stricmp( name, m_vManagers[i]->GetName() ) == 0 )
        {
            return m_vManagers[i];
        }
    }

    return nullptr;
}

CZMTipManager* CZMTipSystem::GetManagerByIndex( int index ) const
{
    int len = m_vManagers.Count();
    if ( index < 0 || index >= len )
        return nullptr;

    return m_vManagers[index];
}

void CZMTipSystem::SaveUsed() const
{
    FOR_EACH_VEC( m_vManagers, i )
    {
        m_vManagers[i]->SaveUsed();
    }
}
//

CZMTipSystem g_ZMTipSystem;
