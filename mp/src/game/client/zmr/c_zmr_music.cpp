#include "cbase.h"

#include <fmod/fmod.hpp>
#include <fmod/fmod_errors.h>


#include "filesystem.h"

#include "c_zmr_music.h"
#include "c_zmr_fmod.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


ConVar zm_cl_music_fadein( "zm_cl_music_fadein", "2" );
ConVar zm_cl_music_fadeout( "zm_cl_music_fadeout", "2" );



#define ZMMUSIC_FILE            "resource/zmmusic.txt"

//
//
//
ZMMusic::CMusic::CMusic( MusicState_t musicStateFlags, FMODSoundHandle_t hndl ) :
    m_fMusicState( musicStateFlags ), m_SndHndl( hndl )
{
}

ZMMusic::CMusic::~CMusic()
{
}

bool ZMMusic::CMusic::IsPartOfMusicState( MusicState_t state ) const
{
    return state != MUSICSTATE_NONE && (m_fMusicState & state) == state;
}

bool ZMMusic::CMusic::IsReady() const
{
    return g_FMODSystem.IsSoundReadyToPlay( m_SndHndl );
}

bool ZMMusic::CMusic::PlayMe( FMODChannelHandle_t& handle )
{
    // Start paused
    // or volume and other stuff won't be taken into account
    handle = g_FMODSystem.StartSound( m_SndHndl, zm_cl_music_fadein.GetFloat() );

    return handle != INVALID_FMODCHANNEL_HANDLE;
}

const char* ZMMusic::CMusic::GetMusicStateName() const
{
    return MusicStateToName( m_fMusicState );
}

const char* ZMMusic::CMusic::MusicStateToName( MusicState_t state )
{
    switch ( state )
    {
    case MUSICSTATE_INGAME :        return "In-Game";
    case MUSICSTATE_LOADINGSCREEN : return "Loading Screen";
    case MUSICSTATE_MAINMENU :      return "Main Menu";
    case MUSICSTATE_NONE :          return "None";
    default:
        return "Multi state track";
    }
}


//
//
//
ZMMusic::CActiveChannel::CActiveChannel( MusicState_t musicState, CMusic* pMusic ) :
    m_iMusicState( musicState )
{
    Assert( pMusic->IsReady() );

    pMusic->PlayMe( m_ChannelHndl );
}

ZMMusic::CActiveChannel::~CActiveChannel()
{
}

void ZMMusic::CActiveChannel::FadeOut( float fadeouttime )
{
    g_FMODSystem.StopSound( m_ChannelHndl, fadeouttime );
}

ZMMusic::MusicState_t ZMMusic::CActiveChannel::GetMusicState() const
{
    return m_iMusicState;
}

bool ZMMusic::CActiveChannel::IsDone() const
{
    return !g_FMODSystem.IsSoundPlaying( m_ChannelHndl );
}



//
ZMMusic::CZMMusicManager::CZMMusicManager() : CAutoGameSystemPerFrame( "ZMMusicSystem" ),
    m_iCurMusicState( MUSICSTATE_NONE ), m_flLastMusicVolume( -1.0f )
{
}

ZMMusic::CZMMusicManager::~CZMMusicManager()
{
    Release();
}

void ZMMusic::CZMMusicManager::Release()
{
    m_vpChannels.PurgeAndDeleteElements();
    m_vpMusic.PurgeAndDeleteElements();
}

void ZMMusic::CZMMusicManager::PostInit()
{
    if ( !CFMODSystem::HasFMOD() ) return;


    RegisterMusic();
}

