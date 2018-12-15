#include "cbase.h"


#include "zmr_mainmenu_subbtn.h"
#include "zmr_mainmenu.h"


using namespace vgui;

CZMMainMenuSubButton::CZMMainMenuSubButton( Panel* pParent, const char* name ) : BaseClass( nullptr, name )
{
    Assert( pParent != nullptr );
    Panel* pRealParent = pParent->GetParent();
    Assert( dynamic_cast<CZMMainMenu*>( pRealParent ) != nullptr );

    SetParent( pRealParent );

    
    
    m_pButtonParent = pParent;


    m_flFadeOutTime = 0.0f;
    m_flFadeOut = 0.0f;
    m_flFadeInTime = 0.0f;
    m_flFadeIn = 0.0f;


    SetPaintBackgroundEnabled( false );
    SetPaintBorderEnabled( false );
}

CZMMainMenuSubButton::~CZMMainMenuSubButton()
{
}

void CZMMainMenuSubButton::Paint()
{
    if ( m_flFadeIn != 0.0f )
    {
        int a = (gpGlobals->curtime - m_flFadeIn) / m_flFadeInTime * 255;

        Color clr = GetButtonDefaultFgColor();
        clr[3] = a;

        if ( a >= 255 )
        {
            clr[3] = 255;
            m_flFadeIn = 0.0f;
        }

        SetDefaultColor( clr, GetButtonDefaultBgColor() );
    }
    else if ( m_flFadeOut != 0.0f )
    {
        int a = (m_flFadeOut - gpGlobals->curtime) / m_flFadeOutTime * 255;

        if ( a <= 0 )
        {
            SetVisible( false );
            m_flFadeOut = 0.0f;
        }
        else
        {
            Color clr = GetButtonDefaultFgColor();
            clr[3] = a;
            SetDefaultColor( clr, GetButtonDefaultBgColor() );
        }
    }




    BaseClass::Paint();
}

void CZMMainMenuSubButton::FireActionSignal()
{
    GetMainMenu()->HideSubButtons();

    BaseClass::FireActionSignal();
}

void CZMMainMenuSubButton::ApplySchemeSettings( IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );

    SetFont( pScheme->GetFont( "ZMMainMenuSubButton", IsProportional() ) );
    //SetContentAlignment( Label::Alignment::a_center );
}

void CZMMainMenuSubButton::FadeIn( float fade )
{
    if ( m_flFadeIn != 0.0f && m_flFadeIn > gpGlobals->curtime )
        return;


    m_flFadeIn = gpGlobals->curtime;
    
    m_flFadeInTime = fade;
    m_flFadeOut = 0.0f;


    SetVisible( true );
}

void CZMMainMenuSubButton::FadeOut( float fade )
{
    if ( m_flFadeOut != 0.0f && m_flFadeOut <= (gpGlobals->curtime+2.0f) )
        return;


    m_flFadeOut = gpGlobals->curtime + fade;

    m_flFadeOutTime = fade;
    m_flFadeIn = 0.0f;
}
