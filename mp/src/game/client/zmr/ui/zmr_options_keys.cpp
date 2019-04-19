#include "cbase.h"
#include "filesystem.h"
#include <inputsystem/iinputsystem.h>
#include "vcontrolslistpanel.h"

#include "zmr_options_keys.h"

#include "IGameUIFuncs.h"
#include <igameresources.h>

extern IGameUIFuncs* gameuifuncs; // for key binding details

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


CZMOptionsSubKeys::CZMOptionsSubKeys( Panel* parent ) : BaseClass( parent )
{
    m_pKeyBindList = new VControlsListPanel( this, "listpanel_keybindlist" );

    m_pEditBtn = new Button( this, "ChangeKeyButton", "" );
    m_pClearBtn = new Button( this, "ClearKeyButton", "" );
    m_pDefaultBtn = new Button( this, "DefaultButton", "" );


    LoadControlSettings( "resource/ui/zmoptionssubkeys.res" );


    if ( FailedLoad() ) return;


    ParseKeys();
}

CZMOptionsSubKeys::~CZMOptionsSubKeys()
{
}

void CZMOptionsSubKeys::OnApplyChanges()
{
    if ( FailedLoad() ) return;


    SaveKeysToConfig();
}

void CZMOptionsSubKeys::OnResetData()
{
    if ( FailedLoad() ) return;


    zmkeydatalist_t zmkeys, survivorkeys;

    CZMTeamKeysConfig::LoadConfigByTeam( KEYTEAM_ZM, zmkeys );
    CZMTeamKeysConfig::LoadConfigByTeam( KEYTEAM_SURVIVOR, survivorkeys );

    FillKeys( zmkeys, survivorkeys );
}

bool CZMOptionsSubKeys::IsValidKeyForBinding( ButtonCode_t code )
{
    return code != KEY_ESCAPE && code != KEY_BACKSPACE;
}

void CZMOptionsSubKeys::OnThink()
{
	if ( m_pKeyBindList->IsCapturing() )
	{
		ButtonCode_t code = BUTTON_CODE_INVALID;
		if ( engine->CheckDoneKeyTrapping( code ) )
        {
            int id = m_pKeyBindList->GetSelectedItem();
            auto* item = m_pKeyBindList->GetItemData( id );

            if ( item && IsValidKeyForBinding( code ) )
            {
                BindCommand( item, code );

                m_pKeyBindList->InvalidateItem( id );
            }
            

            m_pKeyBindList->EndCaptureMode( dc_arrow );
        }
    }
}

void CZMOptionsSubKeys::OnCommand( const char* command )
{
    if ( !m_pKeyBindList->IsCapturing() && Q_stricmp( command, "ChangeKey" ) == 0 )
    {
        m_pKeyBindList->StartCaptureMode( dc_blank );
        return;
    }

    if ( Q_stricmp( command, "ClearKey" ) == 0 )
    {
        OnKeyCodeTyped( KEY_DELETE );
        return;
    }

    if ( Q_stricmp( command, "Defaults" ) == 0 )
    {
        zmkeydatalist_t zmkeys, survivorkeys;

        CZMTeamKeysConfig::ParseConfig( KEYCONFIG_ZM_DEF, zmkeys );
        CZMTeamKeysConfig::ParseConfig( KEYCONFIG_SURVIVOR_DEF, survivorkeys );

        FillKeys( zmkeys, survivorkeys );

        return;
    }


    BaseClass::OnCommand( command );
}

void CZMOptionsSubKeys::OnKeyCodeTyped( vgui::KeyCode code )
{
    if ( code == KEY_ENTER )
    {
        // Starting waiting for a key press
        m_pKeyBindList->StartCaptureMode( dc_blank );
    }
    else if ( code == KEY_DELETE )
    {
        // Clear the key
        int id = m_pKeyBindList->GetSelectedItem();
        auto* item = m_pKeyBindList->GetItemData( id );

        item->SetString( "Key", "" );
        m_pKeyBindList->InvalidateItem( id );
    }
}

void CZMOptionsSubKeys::ItemSelected( int id )
{
    // We need to keep track of the item of interest
    m_pKeyBindList->SetItemOfInterest( id );



    auto* selectedData = m_pKeyBindList->GetItemData( id );


    m_pClearBtn->SetEnabled( (*selectedData->GetString( "Key" )) ? true : false );
}

