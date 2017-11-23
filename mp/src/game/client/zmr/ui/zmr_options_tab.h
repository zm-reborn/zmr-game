#pragma once

#include <vgui_controls/PropertyPage.h>


class CZMOptionsSub : public vgui::PropertyPage
{
public:
    DECLARE_CLASS_SIMPLE( CZMOptionsSub, vgui::PropertyPage );

    CZMOptionsSub( vgui::Panel* parent );
    ~CZMOptionsSub();

    inline bool FailedLoad() { return m_bFailedLoad; };


protected:
    template <class SubItemType>
    void LoadItem( SubItemType** dest, const char* name )
    {
        *dest = dynamic_cast<SubItemType*>( FindChildByName( name ) );

        if ( !*dest )
        {
            Warning( "%s couldn't load item %s.\n", GetPanelClassName(), name );

            m_bFailedLoad = true;
        }
    }

private:
    bool m_bFailedLoad;
};