void ZMMusic::CZMMusicManager::RegisterMusic()
{
    auto* kv = new KeyValues( "Music" );

    if ( !kv->LoadFromFile( filesystem, ZMMUSIC_FILE ) )
    {
        Warning( "Failed to load music! Music file '%s' does not exist!\n", ZMMUSIC_FILE );

        kv->deleteThis();
        return;
    }


    // NOTE: The string should only last till kv is released.
    struct PotentialMusic_t
    {
        const char*     pszRelPath;
        MusicState_t    fMusicStates;
    };

    CUtlVector<PotentialMusic_t> list;


    for ( auto* subkv = kv->GetFirstTrueSubKey(); subkv != nullptr; subkv = subkv->GetNextTrueSubKey() )
    {
        int state = (int)MUSICSTATE_NONE;

        state |= (int)(subkv->GetInt( "mainmenu" ) ? MUSICSTATE_MAINMENU : MUSICSTATE_NONE);
        state |= (int)(subkv->GetInt( "loadingscreen" ) ? MUSICSTATE_LOADINGSCREEN : MUSICSTATE_NONE);

        list.AddToTail( { subkv->GetString( "file" ), (MusicState_t)state } );
    }

    // ZMRTODO: Randomize music here, before registering the sound
    // to prevent loading music into memory needlessly.

    FOR_EACH_VEC( list, i )
    {
        auto hndl = g_FMODSystem.RegisterSound( list[i].pszRelPath );

        if ( hndl == INVALID_FMODSOUND_HANDLE )
        {
            Warning( "Failed to register music '%s'!\n", list[i].pszRelPath );
            continue;
        }


        auto* pMusic = new CMusic( list[i].fMusicStates, hndl );

        m_vpMusic.AddToTail( pMusic );

        DevMsg( "Registered music: %s (%s)\n", list[i].pszRelPath, pMusic->GetMusicStateName() );
    }

    list.RemoveAll();
    kv->deleteThis();


    DevMsg( "Registered %i music tracks.\n", m_vpMusic.Count() );
}

float ZMMusic::CZMMusicManager::GetVolume() const
{
    // IMPORTANT: Don't call me before PostInit or this cvar won't exist.
    static ConVarRef snd_musicvolume( "snd_musicvolume" );
    return snd_musicvolume.GetFloat();
}

ZMMusic::MusicState_t ZMMusic::CZMMusicManager::GetMusicState() const
{
    return m_iCurMusicState;
}

void ZMMusic::CZMMusicManager::SetMusicState( MusicState_t musicState )
{
    if ( musicState == GetMusicState() )
        return;


    DevMsg( "Changing music state to %s!\n", CMusic::MusicStateToName( musicState ) );

    // Fade out all channels
    FOR_EACH_VEC( m_vpChannels, i )
    {
        if ( m_vpChannels[i]->IsDone() )
            continue;

        if ( m_vpChannels[i]->GetMusicState() != musicState )
        {
            m_vpChannels[i]->FadeOut( zm_cl_music_fadeout.GetFloat() );
        }
    }
    

    //
    // Find a random music for this state and play it.
    //
    CUtlVector<CMusic*> available;

    FOR_EACH_VEC( m_vpMusic, i )
    {
        if ( m_vpMusic[i]->IsPartOfMusicState( musicState ) && m_vpMusic[i]->IsReady() )
        {
            available.AddToTail( m_vpMusic[i] );
        }
    }


    if ( available.Count() > 0 )
    {
        m_vpChannels.AddToTail( new CActiveChannel(
            musicState,
            available[random->RandomInt( 0, available.Count() - 1 )] ) );
    }


    m_iCurMusicState = musicState;
}

void ZMMusic::CZMMusicManager::Update( float frametime )
{
    FOR_EACH_VEC( m_vpChannels, i )
    {
        if ( m_vpChannels[i]->IsDone() )
        {
            delete m_vpChannels[i];

             m_vpChannels.Remove( i );
            --i;
        }
    }


    if ( m_flLastMusicVolume != GetVolume() )
    {
        g_FMODSystem.SetMasterVolume( GetVolume() );
        m_flLastMusicVolume = GetVolume();
    }

    if ( engine->IsInGame() && !engine->IsLevelMainMenuBackground() )
    {
        if ( GetMusicState() != MUSICSTATE_INGAME )
        {
            SetMusicState( MUSICSTATE_INGAME );
        }
    }
    else
    {
        if ( GetMusicState() == MUSICSTATE_INGAME || GetMusicState() == MUSICSTATE_NONE )
        {
            SetMusicState( MUSICSTATE_MAINMENU );
        }
    }
}

ZMMusic::CZMMusicManager ZMMusic::g_ZMMusicManager;
