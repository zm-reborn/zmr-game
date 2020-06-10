#include "cbase.h"
#ifdef GAME_DLL
#include "ammodef.h"
#include "bone_setup.h"

#include <collisionutils.h>
#endif



#include "zmr_usercmd.h"
#ifdef GAME_DLL
#include "zmr/weapons/zmr_base.h"
#include "zmr/npcs/zmr_zombiebase.h"
#include "zmr/zmr_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static ConVar zm_sv_clientsidehitdetection( "zm_sv_clientsidehitdetection", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED );

#ifdef GAME_DLL
ConVar zm_sv_debug_clientsidehitdetec( "zm_sv_debug_clientsidehitdetec", "0" );
#endif


void ZMUserCmdHitData_t::Merge( const ZMUserCmdHitData_t& hit )
{
    Assert( entindex == hit.entindex );

    int newhits = nHits + hit.nHits;
    if ( newhits > ZM_USERCMD_MAX_BULLETS_PER_SHOT )
    {
        Warning( "User cmd exceeded max bullets per shot!!!\n" );
        newhits = ZM_USERCMD_MAX_BULLETS_PER_SHOT;
    }

    int cpynum = newhits - nHits;
    for ( int i = 0; i < cpynum; i++ )
        hitgroups[nHits + i] = hit.hitgroups[i];

    nHits = newhits;
}

void ZMUserCmdHitData_t::WriteTo( bf_write* buf ) const
{
    int i;

	buf->WriteUBitLong( entindex, MAX_EDICT_BITS );
    buf->WriteUBitLong( nHits, ZM_NUMHITS_BITS );
    for ( i = 0; i < nHits; i++ )
		buf->WriteUBitLong( hitgroups[i], ZM_HITGROUP_BITS );
}

void ZMUserCmdHitData_t::ReadFrom( bf_read* buf )
{
    int n, i;

	entindex = buf->ReadUBitLong( MAX_EDICT_BITS );

    n = buf->ReadUBitLong( ZM_NUMHITS_BITS );
	nHits = MIN( n, ZM_USERCMD_MAX_BULLETS_PER_SHOT );


	for ( i = 0; i < nHits; i++ )
        hitgroups[i] = buf->ReadUBitLong( ZM_HITGROUP_BITS );
}




CZMUserCmdSystem::CZMUserCmdSystem() : CAutoGameSystem( "CZMUserCmdSystem" )
{

}

CZMUserCmdSystem::~CZMUserCmdSystem()
{
    
}

#ifdef CLIENT_DLL
void CZMUserCmdSystem::ClearDamage()
{
    m_vHitEnts.RemoveAll();
}

void CZMUserCmdSystem::WriteToCmd( CUserCmd& cmd )
{
    int len = m_vHitEnts.Count();
    len = MIN( len, ZM_USERCMD_MAX_HITS );
    cmd.zmHitData.CopyArray( m_vHitEnts.Base(), len );
}

void CZMUserCmdSystem::AddDamage( ZMUserCmdHitData_t hit )
{
    ZMUserCmdHitData_t* pHit = FindDamageByEntity( hit.entindex );
    if ( pHit )
    {
        pHit->Merge( hit );
        return;
    }

    if ( m_vHitEnts.Count() >= ZM_USERCMD_MAX_HITS )
    {
        Warning( "Can't add anymore hit data, exceeded limit of %i!!! (entity: %i)\n",
            ZM_USERCMD_MAX_HITS,
            hit.entindex );
        return;
    }

    m_vHitEnts.AddToTail( hit );
}

int CZMUserCmdSystem::FindDamageByEntity( int entindex ) const
{
    int len = m_vHitEnts.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_vHitEnts[i].entindex == entindex )
        {
            return i;
        }
    }

    return -1;
}

ZMUserCmdHitData_t* CZMUserCmdSystem::FindDamageByEntity( int entindex )
{
    int len = m_vHitEnts.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_vHitEnts[i].entindex == entindex )
        {
            return &(m_vHitEnts[i]);
        }
    }

    return nullptr;
}
#else
extern CAmmoDef* GetAmmoDef();

