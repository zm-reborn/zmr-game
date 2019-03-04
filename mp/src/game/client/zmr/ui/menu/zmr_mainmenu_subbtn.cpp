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
    // We always need to repaint
    if ( m_flFadeIn != 0.0f )
    {
        int a = (gpGlobals->realtime - m_flFadeIn) / m_flFadeInTime * 255;

        if ( a >= 255 )
        {
            a = 255;
            m_flFadeIn = 0.0f;
        }

        
        _defaultFgColor[3] = a;
        SetFgColor( _defaultFgColor );
        Repaint();
    }
    else if ( m_flFadeOut != 0.0f )
    {
        int a = (m_flFadeOut - gpGlobals->realtime) / m_flFadeOutTime * 255;

        if ( a <= 0 )
        {
            SetVisible( false );
            m_flFadeOut = 0.0f;
        }
        else
        {
            _defaultFgColor[3] = a;
            SetFgColor( _defaultFgColor );
            Repaint();
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
    if ( m_flFadeIn != 0.0f && m_flFadeIn > gpGlobals->realtime )
        return;


    m_flFadeIn = gpGlobals->realtime;
    
    m_flFadeInTime = fade;
    m_flFadeOut = 0.0f;


    SetVisible( true );
    Repaint();
}

void CZMMainMenuSubButton::FadeOut( float fade )
{
    if ( m_flFadeOut != 0.0f && m_flFadeOut <= (gpGlobals->realtime+2.0f) )
        return;


    m_flFadeOut = gpGlobals->realtime + fade;

    m_flFadeOutTime = fade;
    m_flFadeIn = 0.0f;

    Repaint();
}
