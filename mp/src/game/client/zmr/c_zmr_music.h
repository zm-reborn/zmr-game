#pragma once


#include "igamesystem.h"
#include "c_zmr_fmod.h"


namespace ZMMusic
{
    enum MusicState_t
    {
        MUSICSTATE_NONE = 0,

        MUSICSTATE_MAINMENU = ( 1 << 0 ),
        MUSICSTATE_LOADINGSCREEN = ( 1 << 1 ),
        MUSICSTATE_INGAME = ( 1 << 2 )
    };


    //
    class CMusic
    {
    public:
        CMusic( MusicState_t musicStateFlags, FMODSoundHandle_t hndl );
        ~CMusic();


        bool IsPartOfMusicState( MusicState_t state ) const;
        bool IsReady() const;

        bool PlayMe( FMODChannelHandle_t& handle );

    private:
        MusicState_t        m_fMusicState;
        FMODSoundHandle_t   m_SndHndl;
    };
    //


    //
    class CActiveChannel
    {
    public:
        CActiveChannel( MusicState_t musicState, CMusic* pMusic );
        ~CActiveChannel();


        void FadeOut( float fadeouttime );
        bool IsDone() const;

        MusicState_t GetMusicState() const;

    private:
        MusicState_t            m_iMusicState;
        FMODChannelHandle_t     m_ChannelHndl;
    };
    //


    //
    class CZMMusicManager : public CAutoGameSystemPerFrame
    {
    public:
        CZMMusicManager();
        ~CZMMusicManager();


        virtual void PostInit() OVERRIDE;

        void Release();

        virtual void Update( float frametime ) OVERRIDE;


        float GetVolume() const;

        MusicState_t GetMusicState() const;
        void SetMusicState( MusicState_t musicState );


    private:
        float m_flLastMusicVolume;

        void RegisterMusic();


        CUtlVector<CMusic*> m_vpMusic;
        CUtlVector<CActiveChannel*> m_vpChannels;

        MusicState_t m_iCurMusicState;
    };

    extern CZMMusicManager g_ZMMusicManager;
    //
}