void CZMOptionsSubKeys::BindCommand( KeyValues* item, ButtonCode_t code )
{
    //
    // We want to change a binding.
    //

    const char* keyname = inputsystem->ButtonCodeToString( code );

    bool bUnboundNeutral = false;


    const char* cmd = item->GetString( "Binding" );

    ZMKeyTeam_t keyteam = CZMTeamKeysConfig::GetCommandType( cmd );

    // Unbind any commands in our list that use the same key.
    for ( int i = 0; i < m_pKeyBindList->GetItemCount(); i++ )
    {
        int id = m_pKeyBindList->GetItemIDFromRow( i );

        auto* kvData = m_pKeyBindList->GetItemData( id );


        ZMKeyTeam_t mykeyteam = CZMTeamKeysConfig::GetCommandType( kvData->GetString( "Binding" ) );

        // If it's a different team's key, ignore it.
        if ( mykeyteam != KEYTEAM_NEUTRAL && keyteam != KEYTEAM_NEUTRAL && mykeyteam != keyteam )
        {
            continue;
        }


        if ( Q_stricmp( keyname, kvData->GetString( "Key" ) ) == 0 )
        {
            kvData->SetString( "Key", "" );
            m_pKeyBindList->InvalidateItem( id );

            if ( mykeyteam == KEYTEAM_NEUTRAL )
                bUnboundNeutral = true;
        }
        
    }


    // If it's a neutral command,
    // we can't bind on top of it.
    if ( !bUnboundNeutral )
    {
        const char* replacedcmd = gameuifuncs->GetBindingForButtonCode( code );

        if (replacedcmd
        &&  *replacedcmd
        &&  CZMTeamKeysConfig::IsNeutralCommand( replacedcmd ))
        {
            return;
        }
    }

    item->SetString( "Key", keyname );
}

void CZMOptionsSubKeys::ParseKeys()
{
    //
    // Parse the bindings and add them to the list.
    //
    const char* file = "scripts/kb_act_zm.lst";

    auto hndl = filesystem->Open( file, "rb", "MOD" );
    if ( !hndl )
    {
        Warning( "Failed to open config file '%s' for read!\n", file );
        return;
    }


    char buf[512];
    buf[511] = NULL;

    int sectionIndex = 0;
    char desc[512];
    char binding[512];
    char token[512];


    while ( filesystem->ReadLine( buf, sizeof( buf ) - 1, hndl ) != nullptr )
    {
        const char* data = buf;

		data = engine->ParseFile( data, token, sizeof( token ) );
		// Done.
		if ( token[0] == NULL )  
			continue;

		Q_strncpy( binding, token, sizeof( binding ) );

		data = engine->ParseFile( data, token, sizeof( token ) );
		if ( token[0] == NULL )
            continue;

        // Skip '======' rows
		if ( token[0] == '=' )
            continue;

		Q_strncpy( desc, token, sizeof( desc ) );

		
		// This is a header
		if ( !stricmp( binding, "blank" ) )
		{
			m_pKeyBindList->AddSection( ++sectionIndex, desc );
			m_pKeyBindList->AddColumnToSection(
                sectionIndex, "Action",
                desc,
                SectionedListPanel::COLUMN_BRIGHT,
                286 );
			m_pKeyBindList->AddColumnToSection(
                sectionIndex, "Key",
                "#GameUI_KeyButton",
                SectionedListPanel::COLUMN_BRIGHT,
                128 );
		}
		else // It's a binding, add it as an item.
		{
			auto* item = new KeyValues( "Item" );
			
			item->SetString( "Action", desc );
			item->SetString( "Binding", binding );
			item->SetString( "Key", "" );


			m_pKeyBindList->AddItem( sectionIndex, item );
			item->deleteThis();
		}
    }


    filesystem->Close( hndl );
}

