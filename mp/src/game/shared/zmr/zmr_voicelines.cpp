#include "cbase.h"
#include "filesystem.h"

#ifdef CLIENT_DLL
#include "voice_status.h"
#include "c_playerresource.h"

#include <engine/IEngineSound.h>
#include <tier3/tier3.h>
#include <vgui/ILocalize.h>
#endif

#ifdef CLIENT_DLL
#include "zmr/c_zmr_util.h"
#endif

#include "zmr_player_shared.h"
#include "zmr_voicelines.h"



#ifdef GAME_DLL
ConVar zm_sv_voiceline_madness( "zm_sv_voiceline_madness", "0", 0, "Don't do it." );
#else
ConVar zm_cl_voiceline_disablesound( "zm_cl_voiceline_disablesound", "0", FCVAR_ARCHIVE );
ConVar zm_cl_voiceline_disablechat( "zm_cl_voiceline_disablechat", "0", FCVAR_ARCHIVE );
ConVar zm_cl_voiceline_disableforzm( "zm_cl_voiceline_disableforzm", "1", FCVAR_ARCHIVE );
#endif

ConVar zm_sv_voiceline_disable( "zm_sv_voiceline_disable", "0", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED );


CZMVoiceLines::CZMVoiceLines()
{
}

CZMVoiceLines::~CZMVoiceLines()
{
    m_vLines.PurgeAndDeleteElements();
}

void CZMVoiceLines::PostInit()
{
#ifdef CLIENT_DLL
    ListenForGameEvent( "voicemenu_use" );
#endif

    LoadVoiceLines();
}

#ifdef CLIENT_DLL
void CZMVoiceLines::LevelInitPreEntity()
{
    bool prev = C_BaseEntity::IsPrecacheAllowed();
    C_BaseEntity::SetAllowPrecache( true );

    int len = m_vLines.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_vLines[i]->m_szSoundBase[0] != NULL )
        {
            char line[256];



            Q_snprintf( line, sizeof( line ), "%s.%s", m_vLines[i]->m_szSoundBase, "Male" );
            C_BaseEntity::PrecacheSound( line );

            Q_snprintf( line, sizeof( line ), "%s.%s", m_vLines[i]->m_szSoundBase, "Female" );
            C_BaseEntity::PrecacheSound( line );
        }
    }

    C_BaseEntity::SetAllowPrecache( prev );
}
#endif

void CZMVoiceLines::LoadVoiceLines()
{
    m_vLines.PurgeAndDeleteElements();


    KeyValues* kv = new KeyValues( "VoiceLines" );
    if ( !kv->LoadFromFile( filesystem, "resource/zmvoicelines.txt" ) )
    {
        kv->deleteThis();
        return;
    }


    KeyValues* data = kv->GetFirstSubKey();
    do
    {
        int index = data->GetInt( "index", -1 );
        if ( index <= -1 )
            continue;

#ifdef CLIENT_DLL
        const char* chatmsg = data->GetString( "chatmsg" );
        const char* snd = data->GetString( "snd" );
        if ( !(*snd) && !(*chatmsg) )
            continue;

        m_vLines.AddToTail( new ZMVoiceLine_t( index, chatmsg, snd ) );
#else
        float delay = fabs( data->GetFloat( "delay", 3.0f ) );

        m_vLines.AddToTail( new ZMVoiceLine_t( index, delay ) );
#endif
    }
    while ( (data = data->GetNextKey()) != nullptr );




    kv->deleteThis();
}

