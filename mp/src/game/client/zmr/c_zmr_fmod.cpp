#include "cbase.h"
#include "filesystem.h"

#include <fmod/fmod.hpp>
#include <fmod/fmod_errors.h>

#include "c_zmr_fmod.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


FMOD::System* CFMODSystem::s_pFMODSystem = nullptr;


CFMODSystem::CFMODSystem() : CAutoGameSystemPerFrame( "ZMFModSystem" )
{
    m_bMuted = false;
    m_flMasterVolume = 1.0f;
}

CFMODSystem::~CFMODSystem()
{
    Release();
}

void CFMODSystem::Release()
{
    FOR_EACH_VEC( m_vpSounds, i )
    {
        if ( m_vpSounds[i] )
        {
            m_vpSounds[i]->release();
            m_vpSounds[i] = nullptr;
        }
    }


    if ( s_pFMODSystem )
    {
        s_pFMODSystem->close();
        s_pFMODSystem->release();

        s_pFMODSystem = nullptr;
    }
}

bool CFMODSystem::Init()
{
    FMOD_RESULT     result;
    unsigned int    version;
    void*           extradriverdata = nullptr;


    // Should probably fail init here, but FMOD isn't used for anything critical
    // right now.

    result = FMOD::System_Create( &s_pFMODSystem );
    if ( result != FMOD_OK || !s_pFMODSystem )
    {
        Warning( "Failed to create FMOD system! Error: %i | %s\n", result, FMOD_ErrorString( result ) );
        return true;
    }

    result = s_pFMODSystem->getVersion( &version );
    if ( result != FMOD_OK )
    {
        Warning( "Failed to retrieve FMOD version! Error: %i | %s\n", result, FMOD_ErrorString( result ) );
    }

    if ( version < FMOD_VERSION )
    {
        Warning( "FMOD lib version %08x doesn't match header version %08x!\n", version, FMOD_VERSION );
    }


    result = s_pFMODSystem->init( 32, FMOD_INIT_NORMAL, extradriverdata );
    if ( result != FMOD_OK )
    {
        Warning( "Failed to init FMOD system! Error: %i | %s\n", result, FMOD_ErrorString( result ) );
        return true;
    }

    return true;
}

void CFMODSystem::PostInit()
{
}

void CFMODSystem::Shutdown()
{
    Release();
}

void CFMODSystem::Update( float frametime )
{
    Assert( s_pFMODSystem );
    if ( !s_pFMODSystem ) return;


    UpdateFocusVolume();


    FMOD_RESULT result = s_pFMODSystem->update();
    if ( result != FMOD_OK )
    {
        Warning( "Failed to update FMOD system! Error: %i | %s\n", result, FMOD_ErrorString( result ) );
        return;
    }
}

//
// Update volume if window focus changes.
//
void CFMODSystem::UpdateFocusVolume()
{
    static ConVarRef snd_mute_losefocus( "snd_mute_losefocus" );

    // No losefocus mute.
    if ( !snd_mute_losefocus.GetBool() )
    {
        if ( m_bMuted )
        {
            SetMasterVolumeInternal( GetMasterVolume() );
            m_bMuted = false;
        }
        
        return;
    }


    bool bInFocus = engine->IsActiveApp();

    if ( bInFocus )
    {
        if ( m_bMuted )
        {
            SetMasterVolumeInternal( GetMasterVolume() );
            m_bMuted = false;
        }
    }
    else
    {
        if ( !m_bMuted )
        {
            SetMasterVolumeInternal( 0.0f );
            m_bMuted = true;
        }
    }
}

FMODSoundHandle_t CFMODSystem::RegisterSound( const char* path )
{
    if ( !s_pFMODSystem ) return INVALID_FMODSOUND_HANDLE;


    char fullpath[MAX_PATH];
    filesystem->RelativePathToFullPath( path, "GAME", fullpath, sizeof( fullpath ), FILTER_NONE, nullptr );

    FMOD_RESULT result;
    FMOD::Sound* pSound = nullptr;

    result = s_pFMODSystem->createSound( fullpath, FMOD_DEFAULT, 0, &pSound );
    if ( result != FMOD_OK || !pSound )
    {
        Warning( "Failed to create sound! Error: %i | %s\n", result, FMOD_ErrorString( result ) );
        return INVALID_FMODSOUND_HANDLE;
    }


    auto hndl = CreateSoundHandle();


    result = pSound->setMode( FMOD_LOOP_NORMAL );
    if ( result != FMOD_OK )
    {
        Warning( "Failed to set sound mode to loop normal! Error: %i | %s\n", result, FMOD_ErrorString( result ) );
    }


    m_vpSounds[hndl] = pSound;

    return hndl;
}