void CZMOptionsSubKeys::FillKeys( zmkeydatalist_t& zmkeys, zmkeydatalist_t& survivorkeys )
{
    //
    // Fill in the bindings to keys.
    //
    bool bResaveKeys = false;


    for ( int i = 0; i < m_pKeyBindList->GetItemCount(); i++ )
    {
        int id = m_pKeyBindList->GetItemIDFromRow( i );

        auto* item = m_pKeyBindList->GetItemData( id );


        const char* cmd = item->GetString( "Binding" );


        ButtonCode_t code = BUTTON_CODE_INVALID;

        zmkeydata_t* pKeyData = nullptr;
        zmkeydatalist_t* pKeyDataList = nullptr;

        ZMKeyTeam_t keyteam = CZMTeamKeysConfig::GetCommandType( cmd );

        switch ( keyteam )
        {
        case KEYTEAM_ZM :
            pKeyDataList = &zmkeys;
            pKeyData = CZMTeamKeysConfig::FindKeyDataFromList( cmd, zmkeys );
            if ( pKeyData )
                code = pKeyData->key;
            break;
        case KEYTEAM_SURVIVOR :
            pKeyDataList = &survivorkeys;
            pKeyData = CZMTeamKeysConfig::FindKeyDataFromList( cmd, survivorkeys );
            if ( pKeyData )
                code = pKeyData->key;
            break;
        default :
            code = gameuifuncs->GetButtonCodeForBind( cmd );
            break;
        }


        // Check if some neutral command is overriding our key.
        // If this is the case, we need to resave the configs immediately.
        if ( keyteam != KEYTEAM_NEUTRAL && code > BUTTON_CODE_NONE )
        {
            const char* replacedcmd = gameuifuncs->GetBindingForButtonCode( code );

            if (replacedcmd
            &&  *replacedcmd
            &&  CZMTeamKeysConfig::IsNeutralCommand( replacedcmd ))
            {
                DevMsg( "Command %s is being overridden by neutral command %s!\n", cmd, replacedcmd );


                pKeyDataList->FindAndRemove( pKeyData );
                delete pKeyData;

                code = BUTTON_CODE_INVALID;

                bResaveKeys = true;
            }
        }

        if ( code > BUTTON_CODE_NONE )
        {
            const char* btnname = inputsystem->ButtonCodeToString( code );
            item->SetString( "Key", btnname );
        }
        else
        {
            item->SetString( "Key", "" );
        }

        m_pKeyBindList->InvalidateItem( id );
    }



    if ( bResaveKeys )
    {
        DevMsg( "Resaving bindings because some of them were overridden!\n" );

        CZMTeamKeysConfig::SaveConfig( KEYCONFIG_ZM, zmkeys );
        CZMTeamKeysConfig::SaveConfig( KEYCONFIG_SURVIVOR, survivorkeys );
    }
}

void CZMOptionsSubKeys::SaveKeysToConfig()
{
    //
    // Client hit "save", save 'em bois.
    //
    zmkeydatalist_t zmkeys;
    zmkeydatalist_t survivorkeys;

    // Loads the keys from configs as well to make sure we don't accidentally remove some bind.
    CZMTeamKeysConfig::LoadConfigByTeam( KEYTEAM_ZM, zmkeys );
    CZMTeamKeysConfig::LoadConfigByTeam( KEYTEAM_SURVIVOR, survivorkeys );


    zmkeydata_t keydata;

    for ( int i = 0; i < m_pKeyBindList->GetItemCount(); i++ )
    {
        int id = m_pKeyBindList->GetItemIDFromRow( i );

        auto* item = m_pKeyBindList->GetItemData( id );

        const char* cmd = item->GetString( "Binding" );
        const char* keyname = item->GetString( "Key" );


        // No key to save
        if ( !keyname || !*keyname )
            continue;


        Q_strncpy( keydata.cmd, cmd, sizeof( keydata.cmd ) );
        keydata.key = inputsystem->StringToButtonCode( keyname );


        zmkeydatalist_t* pKeyList = nullptr;

        ZMKeyTeam_t keyteam = CZMTeamKeysConfig::GetCommandType( cmd );

        switch ( keyteam )
        {
        case KEYTEAM_ZM :
            pKeyList = &zmkeys;
            break;
        case KEYTEAM_SURVIVOR :
            pKeyList = &survivorkeys;
            break;
        default :
            // It's a neutral command, just execute it right now.
            engine->ClientCmd_Unrestricted( VarArgs( "bind \"%s\" \"%s\"", keyname, cmd ) );
            break;
        }
        

        if ( pKeyList )
        {
            auto* pKey = CZMTeamKeysConfig::FindKeyDataFromList( cmd, *pKeyList );
            auto* pOldKey = CZMTeamKeysConfig::FindKeyDataFromListByKey( keydata.key, *pKeyList );

            if ( pOldKey )
            {
                if ( pKey == pOldKey )
                    pKey = nullptr;

                pKeyList->FindAndRemove( pOldKey );
                delete pOldKey;
            }

            if ( pKey )
            {
                pKey->key = keydata.key;
            }
            else
            {
                pKeyList->AddToTail( new zmkeydata_t( keydata ) );
            }
        }
    }


    CZMTeamKeysConfig::SaveConfig( KEYCONFIG_ZM, zmkeys );
    CZMTeamKeysConfig::SaveConfig( KEYCONFIG_SURVIVOR, survivorkeys );


    zmkeys.PurgeAndDeleteElements();
    survivorkeys.PurgeAndDeleteElements();


    // Forcefully execute at least one of the configs.
    // This is for the stock options menu, so it gets updated.
    CZMTeamKeysConfig::ExecuteTeamConfig( true );
}
