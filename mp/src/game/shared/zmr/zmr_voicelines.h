#pragma once



#include "zmr_player_shared.h"


struct ZMVoiceLine_t
{
#ifdef GAME_DLL
    ZMVoiceLine_t( int index, float delay )
    {
        m_iIndex = index;
        m_flDelay = delay;
    }

    int m_iIndex;
    float m_flDelay;
#else
    ZMVoiceLine_t( int index, const char* chatmsg, const char* snd )
    {
        m_iIndex = index;
        Q_strncpy( m_szChatMsg, chatmsg, sizeof( m_szChatMsg ) );
        Q_strncpy( m_szSoundBase, snd, sizeof( m_szSoundBase ) );
    }

    int m_iIndex;
    char m_szChatMsg[64];
    char m_szSoundBase[128];
#endif
};


class CZMVoiceLines : public CAutoGameSystem
#ifdef CLIENT_DLL
    , public CGameEventListener
#endif
{
public:
    CZMVoiceLines();
    ~CZMVoiceLines();


    virtual void PostInit() OVERRIDE;

#ifdef CLIENT_DLL
    virtual void LevelInitPreEntity() OVERRIDE;

    virtual void FireGameEvent( IGameEvent* pEvent ) OVERRIDE;



    bool IsFemale( C_ZMPlayer* pPlayer ) const;
#endif


#ifdef GAME_DLL
    void OnVoiceLine( CZMPlayer* pPlayer, int index );
#else
    void SendVoiceLine( int index );
#endif
    void LoadVoiceLines();


    ZMVoiceLine_t* FindVoiceLineByIndex( int index ) const;

private:
    CUtlVector<ZMVoiceLine_t*> m_vLines;
};

extern CZMVoiceLines* ZMGetVoiceLines();
