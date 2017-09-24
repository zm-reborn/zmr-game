#pragma once

#include <clientscoreboarddialog.h>

//-----------------------------------------------------------------------------
// Purpose: Game ScoreBoard
//-----------------------------------------------------------------------------
class CZMClientScoreBoardDialog : public CClientScoreBoardDialog
{
private:
    DECLARE_CLASS_SIMPLE( CZMClientScoreBoardDialog, CClientScoreBoardDialog );
    
public:
    CZMClientScoreBoardDialog( IViewPort *pViewPort );
    ~CZMClientScoreBoardDialog();


protected:
    // scoreboard overrides
    virtual void InitScoreboardSections();
    virtual void UpdateTeamInfo();
    virtual bool GetPlayerScoreInfo(int playerIndex, KeyValues *outPlayerInfo);
    virtual void UpdatePlayerInfo();

    // vgui overrides for rounded corner background
    virtual void PaintBackground();
    virtual void PaintBorder();
    virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:
    virtual void AddHeader(); // add the start header of the scoreboard
    virtual void AddSection(int teamType, int teamNumber); // add a new section header for a team

    int GetSectionFromTeamNumber( int teamNumber );

    bool DisplayTeamCount( int iTeam );


    // Max is 520.
    enum { NAME_WIDTH = 200, SCORE_WIDTH = 100, DEATH_WIDTH = 80, PING_WIDTH = 60 };


    // rounded corners
    Color					 m_bgColor;
    Color					 m_borderColor;
};
