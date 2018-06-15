#pragma once


#include "npcr_path_follow.h"
#include "npcr_basenpc.h"


namespace NPCR
{
    class CChaseNavPath : public CFollowNavPath
    {
    public:
        DECLARE_CLASS( CChaseNavPath, CFollowNavPath );

        void Update( CBaseNPC* pNPC, CBaseCombatCharacter* pTarget, const CBasePathCost& cost );
        void Update( CBaseNPC* pNPC, CBaseEntity* pTarget, const CBasePathCost& cost );

        virtual bool NeedsRepath( CBaseNPC* pNPC, CBaseEntity* pTarget );


        // Don't decelerate when chasing a target.
        virtual bool ShouldDecelerateToGoal() const OVERRIDE { return false; }

    private:
        virtual void Update( CBaseNPC* pNPC ) OVERRIDE { CFollowNavPath::Update( pNPC ); };
    };
}
