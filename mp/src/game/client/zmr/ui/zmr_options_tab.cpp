#include "cbase.h"

#include "zmr_options_tab.h"


CZMOptionsSub::CZMOptionsSub( vgui::Panel* parent ) : BaseClass( parent, nullptr )
{
    m_bFailedLoad = false;
}

CZMOptionsSub::~CZMOptionsSub()
{
}
