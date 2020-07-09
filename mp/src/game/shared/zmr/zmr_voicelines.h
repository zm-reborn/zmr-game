#pragma once



#include "zmr_player_shared.h"


struct ZMVoiceLine_t
{
    ZMVoiceLine_t( int index, const char* chatmsg, const char* cncpt, float flDelay )
    {
        m_iIndex = index;
        Q_strncpy( m_szChatMsg, chatmsg, sizeof( m_szChatMsg ) );
        Q_strncpy( m_szConcept, cncpt, sizeof( m_szConcept ) );
        m_flDelay = flDelay;
    }

    int m_iIndex;
    char m_szChatMsg[64];
    char m_szConcept[128];
    float m_flDelay;
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


    static void PlayVoiceLine( C_BasePlayer* pOrigin, const Vector* vecPos, const char* szLine, int seed = -1 );

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
