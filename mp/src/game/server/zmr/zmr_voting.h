#pragma once

#include "cbase.h"
#include "vote_controller.h"

#include "zmr/zmr_shareddefs.h"


class CZMVoteRoundRestart : public CBaseIssue
{
public:
    CZMVoteRoundRestart() : CBaseIssue( "ZMVoteRoundRestart" ) {}

    virtual bool IsEnabled() OVERRIDE { return true; };
    virtual bool IsTeamRestrictedVote() OVERRIDE { return true; };
    virtual const char* GetDisplayString( void ) OVERRIDE { return "#ZMVoteRoundRestart"; };
    virtual bool IsYesNoVote() OVERRIDE { return true; };
    virtual const char* GetDetailsString( void ) OVERRIDE { return ""; };
    virtual bool CanTeamCallVote( int iTeam ) const OVERRIDE { return iTeam != ZMTEAM_ZM; };

    virtual void ExecuteCommand( void ) OVERRIDE
    {
        Msg( "Executing round restart, WOAH\n" );
    }

    virtual void ListIssueDetails( CBasePlayer* pForWhom ) OVERRIDE
    {
        if ( !pForWhom ) return;

        Msg( "Restart the mothafucking round!\n" );
    }
};