int CZMUserCmdSystem::ApplyDamage( CZMPlayer* pPlayer, const ZMServerWepData_t& wepdata, const ZMUserCmdHitList_t& list )
{
    const CUserCmd* pCmd = pPlayer->GetCurrentCommand();


    CZMBaseWeapon* pWep = wepdata.hWeapon.Get();
    if ( !pWep )
        return 0;

    //
    // The order in which the client presses the attack and actually doing ("predicting") it is off.
    // So the server actually receives the command with all the hit data late.
    // This is why we simply can't have a if NextPrimaryAttack <= curtime because the server has already shot.
    // We also need some sort of snapshot of the last time we shot to make sure the player's not cheating,
    // hence wepdata.
    //
    if ( (wepdata.iLastFireCommandNumber+1) > pCmd->command_number )
    {
        return 0;
    }


    int iAmmoType = wepdata.iAmmoType;
    int fDmgBits = GetAmmoDef()->DamageType( wepdata.iAmmoType );
    Vector vecSrc = wepdata.vecShootPos;
    int nHitsAccumulated = 0;
    int nEntitiesHitAccumulated = 0;

    CUtlVector<int> dealt;
    ZMUserCmdValidData_t data( pPlayer, pWep, vecSrc );

    // Loop through the entities and damage them.

    int nEntitiesHit = list.Count();
    for ( int i = 0; i < nEntitiesHit; i++ )
    {
        const ZMUserCmdHitData_t& cmddata = list[i];

        int entindex = cmddata.entindex;
        // Don't deal damage more than once to one entity.
        // The client should already combine these to one.
        if ( dealt.Find( entindex ) != -1 )
        {
            Msg( "Player %i trying to deal double damage to entity %i!\n", pPlayer->entindex(), entindex );
            continue;
        }


        CBaseEntity* pEntity = CBaseEntity::Instance( engine->PEntityOfEntIndex( entindex ) );
        if ( !pEntity )
            continue;

        if ( pEntity->m_takedamage != DAMAGE_YES )
            continue;

        if ( !pEntity->IsAlive() )
            continue;


        float flDamage = wepdata.flDamage;
        int nHits = cmddata.nHits;



        // Verify that the player isn't cheating
        data.pVictim = pEntity;
        data.nHits = nHits;
        data.nAlreadyHit = nHitsAccumulated;
        data.nEntitiesAlreadyHit = nEntitiesHitAccumulated;

        if ( !pWep->IsUserCmdHitsValid( data ) )
        {
            Msg( "Player %i invalid usercmd hits (entity %i)! (Reason: %s)\n",
                pPlayer->entindex(),
                entindex,
                pWep->GetUserCmdError() );
            continue;
        }


        // Construct a pseudo FireBullets to let the entity handle all the damage scaling, etc.
        
        // Copy the data back in case the validator updated anything.
        nHits = data.nHits;
        nHitsAccumulated = data.nAlreadyHit;
        nEntitiesHitAccumulated = data.nEntitiesAlreadyHit;

        Vector vecDefEnd = pEntity->WorldSpaceCenter();

        trace_t tr;
        tr.startpos = vecSrc;
        tr.endpos = vecDefEnd;
        tr.fraction = 1.0f; // We'll have to fraction it properly.


        Vector dir = tr.endpos - tr.startpos;
        dir.NormalizeInPlace();


        CTakeDamageInfo dmg( pPlayer, pPlayer, pWep, vec3_origin, vec3_origin, flDamage, fDmgBits );
        dmg.SetAmmoType( iAmmoType );

        if ( !wepdata.bIsMelee )
        {
            pPlayer->ModifyFireBulletsDamage( &dmg );
            CalculateBulletDamageForce( &dmg, dmg.GetAmmoType(), dir, tr.endpos );
        }
        else
        {
            CalculateMeleeDamageForce( &dmg, dir, tr.endpos );
        }

        bool bHasHitboxes = HasHitboxes( pEntity );

#ifdef _DEBUG
        if ( !bHasHitboxes && zm_sv_debug_clientsidehitdetec.GetBool() )
        {
            DevMsg( "Usercmd victim has no hitboxes!\n" );
        }
#endif

        int iLastHitGroup = -1;
        for ( int j = 0; j < nHits; j++ )
        {
            // Entity needs hitgroups
            tr.hitgroup = cmddata.hitgroups[j];


            // Compute the trace end if the hitgroup changes.
            if ( bHasHitboxes && tr.hitgroup != iLastHitGroup )
            {
                tr.endpos = vecDefEnd;
                ComputeTraceEnd( pEntity, tr );
            }

#ifdef _DEBUG
            if ( zm_sv_debug_clientsidehitdetec.GetFloat() > 0.0f )
            {
                QAngle ang;
                VectorAngles( tr.plane.normal, ang );
                NDebugOverlay::Axis( tr.endpos, ang, 8.0f, true, zm_sv_debug_clientsidehitdetec.GetFloat() );
            }
#endif


            pEntity->DispatchTraceAttack( dmg, dir, &tr, nullptr );

            iLastHitGroup = tr.hitgroup;
        }

        if ( g_MultiDamage.GetDamage() != 0.0f )
        {
            DevMsg( "Applying %.0f damage to entity %i from user cmd!\n", g_MultiDamage.GetDamage(), entindex );
        }

        // The entity adds the damages to a multidamage in TraceAttack
        // This fires the TakeDamage on the entity.
        ApplyMultiDamage();

        dealt.AddToTail( entindex );

        nHitsAccumulated += nHits;
        ++nEntitiesHitAccumulated;
    }

    return dealt.Count();
}

