#pragma once


#include <hud_basechat.h>


class CHudChatLine : public CBaseHudChatLine
{
    DECLARE_CLASS_SIMPLE( CHudChatLine, CBaseHudChatLine );

public:
    CHudChatLine( vgui::Panel* parent, const char* panelName ) : CBaseHudChatLine( parent, panelName ) {}

    virtual void    ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;
};

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
class CHudChatInputLine : public CBaseHudChatInputLine
{
    DECLARE_CLASS_SIMPLE( CHudChatInputLine, CBaseHudChatInputLine );
    
public:
    CHudChatInputLine( CBaseHudChat* parent, char const* panelName ) : CBaseHudChatInputLine( parent, panelName ) {}

    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;
};

class CHudChat : public CBaseHudChat
{
    DECLARE_CLASS_SIMPLE( CHudChat, CBaseHudChat );

public:
    CHudChat( const char* pElementName );

    virtual void CreateChatInputLine() OVERRIDE;
    virtual void CreateChatLines( void ) OVERRIDE;

    virtual void Init() OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;

    virtual int GetChatInputOffset() OVERRIDE;

    virtual Color GetClientColor( int clientIndex ) OVERRIDE;

};
