#pragma once


#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/TextEntry.h>


#include "c_zmr_zmkeys.h"

#include "zmr_options_tab.h"


class VControlsListPanel;
enum ButtonCode_t;

class CZMOptionsSubKeys : public CZMOptionsSub
{
public:
    DECLARE_CLASS_SIMPLE( CZMOptionsSubKeys, CZMOptionsSub );

    CZMOptionsSubKeys( vgui::Panel* parent );
    ~CZMOptionsSubKeys();

    virtual void OnApplyChanges() OVERRIDE;
    virtual void OnResetData() OVERRIDE;

    virtual void OnKeyCodeTyped( vgui::KeyCode code ) OVERRIDE;

    virtual void OnThink() OVERRIDE;

    bool IsValidKeyForBinding( ButtonCode_t code );

    virtual void OnCommand( const char* command ) OVERRIDE;

    MESSAGE_FUNC_INT( ItemSelected, "ItemSelected", itemID );

private:
    void FillKeys( zmkeydatalist_t& zmkeys, zmkeydatalist_t& survivorkeys );
    void ParseKeys();
    void BindCommand( KeyValues* item, ButtonCode_t code );
    void SaveKeysToConfig();


    vgui::Button* m_pEditBtn;
    vgui::Button* m_pClearBtn;
    vgui::Button* m_pDefaultBtn;


    VControlsListPanel* m_pKeyBindList;
};
