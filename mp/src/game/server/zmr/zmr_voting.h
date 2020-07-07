#pragma once

#include "cbase.h"
#include "vote_controller.h"

#include "zmr_gamerules.h"
#include "zmr_shareddefs.h"


class CZMVoteRoundRestart : public CBaseIssue
{
public:
    CZMVoteRoundRestart() : CBaseIssue( "ZMVoteRoundRestart" ) {}

    virtual bool IsEnabled() OVERRIDE { return !ZMRules()->IsInRoundEnd(); };
    virtual bool IsTeamRestrictedVote() OVERRIDE { return false; }; // This will include the ZM, but otherwise it would be completely team restricted.
    virtual const char* GetDisplayString( void ) OVERRIDE { return "#ZMVoteRoundRestart"; };
    virtual bool IsYesNoVote() OVERRIDE { return true; };
    virtual const char* GetDetailsString( void ) OVERRIDE { return ""; };
    virtual bool CanTeamCallVote( int iTeam ) const OVERRIDE { return iTeam != ZMTEAM_ZM; };

    virtual void ListIssueDetails( CBasePlayer* pForWhom ) OVERRIDE {}

    virtual void ExecuteCommand( void ) OVERRIDE
    {
        ZMRules()->EndRound( ZMROUND_VOTERESTART );
    }

    virtual bool CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime ) OVERRIDE
    {
        if ( ZMRules()->IsInRoundEnd() )
            return false;

        return CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime );
    }
};
