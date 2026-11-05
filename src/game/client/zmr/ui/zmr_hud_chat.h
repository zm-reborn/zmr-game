#pragma once


#include <hud_basechat.h>


class CHudChatLine : public CBaseHudChatLine
{
    DECLARE_CLASS_SIMPLE( CHudChatLine, CBaseHudChatLine );

public:
    CHudChatLine( vgui::Panel* parent, const char* panelName ) : CBaseHudChatLine( parent, panelName ) {}

    virtual void    ApplySchemeSettings( vgui::IScheme* pScheme ) override;
};

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
class CHudChatInputLine : public CBaseHudChatInputLine
{
    DECLARE_CLASS_SIMPLE( CHudChatInputLine, CBaseHudChatInputLine );
    
public:
    CHudChatInputLine( CBaseHudChat* parent, char const* panelName ) : CBaseHudChatInputLine( parent, panelName ) {}

    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) override;
};

class CHudChat : public CBaseHudChat
{
    DECLARE_CLASS_SIMPLE( CHudChat, CBaseHudChat );

public:
    CHudChat( const char* pElementName );

    virtual void CreateChatInputLine() override;
    virtual void CreateChatLines( void ) override;

    virtual void Init() override;
    virtual void Reset() override;
    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) override;

    virtual int GetChatInputOffset() override;

    virtual Color GetClientColor( int clientIndex ) override;

    virtual void MsgFunc_SayText2( bf_read &msg ) override;
};