#ifdef CLIENT_DLL
void CZMVoiceLines::FireGameEvent( IGameEvent* pEvent )
{
    if ( Q_strcmp( pEvent->GetName(), "voicemenu_use" ) == 0 )
    {
        C_ZMPlayer* pLocal = C_ZMPlayer::GetLocalPlayer();
        if ( !pLocal )
            return;


        ZMVoiceLine_t* pLine = FindVoiceLineByIndex( pEvent->GetInt( "voiceline", -1 ) );
        if ( !pLine )
            return;

        int index = engine->GetPlayerForUserID( pEvent->GetInt( "userid", 0 ) );
        if ( index <= 0 )
            return;


        // If the player is muted, don't do anything.
        bool bIsMuted = GetLocalPlayerIndex() != index && GetClientVoiceMgr()->IsPlayerBlocked( index );

        // Disable chat if we're the ZM as well.
        bool bDisableChat = zm_cl_voiceline_disablechat.GetBool()
                        ||  (pLocal->IsZM() && zm_cl_voiceline_disableforzm.GetBool());


        C_ZMPlayer* pPlayer = ToZMPlayer( UTIL_PlayerByIndex( index ) );
            


        // Play the sound
        if ( pLine->m_szSoundBase[0] != NULL && !zm_cl_voiceline_disablesound.GetBool() && !bIsMuted )
        {
            const char* gender = IsFemale( pPlayer ) ? "Female" : "Male";

            char line[256];
            Q_snprintf( line, sizeof( line ), "%s.%s", pLine->m_szSoundBase, gender );

            if ( pPlayer && !pPlayer->IsDormant() )
            {
                pPlayer->EmitSound( line );
            }
            else
            {
                Vector pos;
                pos.x = pEvent->GetFloat( "pos_x" );
                pos.y = pEvent->GetFloat( "pos_y" );
                pos.z = pEvent->GetFloat( "pos_z" );

                CSingleUserRecipientFilter filter( pLocal );
                pLocal->EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, line, &pos, 0.0f, nullptr );
            }
        }

        
        // Print chat message
        if ( pLine->m_szChatMsg[0] != NULL && !bDisableChat )
        {
            // We'll have to convert ansi -> unicode -> ansi to get localization to work :(
            wchar_t buf[512];
            wchar_t playerName[128];


            g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( index ), playerName, sizeof( playerName ) );
            


            g_pVGuiLocalize->ConstructString( buf, sizeof( buf ), g_pVGuiLocalize->Find( "ZM_Chat_Voice" ),
                2,
                playerName,
                g_pVGuiLocalize->Find( pLine->m_szChatMsg ) );


            char ansi[512];
            g_pVGuiLocalize->ConvertUnicodeToANSI( buf, ansi, sizeof( ansi ) );


            ZMClientUtil::ChatPrint( index, false, ansi );
        }
    }
}

bool CZMVoiceLines::IsFemale( C_ZMPlayer* pPlayer ) const
{
    if ( !pPlayer )
        return false;

    int modelIndex = pPlayer->GetModelIndex();
    if ( modelIndex <= -1 )
        return false;


    const model_t* pModel = modelinfo->GetModel( modelIndex );
    if ( !pModel )
        return false;


    const char* modelname = modelinfo->GetModelName( pModel );
    if ( !modelname || !(*modelname) )
        return false;


    return Q_strstr( modelname, "female" ) != nullptr;
}
#endif

#ifdef GAME_DLL
void CZMVoiceLines::OnVoiceLine( CZMPlayer* pPlayer, int index )
{
    if ( zm_sv_voiceline_disable.GetBool() )
        return;


    ZMVoiceLine_t* pLine = FindVoiceLineByIndex( index );
    if ( !pLine )
        return;


    if ( !zm_sv_voiceline_madness.GetBool() )
    {
        if ( pPlayer->GetNextVoiceLineTime() > gpGlobals->curtime )
            return;
    }


    pPlayer->SetNextVoiceLineTime( gpGlobals->curtime + pLine->m_flDelay );


    Vector origin = pPlayer->EyePosition();


    IGameEvent* pEvent = gameeventmanager->CreateEvent( "voicemenu_use", true );
    if ( pEvent )
    {
        pEvent->SetInt( "userid", pPlayer->GetUserID() );
        pEvent->SetInt( "voiceline", index );
        pEvent->SetFloat( "pos_x", origin.x );
        pEvent->SetFloat( "pos_y", origin.y );
        pEvent->SetFloat( "pos_z", origin.z );
        gameeventmanager->FireEvent( pEvent, false );
    }
}
#else
void CZMVoiceLines::SendVoiceLine( int index )
{
    if ( zm_sv_voiceline_disable.GetBool() )
        return;


    engine->ClientCmd( VarArgs( "zm_cmd_voicemenu %i", index ) );
}
#endif


ZMVoiceLine_t* CZMVoiceLines::FindVoiceLineByIndex( int index ) const
{
    int len = m_vLines.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_vLines[i]->m_iIndex == index )
            return m_vLines[i];
    }

    return nullptr;
}


static CZMVoiceLines g_ZMVoiceLines;

CZMVoiceLines* ZMGetVoiceLines()
{
    return &g_ZMVoiceLines;
}
