#include "cbase.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/ComboBox.h>
#include <vgui/ivgui.h>
#include "filesystem.h"
#include <vgui_controls/AnimationController.h>


#include "zmr/c_zmr_player.h"
#include "zmr_options.h"



using namespace vgui;

class CZMOptionsMenu : public Frame
{
public:
    DECLARE_CLASS_SIMPLE( CZMOptionsMenu, Frame );

    CZMOptionsMenu( VPANEL parent );
    ~CZMOptionsMenu();


    virtual void OnTick() OVERRIDE;
    virtual void OnThink() OVERRIDE;

    virtual void OnCommand( const char* ) OVERRIDE;


protected:
    ComboBox* m_pPartBox;

    void UpdateMenu();
    void ApplySettings();
};

class CZMOptionsMenuInterface : public IZMUi
{
public:
    CZMOptionsMenuInterface() { m_Panel = nullptr; };

    void Create( VPANEL parent ) OVERRIDE
    {
        m_Panel = new CZMOptionsMenu( parent );
    }
    void Destroy() OVERRIDE
    {
        if ( m_Panel )
        {
            m_Panel->SetParent( nullptr );
            delete m_Panel;
        }
    }
    vgui::Panel* GetPanel() OVERRIDE { return m_Panel; }

private:
    CZMOptionsMenu* m_Panel;
};

static CZMOptionsMenuInterface g_ZMOptionsMenuInt;
IZMUi* g_pZMOptionsMenu = (IZMUi*)&g_ZMOptionsMenuInt;



static bool g_bZMOptionsShow = false;

CON_COMMAND( ToggleZMOptions, "" )
{
    g_bZMOptionsShow = !g_bZMOptionsShow;
}


CZMOptionsMenu::CZMOptionsMenu( VPANEL parent ) : BaseClass( nullptr, "ZMOptionsMenu" )
{
    SetParent( parent );

    SetMouseInputEnabled( true );
    SetKeyBoardInputEnabled( true );
    SetProportional( false );
    SetVisible( false );
    SetMoveable( true );
    SetSizeable( false );


    SetScheme( scheme()->LoadSchemeFromFile( "resource/SourceScheme.res", "SourceScheme" ) );


    LoadControlSettings( "resource/ui/zmoptions.res" );

    m_pPartBox = dynamic_cast<ComboBox*>( FindChildByName( "part_type" ) );

    if ( !m_pPartBox )
    {
        Warning( "Failed to load zmoptions.res!\n" );
        return;
    }

    KeyValues* kv = new KeyValues( "participation" );

    if ( kv->LoadFromFile( filesystem, "resource/zmoptions_participation.res", "MOD" ) )
    {
        KeyValues* pKey = kv->GetFirstSubKey();

        while ( pKey )
        {
            m_pPartBox->AddItem( pKey->GetString( "label", "" ), pKey );

            pKey = pKey->GetNextKey();
        }
    }
    else
    {
        Warning( "Couldn't load participation values from file!\n" );
    }

    kv->deleteThis();

    m_pPartBox->ActivateItemByRow( 0 );


    vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

CZMOptionsMenu::~CZMOptionsMenu()
{
}

void CZMOptionsMenu::OnTick()
{
    BaseClass::OnTick();

    
    if ( g_bZMOptionsShow != IsVisible() )
    {
        SetVisible( g_bZMOptionsShow );

        if ( IsVisible() )
            UpdateMenu();

        return;
    }
}

void CZMOptionsMenu::OnThink()
{
    
}

void CZMOptionsMenu::OnCommand( const char* command )
{
    if ( Q_stricmp( command, "Close" ) == 0 )
    {
        // Don't do fade in/our effects right now.
        g_bZMOptionsShow = false;
        return;
    }
    else if ( Q_stricmp( command, "Ok" ) == 0 )
    {
        ApplySettings();

        g_bZMOptionsShow = false;
    }

    BaseClass::OnCommand( command );
}

void CZMOptionsMenu::UpdateMenu()
{
    Participation_t part = C_ZMPlayer::GetLocalParticipation();

    m_pPartBox->ActivateItem( part );
}

void CZMOptionsMenu::ApplySettings()
{
    C_ZMPlayer::SetLocalParticipation( (Participation_t)m_pPartBox->GetActiveItem() );
}