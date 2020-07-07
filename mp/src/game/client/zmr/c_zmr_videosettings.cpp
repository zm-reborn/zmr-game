#include "cbase.h"

#ifdef WIN32
#define _WINREG_
#undef ReadConsoleInput
#undef INVALID_HANDLE_VALUE
#undef GetCommandLine
#include <Windows.h>
#endif

#include <tier0/icommandline.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//
// Unused for now.
//

#ifdef WIN32
static bool ZMSetBorderlessFullscreen( HWND window )
{
    HMONITOR monitor = MonitorFromWindow( window, MONITOR_DEFAULTTONEAREST );

    if ( monitor == nullptr )
        return false;

    MONITORINFO monitorInfo;
    Q_memset( &monitorInfo, 0, sizeof( monitorInfo ) );
    monitorInfo.cbSize = sizeof( monitorInfo );

    if ( !GetMonitorInfo( monitor, &monitorInfo ) )
        return false;

    LONG style = GetWindowLong( window, GWL_STYLE );
    style &= ~( WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZE | WS_MINIMIZE );
    SetWindowLong( window, GWL_STYLE, style );

    LONG exstyle = GetWindowLong( window, GWL_EXSTYLE );
    exstyle &= ~( WS_EX_DLGMODALFRAME | WS_EX_COMPOSITED | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_LAYERED | WS_EX_STATICEDGE | WS_EX_TOOLWINDOW | WS_EX_APPWINDOW );
    SetWindowLong( window, GWL_EXSTYLE, exstyle );

    SetWindowPos(
        window,
        nullptr,
        monitorInfo.rcMonitor.left,
        monitorInfo.rcMonitor.top,
        // This doesn't actually change the rendering size.
        // Have to use these in case monitor size != actual "window" size
        ScreenWidth(), //monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left
        ScreenHeight(), //monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top
        SWP_NOSENDCHANGING | SWP_NOZORDER | SWP_NOOWNERZORDER
    );

    return true;
}

static BOOL CALLBACK FindAndSetBorderless( HWND hwnd, LPARAM lParam )
{
    if ( hwnd == nullptr)
        return TRUE;

    DWORD wpid;
    GetWindowThreadProcessId( hwnd, &wpid );

    if ( wpid != (DWORD)lParam )
        return TRUE;

    if ( GetWindow( hwnd, GW_OWNER ) != nullptr )
        return TRUE;

    if ( !IsWindowVisible( hwnd ) )
        return TRUE;

    if ( !ZMSetBorderlessFullscreen( hwnd ) )
        return TRUE;

    return FALSE;
}
#endif // WIN32

//
// We use command line arg instead of cvar because
// cvars don't seem to be loaded before getting here.
// + dealing with cvar changes will be pain anyway.
//
bool ZMIsBorderless()
{
    return CommandLine()->CheckParm( "-borderless" );
}

void ZMCheckBorderless()
{
    if ( ZMIsBorderless() )
    {
#ifdef WIN32
        EnumWindows( FindAndSetBorderless, GetCurrentProcessId() );
#endif // WIN32
    }
}

void ZMInitBorderless()
{
    // Whenever video settings are updated (and window settings are reset)
    g_pMaterialSystem->AddModeChangeCallBack( &ZMCheckBorderless );

    ZMCheckBorderless();
}

void ZMSetBorderless( bool state )
{
    bool updated = ZMIsBorderless() != state;

    if ( state )
    {
        CommandLine()->AppendParm( "-borderless", nullptr );
    }
    else
    {
        CommandLine()->RemoveParm( "-borderless" );
        
    }

    if ( updated )
    {
        ZMCheckBorderless();
    }
}