bool CZMUserCmdSystem::HasHitboxes( CBaseEntity* pVictim )
{
    // Does it have proper hitboxes with multiple hitgroups?

    CBaseAnimating* pAnim = pVictim->GetBaseAnimating();
    if ( !pAnim )
        return false;

    CStudioHdr* pHdr = pAnim->GetModelPtr();
    if ( !pHdr )
        return false;

    mstudiohitboxset_t* set = pHdr->pHitboxSet( pAnim->GetHitboxSet() );
    if ( !set || !set->numhitboxes )
        return false;


    int iHitGroup = -1;

    for ( int i = 0; i < set->numhitboxes; i++ )
    {
        mstudiobbox_t* pBox = set->pHitbox( i );
        if ( iHitGroup != -1 && pBox->group != iHitGroup )
        {
            return true;
        }

        iHitGroup = pBox->group;
    }

    return false;
}

bool CZMUserCmdSystem::ComputeTraceEnd( CBaseEntity* pVictim, trace_t& tr )
{
    // Compute where the trace could've ended based on the hitgroup.
    // The victim may need that to figure out how much damage they receive.

    CBaseAnimating* pAnim = pVictim->GetBaseAnimating();
    if ( !pAnim )
        return false;

    CStudioHdr* pHdr = pAnim->GetModelPtr();
    if ( !pHdr )
        return false;

    mstudiohitboxset_t* set = pHdr->pHitboxSet( pAnim->GetHitboxSet() );
    if ( !set || !set->numhitboxes )
        return false;


    // The model will have multiple hitboxes with the same hitgroup but it's not a big deal, really.
    int iHitGroup = tr.hitgroup;

    for ( int i = 0; i < set->numhitboxes; i++ )
    {
        mstudiobbox_t* pBox = set->pHitbox( i );
        if ( pBox->group != iHitGroup )
            continue;
        

        // We found the bone with the same hitgroup, now compute the actual intersection.

        int iBone = pBox->bone;
        mstudiobone_t* pBone = pHdr->pBone( iBone );


        tr.surface.flags = SURF_HITBOX;
        tr.surface.surfaceProps = physprops->GetSurfaceIndex( pBone->pszSurfaceProp() );
        tr.physicsbone = pBone->physicsbone; // We need the physics bone for proper ragdolling.


        matrix3x4_t* pMatrix = pAnim->GetBoneCache()->GetCachedBone( iBone );
        if ( !pMatrix )
            return false;


        MatrixPosition( *pMatrix, tr.endpos );

        // Center it
        Vector boxextents;
        for ( int j = 0; j < 3; j++ )
            boxextents[j] = (pBox->bbmin[j] + pBox->bbmax[j]) * 0.5f;

        VectorTransform( boxextents, *pMatrix, tr.endpos );


        // Do the actual magic
        //trace_t tempTrace;
        //if ( IntersectRayWithBox( start, delta2, pBox->bbmin, pBox->bbmax, 0.0f, &tempTrace ) )
        //{
        //    tr.endpos = tempTrace.endpos;
        //}

        return true;
    }
    
    return false;
}
#endif

bool CZMUserCmdSystem::UsesClientsideDetection()
{
    return zm_sv_clientsidehitdetection.GetBool();
}

bool CZMUserCmdSystem::UsesClientsideDetection( CBaseEntity* pEnt ) const
{
    //
    // NOTE: Client will still have to manually insert hit data from TraceAttack.
    //
    if ( !UsesClientsideDetection() )
        return false;

#ifdef CLIENT_DLL
    return pEnt && pEnt->IsNPCR();
#else
    return pEnt && pEnt->IsBaseZombie();
#endif
}


CZMUserCmdSystem g_ZMUserCmdSystem;
