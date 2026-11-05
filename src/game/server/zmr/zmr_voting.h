#pragma once

#include "cbase.h"
#include "vote_controller.h"

#include "zmr_gamerules.h"
#include "zmr_shareddefs.h"


class CZMVoteRoundRestart : public CBaseIssue
{
public:
    CZMVoteRoundRestart( CVoteController* pVoteController  ) : CBaseIssue( "ZMVoteRoundRestart", pVoteController ) {}

    virtual bool IsEnabled() override { return !ZMRules()->IsInRoundEnd(); };
    virtual bool IsTeamRestrictedVote() override { return false; }; // This will include the ZM, but otherwise it would be completely team restricted.
    virtual const char* GetDisplayString( void ) override { return "#ZMVoteRoundRestart"; };
    virtual bool IsYesNoVote() override { return true; };
    virtual const char* GetDetailsString( void ) override { return ""; };
    virtual bool CanTeamCallVote( int iTeam ) const override { return iTeam != ZMTEAM_ZM; };

    virtual void ListIssueDetails( CBasePlayer* pForWhom ) override {}

    virtual void ExecuteCommand( void ) override
    {
        ZMRules()->EndRound( ZMROUND_VOTERESTART );
    }

    virtual bool RequestCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime ) override
    {
        if ( ZMRules()->IsInRoundEnd() )
            return false;

        return CBaseIssue::RequestCallVote( nEntIndex, pszDetails, nFailCode, nTime );
    }
};
