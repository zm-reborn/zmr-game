#include "cbase.h"

#include "zmr_gamerules.h"
#include "zmr_team.h"
#include "zmr_objectives.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void CObjLine::WriteEmptyDisplayUserMsg()
{
    WRITE_BYTE( 0 );
    WRITE_BYTE( 0 );
    WRITE_FLOAT( 0.0f );
}

void CObjLine::WriteArg() const
{
    switch ( m_iTextArgType )
    {
    case OBJARGTYPE_STRING : WRITE_STRING( m_szTextArg ); break;
    case OBJARGTYPE_TIMER : WRITE_FLOAT( m_flTimerEnd - gpGlobals->curtime ); break;
    case OBJARGTYPE_INT :
    case OBJARGTYPE_FLOAT : 
    default : WRITE_FLOAT( m_flTextArgNum ); break;
    }
}

void CObjLine::WriteDisplayUserMsg() const
{
    WRITE_BYTE( ( m_szTexts[0] != NULL ) ? 1 : 0 );

    WRITE_BYTE( m_iTextArgType );

    WriteArg();

    if ( m_szTexts[0] != NULL )
        WRITE_STRING( m_szTexts );
}

void CObjLine::WriteUpdateUserMsg() const
{
    WRITE_BYTE( m_bComplete ? 1 : 0 );

    WRITE_BYTE( m_iTextArgType );
    WriteArg();
}

void CObjLine::ParseArg( const char* psz )
{
    if ( !psz || !*psz )
    {
        m_flTextArgNum = 0.0f;
        m_szTextArg[0] = NULL;
        m_iTextArgType = OBJARGTYPE_NONE;
        return;
    }
    

    if ( isdigit( psz[0] ) )
    {
        m_iTextArgType = OBJARGTYPE_INT;
    }
    else
    {
        if ( psz[0] == 't' && isdigit( psz[1] ) )
        {
            m_iTextArgType = OBJARGTYPE_TIMER;
        }
        else
        {
            m_iTextArgType = OBJARGTYPE_STRING;
        }
    }

    if ( m_iTextArgType == OBJARGTYPE_TIMER )
    {
        m_flTextArgNum = atof( &psz[1] );

        m_flTimerEnd = gpGlobals->curtime + m_flTextArgNum;
    }
    else
    {
        m_flTextArgNum = atof( psz );
    }

    Q_strncpy( m_szTextArg, psz, sizeof( m_szTextArg ) );
}

void CObjLine::SetText( const char* psz )
{
    Q_strncpy( m_szTexts, ( psz && *psz ) ? psz : "", sizeof( m_szTexts ) );
}


LINK_ENTITY_TO_CLASS( info_objectives, CZMEntObjectives );