float CFMODSystem::GetMasterVolume() const
{
    return m_flMasterVolume;
}

bool CFMODSystem::IsMuted() const
{
    return m_bMuted;
}

void CFMODSystem::SetMasterVolume( float volume )
{
    if ( !IsMuted() )
    {
        SetMasterVolumeInternal( volume );
    }

    m_flMasterVolume = volume;
}

void CFMODSystem::SetMasterVolumeInternal( float volume )
{
    Assert( s_pFMODSystem );

    FMOD::ChannelGroup* pMasterGroup = nullptr;
    CFMODSystem::s_pFMODSystem->getMasterChannelGroup( &pMasterGroup );

    pMasterGroup->setVolume( volume );
}

FMODChannelHandle_t CFMODSystem::StartSound( FMODSoundHandle_t hndl, float fadeintime )
{
    Assert( m_vpSounds.IsValidIndex( hndl ) && m_vpSounds[hndl] );
    Assert( s_pFMODSystem );


    FMOD::Channel* pChannel = nullptr;

    FMOD_RESULT result = s_pFMODSystem->playSound( m_vpSounds[hndl], nullptr, true, &pChannel );
    if ( result != FMOD_OK )
    {
        return INVALID_FMODCHANNEL_HANDLE;
    }


    auto channelhndl = CreateChannelHandle();

    if ( fadeintime > 0.0f )
    {
        int samplerate = 0;
        s_pFMODSystem->getSoftwareFormat( &samplerate, nullptr, nullptr );

        unsigned long long fadein = (unsigned long long)(fadeintime * samplerate);

        unsigned long long dspclock = 0;
        pChannel->getDSPClock( nullptr, &dspclock );

        pChannel->addFadePoint( dspclock, 0.0f );
        pChannel->addFadePoint( dspclock + fadein, 1.0f );
    }



    pChannel->setPaused( false );


    m_vpChannels[channelhndl] = pChannel;

    return channelhndl;
}

bool CFMODSystem::StopSound( FMODChannelHandle_t hndl, float fadeouttime )
{
    Assert( m_vpChannels.IsValidIndex( hndl ) && m_vpChannels[hndl] );
    Assert( s_pFMODSystem );


    auto* pChannel = m_vpChannels[hndl];


    if ( fadeouttime > 0.0f )
    {
        int samplerate = 0;
        s_pFMODSystem->getSoftwareFormat( &samplerate, nullptr, nullptr );

        unsigned long long dspclock = 0;
        pChannel->getDSPClock( nullptr, &dspclock );

        unsigned long long fadeout = (unsigned long long)(fadeouttime * samplerate);

        unsigned long long dspend = dspclock + fadeout;

        pChannel->addFadePoint( dspclock, 1.0f );
        pChannel->addFadePoint( dspend, 0.0f );

        pChannel->setDelay( 0, dspend, true );
    }
    else
    {
        pChannel->stop();
    }

    m_vpChannels[hndl] = nullptr;

    return true;
}

bool CFMODSystem::IsSoundPlaying( FMODChannelHandle_t hndl ) const
{
    if ( !m_vpChannels.IsValidIndex( hndl ) || m_vpChannels[hndl] == nullptr )
        return false;


    bool playing = false;
    m_vpChannels[hndl]->isPlaying( &playing );

    return playing;
}

bool CFMODSystem::IsSoundReadyToPlay( FMODSoundHandle_t hndl ) const
{
    Assert( m_vpSounds.IsValidIndex( hndl ) && m_vpSounds[hndl] );

    FMOD_OPENSTATE state;
    m_vpSounds[hndl]->getOpenState( &state, nullptr, nullptr, nullptr );

    return state == FMOD_OPENSTATE_READY;
}

FMODSoundHandle_t CFMODSystem::CreateSoundHandle()
{
    FOR_EACH_VEC( m_vpSounds, i )
    {
        if ( m_vpSounds[i] == nullptr )
            return (FMODSoundHandle_t)i;
    }

    return (FMODSoundHandle_t)m_vpSounds.AddToTail( nullptr );
}

FMODChannelHandle_t CFMODSystem::CreateChannelHandle()
{
    FOR_EACH_VEC( m_vpChannels, i )
    {
        if ( m_vpChannels[i] == nullptr )
            return (FMODChannelHandle_t)i;
    }

    return (FMODChannelHandle_t)m_vpChannels.AddToTail( nullptr );
}

CFMODSystem g_FMODSystem;

