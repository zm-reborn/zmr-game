#include "cbase.h"

#include "zmr_options_tab.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CZMOptionsSub::CZMOptionsSub( vgui::Panel* parent ) : BaseClass( parent, nullptr )
{
    m_bFailedLoad = false;
}

CZMOptionsSub::~CZMOptionsSub()
{
}