BEGIN_DATADESC( CZMEntObjectives )
    DEFINE_INPUTFUNC( FIELD_VOID, "Display", InputDisplay ),
    DEFINE_INPUTFUNC( FIELD_VOID, "DisplayActivator", InputDisplayActivator ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Update", InputUpdate ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Reset", InputReset ),

    DEFINE_INPUTFUNC( FIELD_STRING, "SetMainText", InputSetMainText ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild1Text", InputSetChild1Text ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild2Text", InputSetChild2Text ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild3Text", InputSetChild3Text ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild4Text", InputSetChild4Text ),

    DEFINE_INPUTFUNC( FIELD_STRING, "SetMainTextArg", InputSetMainTextArg ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild1TextArg", InputSetChild1TextArg ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild2TextArg", InputSetChild2TextArg ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild3TextArg", InputSetChild3TextArg ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild4TextArg", InputSetChild4TextArg ),

    /*DEFINE_INPUTFUNC( FIELD_STRING, "SetMainTextArgType", InputSetMainTextArgType ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild1TextArgType", InputSetChild1TextArgType ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild2TextArgType", InputSetChild2TextArgType ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild3TextArgType", InputSetChild3TextArgType ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetChild4TextArgType", InputSetChild4TextArgType ),*/

    DEFINE_INPUTFUNC( FIELD_FLOAT, "IncMainTextArg", InputIncMainTextArg ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "IncChild1TextArg", InputIncChild1TextArg ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "IncChild2TextArg", InputIncChild2TextArg ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "IncChild3TextArg", InputIncChild3TextArg ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "IncChild4TextArg", InputIncChild4TextArg ),

    // Like InValue in other entities.
    // Just sets the argument value.
    DEFINE_INPUTFUNC( FIELD_FLOAT, "InValueMainArg", InputMainArg ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "InValueChild1Arg", InputChild1Arg ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "InValueChild2Arg", InputChild2Arg ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "InValueChild3Arg", InputChild3Arg ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "InValueChild4Arg", InputChild4Arg ),

    DEFINE_INPUTFUNC( FIELD_VOID, "CompleteMain", InputCompleteMain ),
    DEFINE_INPUTFUNC( FIELD_VOID, "CompleteChild1", InputCompleteChild1 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "CompleteChild2", InputCompleteChild2 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "CompleteChild3", InputCompleteChild3 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "CompleteChild4", InputCompleteChild4 ),

    DEFINE_INPUTFUNC( FIELD_VOID, "InCompleteMain", InputInCompleteMain ),
    DEFINE_INPUTFUNC( FIELD_VOID, "InCompleteChild1", InputInCompleteChild1 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "InCompleteChild2", InputInCompleteChild2 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "InCompleteChild3", InputInCompleteChild3 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "InCompleteChild4", InputInCompleteChild4 ),

    DEFINE_INPUTFUNC( FIELD_VOID, "ResetMain", InputResetMain ),
    DEFINE_INPUTFUNC( FIELD_VOID, "ResetChild1", InputResetChild1 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "ResetChild2", InputResetChild2 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "ResetChild3", InputResetChild3 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "ResetChild4", InputResetChild4 ),

    DEFINE_INPUTFUNC( FIELD_STRING, "UpdateMainText", InputUpdateMainText ),
    DEFINE_INPUTFUNC( FIELD_STRING, "UpdateChild1Text", InputUpdateChild1Text ),
    DEFINE_INPUTFUNC( FIELD_STRING, "UpdateChild2Text", InputUpdateChild2Text ),
    DEFINE_INPUTFUNC( FIELD_STRING, "UpdateChild3Text", InputUpdateChild3Text ),
    DEFINE_INPUTFUNC( FIELD_STRING, "UpdateChild4Text", InputUpdateChild4Text ),

    DEFINE_KEYFIELD( m_iRecipient, FIELD_INTEGER, "Recipient" ),

    DEFINE_THINKFUNC( CheckDirtyThink ),
END_DATADESC()

CZMEntObjectives::CZMEntObjectives()
{
    m_iRecipient = OBJRECIPIENT_HUMANS;
    m_iDirtyStatus = OBJDIRTY_NONE;
    m_bDisplay = false;

    Reset();
}

CZMEntObjectives::~CZMEntObjectives()
{
}

void CZMEntObjectives::Spawn()
{
    BaseClass::Spawn();


    if ( m_iRecipient <= OBJRECIPIENT_INVALID || m_iRecipient >= OBJRECIPIENT_MAX )
    {
        Warning( "Invalid objectives recipient!\n" );

        m_iRecipient = OBJRECIPIENT_HUMANS;
    }

    // Don't think instantly since Active-keyvalue still isn't set for some reason.
    SetThink( &CZMEntObjectives::CheckDirtyThink );
    SetNextThink( gpGlobals->curtime + 1.0f );
}

void CZMEntObjectives::CheckDirtyThink()
{
    if ( IsDirty() && IsDisplaying() )
    {
        if ( m_iDirtyStatus == OBJDIRTY_DISPLAY )
        {
            CZMEntObjectivesManager::Display( this );
        }
        else
        {
            CZMEntObjectivesManager::Update( this );
        }
        
        m_iDirtyStatus = OBJDIRTY_NONE;
    }

    SetNextThink( gpGlobals->curtime + 0.1f );
}

void CZMEntObjectives::Reset()
{
    for ( int i = 0; i < NUM_OBJ_LINES; i++ )
    {
        m_Lines[i].Reset();
    }
}

void CZMEntObjectives::UpdateDirtyStatus( DirtyStatus_t dirtytype )
{
    if ( dirtytype > m_iDirtyStatus )
        m_iDirtyStatus = dirtytype;
}

void CZMEntObjectives::InputReset( inputdata_t& inputData ) { Reset(); }


