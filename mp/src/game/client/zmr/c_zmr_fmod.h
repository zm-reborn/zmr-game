#pragma once


#include "igamesystem.h"


typedef int FMODSoundHandle_t;
typedef int FMODChannelHandle_t;

#define INVALID_FMODSOUND_HANDLE        ((FMODSoundHandle_t)-1)
#define INVALID_FMODCHANNEL_HANDLE      ((FMODChannelHandle_t)-1)

namespace FMOD
{
    class Channel;
    class Sound;
    class System;
}

class CFMODSystem : public CAutoGameSystemPerFrame
{
public:
    CFMODSystem();
    ~CFMODSystem();

    virtual bool Init() OVERRIDE;
    virtual void PostInit() OVERRIDE;
    virtual void Shutdown() OVERRIDE;

    virtual void Update( float frametime ) OVERRIDE;


    static bool HasFMOD() { return s_pFMODSystem != nullptr; }


    FMODSoundHandle_t   RegisterSound( const char* relpath );

    float               GetMasterVolume() const;
    void                SetMasterVolume( float volume );

    FMODChannelHandle_t StartSound( FMODSoundHandle_t hndl, float fadeintime = 0.0f );
    bool                StopSound( FMODChannelHandle_t hndl, float fadeouttime = 0.0f );

    bool                IsSoundPlaying( FMODChannelHandle_t hndl ) const;
    bool                IsSoundReadyToPlay( FMODSoundHandle_t hndl ) const;

private:
    void Release();

    FMODSoundHandle_t   CreateSoundHandle();
    FMODChannelHandle_t CreateChannelHandle();


    CUtlVector<FMOD::Sound*>    m_vpSounds;
    CUtlVector<FMOD::Channel*>  m_vpChannels;

    static FMOD::System*        s_pFMODSystem;
};

extern CFMODSystem g_FMODSystem;
