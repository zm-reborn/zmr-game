#include "cbase.h"

#include <vgui/ISystem.h>
#include <steam/steam_api.h>


#include "zmr_mainmenu_contactbuttons.h"



DECLARE_BUILD_FACTORY( CZMMainMenuContactButtonList );




class CZMMainMenuContactButton : public vgui::Panel
{
public:
    typedef vgui::Panel BaseClass;
    typedef CZMMainMenuContactButton ThisClass;
    //DECLARE_CLASS( CZMMainMenuContactButtons, vgui::Button );

    CZMMainMenuContactButton( vgui::Panel* pParent, const char* name, const char* image = "", const char* url = "" );
    ~CZMMainMenuContactButton();


    //virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;
    //virtual void ApplySettings( KeyValues* kv ) OVERRIDE;

    virtual void Paint() OVERRIDE;

    virtual void OnMousePressed( vgui::MouseCode code ) OVERRIDE;
    virtual void OnCursorEntered() OVERRIDE { SetCursor( vgui::dc_hand ); }
    virtual void OnCursorExited() OVERRIDE { SetCursor( vgui::dc_arrow ); }


    void SetImage( const char* name );

private:
    char* m_pszUrl;

    vgui::IImage* m_pImage;

    Color m_Color;
    Color m_OverColor;

    float m_flNextClick;
};

using namespace vgui;



CZMMainMenuContactButton::CZMMainMenuContactButton(
    Panel* pParent,
    const char* name,
    const char* image,
    const char* url ) : BaseClass( pParent, name )
{
    SetMouseInputEnabled( true );
    SetPaintBackgroundEnabled( false );



    m_pImage = scheme()->GetImage( image, true );
    
    if ( *url )
    {
        int len = Q_strlen( url ) + 1;
        m_pszUrl = new char[len];
        Q_strncpy( m_pszUrl, url, len );
    }
    else
    {
        m_pszUrl = "";
    }
    

    

    
    m_Color = Color( 255, 255, 255, 60 );
    m_OverColor = COLOR_WHITE;


    m_flNextClick = 0.0f;
}

CZMMainMenuContactButton::~CZMMainMenuContactButton()
{
}

void CZMMainMenuContactButton::OnMousePressed( vgui::MouseCode code )
{
    if ( *m_pszUrl && m_flNextClick <= gpGlobals->curtime )
    {
        auto* pSteamFriends = steamapicontext->SteamFriends();
        auto* pSteamUtils = steamapicontext->SteamUtils();
        if (pSteamFriends
        &&  pSteamUtils
        &&  pSteamUtils->IsOverlayEnabled() )
        {
            pSteamFriends->ActivateGameOverlayToWebPage( m_pszUrl );
        }
        else
        {
            system()->ShellExecute( "open", m_pszUrl );
        }


        m_flNextClick = gpGlobals->curtime + 5.0f;
    }

    BaseClass::OnMousePressed( code );
}

void CZMMainMenuContactButton::Paint()
{
    if ( m_pImage )
    {
        m_pImage->SetPos( 0, 0 );
        m_pImage->SetSize( GetWide(), GetTall() );
        m_pImage->SetColor( IsCursorOver() ? m_OverColor : m_Color );
        m_pImage->Paint();
    }
}
//




CZMMainMenuContactButtonList::CZMMainMenuContactButtonList( Panel* pParent, const char* name ) : BaseClass( pParent, name )
{
    SetPaintBackgroundEnabled( false );
}


CZMMainMenuContactButtonList::~CZMMainMenuContactButtonList()
{
}

void CZMMainMenuContactButtonList::ApplySchemeSettings( vgui::IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );

    int size = GetTall();

    int len = GetChildCount();

    SetWide( len * size );


    for ( int i = 0; i < len; i++ )
    {
        auto* pChild = dynamic_cast<CZMMainMenuContactButton*>( GetChild( i ) );
        if ( pChild )
        {
            pChild->SetBounds( i * size, 0, size, size );
        }
    }
}

void CZMMainMenuContactButtonList::ApplySettings( KeyValues* kv )
{
    KeyValues* subkv = kv->FindKey( "links" );
    if ( subkv )
    {
        for ( subkv = subkv->GetFirstSubKey(); subkv; subkv = subkv->GetNextKey() )
        {
            AddButton( subkv );
        }
    }

    BaseClass::ApplySettings( kv );
}

void CZMMainMenuContactButtonList::AddButton( KeyValues* kv )
{
    new CZMMainMenuContactButton(
        this,
        kv->GetName(),
        kv->GetString( "icon" ),
        kv->GetString( "url" ) );
}
