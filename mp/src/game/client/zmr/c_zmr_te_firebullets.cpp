//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_basetempentity.h"
#include <cliententitylist.h>
#include "ammodef.h"
#include "c_te_effect_dispatch.h"
#include "shot_manipulator.h"


#include "c_zmr_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class C_ZMTEFireBullets : public C_BaseTempEntity
{
public:
    DECLARE_CLASS( C_ZMTEFireBullets, C_BaseTempEntity );
    DECLARE_CLIENTCLASS();

    virtual void PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;

    void CreateEffects();

    static void ThirdpersonMuzzleflash( C_BaseCombatWeapon* pWpn );

public:
    int		m_iPlayer;
    Vector	m_vecOrigin;
    Vector  m_vecDir;
    int		m_iAmmoID;
    int		m_iWeaponIndex;
    int		m_iSeed;
    float	m_flSpread;
    int		m_iShots;
    bool	m_bDoImpacts;
    bool	m_bDoTracers;
};

class CTraceFilterSkipPlayerAndViewModelOnly : public CTraceFilter
{
public:
    virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
    {
        C_BaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
        if( pEntity &&
            ( ( dynamic_cast<C_BaseViewModel *>( pEntity ) != NULL ) ||
            ( dynamic_cast<C_BasePlayer *>( pEntity ) != NULL ) ) )
        {
            return false;
        }
        else
        {
            return true;
        }
    }
};

ConVar zm_cl_muzzleflash_thirdperson_type( "zm_cl_muzzleflash_thirdperson_type", "2" );
void C_ZMTEFireBullets::CreateEffects()
{
    CAmmoDef* pAmmoDef = GetAmmoDef();

    if ( !pAmmoDef )
         return;


    auto* pPlayer = ToZMPlayer( ClientEntityList().GetEnt( m_iPlayer ) );
    if ( !pPlayer || !pPlayer->GetActiveWeapon() )
        return;


    auto* pWpn = pPlayer->GetActiveWeapon();
    if ( !pWpn )
        return;


    int iSeed = m_iSeed;

    int iAttachment = pWpn->LookupAttachment( "muzzle" );

                    
    CShotManipulator Manipulator( m_vecDir );

    for ( int iShot = 0; iShot < m_iShots; iShot++ )
    {
        RandomSeed( iSeed );	// init random system with this seed

        // Don't run the biasing code for the player at the moment.
        Vector vecDir = Manipulator.ApplySpread( Vector( m_flSpread, m_flSpread, m_flSpread ) );
        Vector vecEnd = m_vecOrigin + vecDir * MAX_TRACE_LENGTH;
        trace_t tr;
        CTraceFilterSkipPlayerAndViewModelOnly traceFilter;

        UTIL_TraceLine( m_vecOrigin, vecEnd, MASK_SHOT, &traceFilter, &tr);

        if ( m_bDoTracers )
        {
            const char* pTracerName = pWpn->GetTracerType();

            CEffectData data;
            data.m_vStart = tr.startpos;
            data.m_vOrigin = tr.endpos;
            data.m_hEntity = pWpn->GetRefEHandle();
            data.m_flScale = 0.0f;

            data.m_nAttachmentIndex = iAttachment;

            if ( data.m_nAttachmentIndex > 0 )
                data.m_fFlags |= TRACER_FLAG_USEATTACHMENT;


            if ( pTracerName && *pTracerName )
            {
                DispatchEffect( pTracerName, data );
            }
            else
            {
                DispatchEffect( "Tracer", data );
            }
        }
                    
        if ( m_bDoImpacts )
        {
            pWpn->DoImpactEffect( tr, pAmmoDef->DamageType( m_iAmmoID ) );
        }

        iSeed++;
    }


    ThirdpersonMuzzleflash( pWpn );
}

void C_ZMTEFireBullets::ThirdpersonMuzzleflash( C_BaseCombatWeapon* pWpn )
{
    //
    // ZMRTODO: Unhack this.
    // Here for now, we should probably use the world model animations to dispatch this.
    //
    char effect[64];
    effect[0] = NULL;

    switch ( zm_cl_muzzleflash_thirdperson_type.GetInt() )
    {
    case 1 :
        Q_strncpy( effect, "PISTOL", sizeof( effect ) );
        break;
    case 2 :
        Q_strncpy( effect, "SMG1", sizeof( effect ) );
        break;
    case 3 :
        Q_strncpy( effect, "SHOTGUN", sizeof( effect ) );
        break;
    case 4 :
        Q_strncpy( effect, "357", sizeof( effect ) );
        break;
    case 5 :
        Q_strncpy( effect, "RPG", sizeof( effect ) );
        break;
    case 6 :
        Q_strncpy( effect, "COMBINE", sizeof( effect ) );
        break;
    default :
        break;
    }


    if ( effect[0] != NULL )
    {
        char options[64];
        Q_snprintf( options, sizeof( options ), "%s muzzle", effect );

        pWpn->DispatchMuzzleEffect( options, false );
    }
}

void C_ZMTEFireBullets::PostDataUpdate( DataUpdateType_t updateType )
{
    if ( m_bDoTracers || m_bDoImpacts )
    {
        CreateEffects();
    }
}


IMPLEMENT_CLIENTCLASS_EVENT( C_ZMTEFireBullets, DT_ZM_TE_FireBullets, CZMTEFireBullets );


BEGIN_RECV_TABLE_NOBASE( C_ZMTEFireBullets, DT_ZM_TE_FireBullets )
    RecvPropVector( RECVINFO( m_vecOrigin ) ),
    RecvPropVector( RECVINFO( m_vecDir ) ),
    RecvPropInt( RECVINFO( m_iAmmoID ) ),
    RecvPropInt( RECVINFO( m_iSeed ) ),
    RecvPropInt( RECVINFO( m_iShots ) ),
    RecvPropInt( RECVINFO( m_iPlayer ) ),
    RecvPropInt( RECVINFO( m_iWeaponIndex ) ),
    RecvPropFloat( RECVINFO( m_flSpread ) ),
    RecvPropBool( RECVINFO( m_bDoImpacts ) ),
    RecvPropBool( RECVINFO( m_bDoTracers ) ),
END_RECV_TABLE()