void CZMEntObjectives::InputSetMainText( inputdata_t& inputData ) { m_Lines[0].SetText( inputData.value.String() ); UpdateDirtyStatus( OBJDIRTY_DISPLAY ); }
void CZMEntObjectives::InputSetMainTextArg( inputdata_t& inputData ) { m_Lines[0].ParseArg( inputData.value.String() ); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputIncMainTextArg( inputdata_t& inputData ) { m_Lines[0].m_flTextArgNum += inputData.value.Float(); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputMainArg( inputdata_t& inputData ) { m_Lines[0].m_flTextArgNum = inputData.value.Float();  UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputCompleteMain( inputdata_t& inputData ) { m_Lines[0].m_bComplete = true; UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputInCompleteMain( inputdata_t& inputData ) { m_Lines[0].m_bComplete = false; UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputResetMain( inputdata_t& inputData ) { m_Lines[0].Reset(); UpdateDirtyStatus( OBJDIRTY_DISPLAY ); }

// DEPRECATED! Dirty checks will automatically do this for us.
void CZMEntObjectives::InputUpdateMainText( inputdata_t& inputData )
{
    const char* psz = inputData.value.String();
    if ( psz && *psz ) m_Lines[0].SetText( psz );
    CZMEntObjectivesManager::UpdateLine( this, 0, inputData.pActivator );
    m_bDisplay = true;
    m_iDirtyStatus = OBJDIRTY_NONE;
}

void CZMEntObjectives::InputSetChild1Text( inputdata_t& inputData ) { m_Lines[1].SetText( inputData.value.String() ); UpdateDirtyStatus( OBJDIRTY_DISPLAY ); }
void CZMEntObjectives::InputSetChild1TextArg( inputdata_t& inputData ) { m_Lines[1].ParseArg( inputData.value.String() ); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputIncChild1TextArg( inputdata_t& inputData ) { m_Lines[1].m_flTextArgNum += inputData.value.Float(); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputChild1Arg( inputdata_t& inputData ) { m_Lines[1].m_flTextArgNum = inputData.value.Float(); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputCompleteChild1( inputdata_t& inputData ) { m_Lines[1].m_bComplete = true; UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputInCompleteChild1( inputdata_t& inputData ) { m_Lines[1].m_bComplete = false; UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputResetChild1( inputdata_t& inputData ) { m_Lines[1].Reset(); UpdateDirtyStatus( OBJDIRTY_DISPLAY ); }

// DEPRECATED! Dirty checks will automatically do this for us.
void CZMEntObjectives::InputUpdateChild1Text( inputdata_t& inputData )
{
    const char* psz = inputData.value.String();
    if ( psz && *psz ) m_Lines[1].SetText( psz );
    CZMEntObjectivesManager::UpdateLine( this, 1, inputData.pActivator );
    m_bDisplay = true;
    m_iDirtyStatus = OBJDIRTY_NONE;
}

void CZMEntObjectives::InputSetChild2Text( inputdata_t& inputData ) { m_Lines[2].SetText( inputData.value.String() ); UpdateDirtyStatus( OBJDIRTY_DISPLAY ); }
void CZMEntObjectives::InputSetChild2TextArg( inputdata_t& inputData ) { m_Lines[2].ParseArg( inputData.value.String() ); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputIncChild2TextArg( inputdata_t& inputData ) { m_Lines[2].m_flTextArgNum += inputData.value.Float(); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputChild2Arg( inputdata_t& inputData ) { m_Lines[2].m_flTextArgNum = inputData.value.Float(); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputCompleteChild2( inputdata_t& inputData ) { m_Lines[2].m_bComplete = true; UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputInCompleteChild2( inputdata_t& inputData ) { m_Lines[2].m_bComplete = false; UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputResetChild2( inputdata_t& inputData ) { m_Lines[2].Reset(); UpdateDirtyStatus( OBJDIRTY_DISPLAY ); }

// DEPRECATED! Dirty checks will automatically do this for us.
void CZMEntObjectives::InputUpdateChild2Text( inputdata_t& inputData )
{
    const char* psz = inputData.value.String();
    if ( psz && *psz ) m_Lines[2].SetText( psz );
    CZMEntObjectivesManager::UpdateLine( this, 2, inputData.pActivator );
    m_bDisplay = true;
    m_iDirtyStatus = OBJDIRTY_NONE;
}

void CZMEntObjectives::InputSetChild3Text( inputdata_t& inputData ) { m_Lines[3].SetText( inputData.value.String() ); UpdateDirtyStatus( OBJDIRTY_DISPLAY ); }
void CZMEntObjectives::InputSetChild3TextArg( inputdata_t& inputData ) { m_Lines[3].ParseArg( inputData.value.String() ); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputIncChild3TextArg( inputdata_t& inputData ) { m_Lines[3].m_flTextArgNum += inputData.value.Float(); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputChild3Arg( inputdata_t& inputData ) { m_Lines[3].m_flTextArgNum = inputData.value.Float(); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputCompleteChild3( inputdata_t& inputData ) { m_Lines[3].m_bComplete = true; UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputInCompleteChild3( inputdata_t& inputData ) { m_Lines[3].m_bComplete = false; UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputResetChild3( inputdata_t& inputData ) { m_Lines[3].Reset(); UpdateDirtyStatus( OBJDIRTY_DISPLAY ); }

// DEPRECATED! Dirty checks will automatically do this for us.
void CZMEntObjectives::InputUpdateChild3Text( inputdata_t& inputData )
{
    const char* psz = inputData.value.String();
    if ( psz && *psz ) m_Lines[3].SetText( psz );
    CZMEntObjectivesManager::UpdateLine( this, 3, inputData.pActivator );
    m_bDisplay = true;
    m_iDirtyStatus = OBJDIRTY_NONE;
}

void CZMEntObjectives::InputSetChild4Text( inputdata_t& inputData ) { m_Lines[4].SetText( inputData.value.String() ); UpdateDirtyStatus( OBJDIRTY_DISPLAY ); }
void CZMEntObjectives::InputSetChild4TextArg( inputdata_t& inputData ) { m_Lines[4].ParseArg( inputData.value.String() ); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputIncChild4TextArg( inputdata_t& inputData ) { m_Lines[4].m_flTextArgNum += inputData.value.Float(); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputChild4Arg( inputdata_t& inputData ) { m_Lines[4].m_flTextArgNum = inputData.value.Float(); UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputCompleteChild4( inputdata_t& inputData ) { m_Lines[4].m_bComplete = true; UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputInCompleteChild4( inputdata_t& inputData ) { m_Lines[4].m_bComplete = false; UpdateDirtyStatus( OBJDIRTY_UPDATE ); }
void CZMEntObjectives::InputResetChild4( inputdata_t& inputData ) { m_Lines[4].Reset(); UpdateDirtyStatus( OBJDIRTY_DISPLAY ); }

// DEPRECATED
void CZMEntObjectives::InputUpdateChild4Text( inputdata_t& inputData )
{
    const char* psz = inputData.value.String();
    if ( psz && *psz ) m_Lines[4].SetText( psz );
    CZMEntObjectivesManager::UpdateLine( this, 4, inputData.pActivator );
    m_bDisplay = true;
    m_iDirtyStatus = OBJDIRTY_NONE;
}

void CZMEntObjectives::InputDisplay( inputdata_t& inputData )
{
    CZMEntObjectivesManager::Display( this, inputData.pActivator );
    m_bDisplay = true;
    m_iDirtyStatus = OBJDIRTY_NONE;
}

void CZMEntObjectives::InputDisplayActivator( inputdata_t& inputData )
{
    CZMEntObjectivesManager::Display( this, inputData.pActivator, OBJRECIPIENT_ACTIVATOR );
    m_bDisplay = true;
    m_iDirtyStatus = OBJDIRTY_NONE;
}

// DEPRECATED
void CZMEntObjectives::InputUpdate( inputdata_t& inputData )
{
    CZMEntObjectivesManager::Update( this, inputData.pActivator );
    m_bDisplay = true;
    m_iDirtyStatus = OBJDIRTY_NONE;
}

void CZMEntObjectives::GetRecipientFilter( CBaseEntity* pActivator, CRecipientFilter& filter, ObjRecipient_t rec ) const
{
    ObjRecipient_t recipient = m_iRecipient;

    if ( rec != OBJRECIPIENT_INVALID )
        recipient = rec;


    RecipientToFilter( pActivator, filter, recipient );
}

void CZMEntObjectives::RecipientToFilter( CBaseEntity* pActivator, CRecipientFilter& filter, ObjRecipient_t rec )
{
    switch ( rec )
    {
    case OBJRECIPIENT_ALL :
        filter.AddAllPlayers();
        break;
    case OBJRECIPIENT_ZM :
        filter.AddRecipientsByTeam( GetGlobalTeam( ZMTEAM_ZM ) );
        break;
    case OBJRECIPIENT_ACTIVATOR :
    {
        CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>( pActivator );

        if ( pPlayer )
            filter.AddRecipient( pPlayer );

        break;
    }
    case OBJRECIPIENT_HUMANS :
        filter.AddRecipientsByTeam( GetGlobalTeam( ZMTEAM_HUMAN ) );
        filter.AddRecipientsByTeam( GetGlobalTeam( ZMTEAM_SPECTATOR ) );
        break;
    default :
        break;
    }
}

/*
ObjArgType_t CZMEntObjectives::TypeNameToType( const char* psz )
{
    if ( psz )
    {
        if ( Q_stricmp( psz, "timer" ) == 0 )
            return OBJARGTYPE_TIMER;
        if ( Q_stricmp( psz, "float" ) == 0 )
            return OBJARGTYPE_FLOAT;
    }

    return OBJARGTYPE_INT;
}
*/
