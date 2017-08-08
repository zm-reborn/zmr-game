#include "cbase.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Slider.h>
#include <vgui/ivgui.h>
#include "filesystem.h"
#include <vgui_controls/AnimationController.h>


#include "zmr/c_zmr_player.h"
#include "zmr_options.h"


extern ConVar zm_cl_poweruser;
extern ConVar zm_cl_poweruser_boxselect;

extern ConVar cl_yawspeed;
extern ConVar cl_pitchspeed;


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


    inline bool FailedLoad() { return m_bFailedLoad; };

protected:
    bool m_bFailedLoad;

    template <typename Type>
    void LoadItem( Type** dest, const char* name )
    {
        *dest = dynamic_cast<Type*>( FindChildByName( name ) );

        if ( !*dest )
        {
            Warning( "Couldn't load item %s.\n", name );

            m_bFailedLoad = true;
        }
    }

    ComboBox* m_pPartBox;
    CheckButton* m_pCheck_PowerUser;
    CheckButton* m_pCheck_BoxPowerUser;
    Slider* m_pSlider_Pitch;
    Slider* m_pSlider_Yaw;

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
    m_bFailedLoad = false;

    SetParent( parent );

    SetMouseInputEnabled( true );
    SetKeyBoardInputEnabled( true );
    SetProportional( false );
    SetVisible( false );
    SetMoveable( true );
    SetSizeable( false );


    SetScheme( scheme()->LoadSchemeFromFile( "resource/SourceScheme.res", "SourceScheme" ) );


    LoadControlSettings( "resource/ui/zmoptions.res" );

    LoadItem( &m_pPartBox, "part_type" );
    LoadItem( &m_pCheck_PowerUser, "check_poweruser" );
    LoadItem( &m_pCheck_BoxPowerUser, "check_poweruser_box" );
    LoadItem( &m_pSlider_Yaw, "scrollhor_slider" );
    LoadItem( &m_pSlider_Pitch, "scrollver_slider" );

    if ( FailedLoad() ) return;


    m_pSlider_Yaw->SetRange( 10, 500 );
    m_pSlider_Pitch->SetRange( 10, 500 );


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
    if ( FailedLoad() ) return;


    Participation_t part = C_ZMPlayer::GetLocalParticipation();

    m_pPartBox->ActivateItem( part );


    m_pCheck_PowerUser->SetSelected( zm_cl_poweruser.GetBool() );
    m_pCheck_BoxPowerUser->SetSelected( zm_cl_poweruser_boxselect.GetBool() );


    m_pSlider_Yaw->SetValue( (int)cl_yawspeed.GetFloat() );
    m_pSlider_Pitch->SetValue( (int)cl_pitchspeed.GetFloat() );
}

void CZMOptionsMenu::ApplySettings()
{
    if ( FailedLoad() ) return;


    C_ZMPlayer::SetLocalParticipation( (Participation_t)m_pPartBox->GetActiveItem() );

    zm_cl_poweruser.SetValue( m_pCheck_PowerUser->IsSelected() ? 1 : 0 );

    zm_cl_poweruser_boxselect.SetValue( m_pCheck_BoxPowerUser->IsSelected() ? 1 : 0 );


    cl_yawspeed.SetValue( m_pSlider_Yaw->GetValue() );
    cl_pitchspeed.SetValue( m_pSlider_Pitch->GetValue() );
}