//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IScheme.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "materialsystem/imaterialsystem.h"
#include "engine/ivmodelinfo.h"

#include "c_sceneentity.h"
#include "gamestringpool.h"
#include "model_types.h"
#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "iefx.h"
#include "dlight.h"
#include "activitylist.h"

#include "zmr_panel_modelpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;

DECLARE_BUILD_FACTORY( CZMModelPanel );


CZMModelPanel::CZMModelPanel( vgui::Panel* pParent, const char* pName ) : CModelPanel( pParent, pName )
{
}

void CZMModelPanel::DeleteModelData( void )
{
    if ( m_hModel.Get() )
    {
#ifdef ZMR // ZMRCHANGE
        // Don't know if this is a good idea, but you'd think there'd be some problem with having player models client-only.
        // Doesn't actually do anything(?)
        /*int index = m_hModel->GetModelIndex();

        if ( IsClientOnlyModelIndex( index ) )
        {
            modelinfo->ReleaseDynamicModel( index );
        }*/
#endif

        m_hModel->Remove();
        m_hModel = NULL;
        m_flFrameDistance = 0;
    }

    for ( int i = 0 ; i < m_AttachedModels.Count() ; i++ )
    {
        if ( m_AttachedModels[i].Get() )
        {
            m_AttachedModels[i]->Remove();
        }
        m_AttachedModels.Remove( i );
    }
}

void CZMModelPanel::SetupModel( void )
{
    if ( !m_pModelInfo )
        return; 

    MDLCACHE_CRITICAL_SECTION();

    // remove any current models we're using
    DeleteModelData();

    const char *pszModelName = GetModelName();
    if ( !pszModelName || !pszModelName[0] )
        return;

    // create the new model
    CModelPanelModel *pEnt = new CModelPanelModel;

    if ( !pEnt )
        return;

#ifdef ZMR // ZMRCHANGE
    // This will fail if we're not in-game.
    // Therefore we need to register a client only dynamic model to get around it.
    int index;
    index = modelinfo->GetModelIndex( pszModelName );

    if ( index == -1 )
    {
        // No model found, register our own.
        index = modelinfo->RegisterDynamicModel( pszModelName, true );

        if ( index == -1 || !pEnt->InitializeAsClientEntityByIndex( index, RENDER_GROUP_OPAQUE_ENTITY ) )
        {
            // Even that failed, alright.
            index = -1;
        }
    }
    else
    {
        // Just initialize normally.
        if ( !pEnt->InitializeAsClientEntityByIndex( index, RENDER_GROUP_OPAQUE_ENTITY ) )
        {
            index = -1;
        }
    }

    if ( index == -1 )
    {
        pEnt->Remove();
        return;
    }
#else
    if ( pEnt->InitializeAsClientEntity( pszModelName, RENDER_GROUP_OPAQUE_ENTITY ) == false )
    {
        // we failed to initialize this entity so just return gracefully
        pEnt->Remove();
        return;
    }
#endif
    
    // setup the handle
    m_hModel = pEnt;

    pEnt->DontRecordInTools();
    pEnt->AddEffects( EF_NODRAW ); // don't let the renderer draw the model normally

    if ( m_pModelInfo->m_nSkin >= 0 )
    {
        pEnt->m_nSkin = m_pModelInfo->m_nSkin;
    }

    // do we have any animation information?
    if ( m_pModelInfo->m_Animations.Count() > 0 && m_pModelInfo->m_Animations.IsValidIndex( GetDefaultAnimation() ) )
    {
        CModelPanelModelAnimation *pAnim = m_pModelInfo->m_Animations[ GetDefaultAnimation() ];
        int sequence = ACT_INVALID;
        if ( pAnim->m_pszActivity && pAnim->m_pszActivity[0] )
        {
            Activity activity = (Activity)ActivityList_IndexForName( pAnim->m_pszActivity );
            sequence = pEnt->SelectWeightedSequence( activity );
        }
        else if ( pAnim->m_pszSequence && pAnim->m_pszSequence[0] )
        {
            sequence = pEnt->LookupSequence( pAnim->m_pszSequence );
        }
        if ( sequence != ACT_INVALID )
        {
            pEnt->ResetSequence( sequence );
            pEnt->SetCycle( 0 );

            if ( pAnim->m_pPoseParameters )
            {
                for ( KeyValues *pData = pAnim->m_pPoseParameters->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
                {
                    const char *pName = pData->GetName();
                    float flValue = pData->GetFloat();
        
                    pEnt->SetPoseParameter( pName, flValue );
                }
            }

            pEnt->m_flAnimTime = gpGlobals->curtime;
        }
    }

    // setup any attached models
    for ( int i = 0 ; i < m_pModelInfo->m_AttachedModelsInfo.Count() ; i++ )
    {
        CModelPanelAttachedModelInfo *pInfo = m_pModelInfo->m_AttachedModelsInfo[i];
        C_BaseAnimating *pTemp = new C_BaseAnimating;

        if ( pTemp )
        {
            if ( pTemp->InitializeAsClientEntity( pInfo->m_pszModelName, RENDER_GROUP_OPAQUE_ENTITY ) == false )
            {	
                // we failed to initialize this model so just skip it
                pTemp->Remove();
                continue;
            }

            pTemp->DontRecordInTools();
            pTemp->AddEffects( EF_NODRAW ); // don't let the renderer draw the model normally
            pTemp->FollowEntity( m_hModel.Get() ); // attach to parent model

            if ( pInfo->m_nSkin >= 0 )
            {
                pTemp->m_nSkin = pInfo->m_nSkin;
            }

            pTemp->m_flAnimTime = gpGlobals->curtime;
            m_AttachedModels.AddToTail( pTemp );
        }
    }

    CalculateFrameDistance();
}

void CZMModelPanel::Paint()
{
    EditablePanel::Paint();
#ifdef ZMR // ZMRCHANGE: We will be drawing this out of map.
    if ( !m_pModelInfo )
        return;
#else
    C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

    if ( !pLocalPlayer || !m_pModelInfo )
        return;
#endif
    MDLCACHE_CRITICAL_SECTION();

    UpdateModel();

    if ( !m_hModel.Get() )
        return;

    int i = 0;
    int x, y, w, h;

    GetBounds( x, y, w, h );
    ParentLocalToScreen( x, y );

    Vector vecExtraModelOffset( 0, 0, 0 );
    float flWidthRatio = ((float)w / (float)h ) / ( 4.0f / 3.0f );

    // is this a player model?
    if ( Q_strstr( GetModelName(), "models/player/" ) )
    {
        // need to know if the ratio is not 4/3
        // HACK! HACK! to get our player models to appear the way they do in 4/3 if we're using other aspect ratios
        if ( flWidthRatio > 1.05f ) 
        {
            vecExtraModelOffset.Init( -60, 0, 0 );
        }
        else if ( flWidthRatio < 0.95f )
        {
            vecExtraModelOffset.Init( 15, 0, 0 );
        }
    }

    m_hModel->SetAbsOrigin( m_pModelInfo->m_vecOriginOffset + vecExtraModelOffset );
    m_hModel->SetAbsAngles( QAngle( m_pModelInfo->m_vecAbsAngles.x, m_pModelInfo->m_vecAbsAngles.y, m_pModelInfo->m_vecAbsAngles.z ) );

    // do we have a valid sequence?
    if ( m_hModel->GetSequence() != -1 )
    {
        m_hModel->FrameAdvance( gpGlobals->frametime );
    }

    CMatRenderContextPtr pRenderContext( materials );
    
    // figure out what our viewport is right now
    int viewportX, viewportY, viewportWidth, viewportHeight;
    pRenderContext->GetViewport( viewportX, viewportY, viewportWidth, viewportHeight );

    // Now draw it.
    CViewSetup view;
    view.x = x + m_pModelInfo->m_vecViewportOffset.x + viewportX; // we actually want to offset by the 
    view.y = y + m_pModelInfo->m_vecViewportOffset.y + viewportY; // viewport origin here because Push3DView expects global coords below
    view.width = w;
    view.height = h;

    view.m_bOrtho = false;

    // scale the FOV for aspect ratios other than 4/3
    view.fov = ScaleFOVByWidthRatio( m_nFOV, flWidthRatio );

    view.origin = vec3_origin;
    view.angles.Init();
    view.zNear = VIEW_NEARZ;
    view.zFar = 1000;

    

    // Not supported by queued material system - doesn't appear to be necessary
//	ITexture *pLocalCube = pRenderContext->GetLocalCubemap();
#ifndef ZMR // ZMRCHANGE: Don't use cubemaps, they fuck up map's own cubemaps. No reason to use them anyway.
    if ( g_pMaterialSystemHardwareConfig->GetHDREnabled() )
    {
        pRenderContext->BindLocalCubemap( m_DefaultHDREnvCubemap );
    }
    else
    {
        pRenderContext->BindLocalCubemap( m_DefaultEnvCubemap );
    }
#endif
    pRenderContext->SetLightingOrigin( vec3_origin );
    pRenderContext->SetAmbientLight( 0.4, 0.4, 0.4 );

    static Vector white[6] = 
    {
        Vector( 0.4, 0.4, 0.4 ),
        Vector( 0.4, 0.4, 0.4 ),
        Vector( 0.4, 0.4, 0.4 ),
        Vector( 0.4, 0.4, 0.4 ),
        Vector( 0.4, 0.4, 0.4 ),
        Vector( 0.4, 0.4, 0.4 ),
    };

    g_pStudioRender->SetAmbientLightColors( white );
    g_pStudioRender->SetLocalLights( 0, NULL );

    if ( m_pModelInfo->m_bUseSpotlight )
    {
        Vector vecMins, vecMaxs;
        m_hModel->GetRenderBounds( vecMins, vecMaxs );
        LightDesc_t spotLight( vec3_origin + Vector( 0, 0, 200 ), Vector( 1, 1, 1 ), m_hModel->GetAbsOrigin() + Vector( 0, 0, ( vecMaxs.z - vecMins.z ) * 0.75 ), 0.035, 0.873 );
        g_pStudioRender->SetLocalLights( 1, &spotLight );
    }

    Frustum dummyFrustum;
    render->Push3DView( view, 0, NULL, dummyFrustum );

    modelrender->SuppressEngineLighting( true );
    float color[3] = { 1.0f, 1.0f, 1.0f };
    render->SetColorModulation( color );
    render->SetBlend( 1.0f );
    m_hModel->DrawModel( STUDIO_RENDER );

    for ( i = 0 ; i < m_AttachedModels.Count() ; i++ )
    {
        if ( m_AttachedModels[i].Get() )
        {
            m_AttachedModels[i]->DrawModel( STUDIO_RENDER );
        }
    }

    modelrender->SuppressEngineLighting( false );
    
    render->PopView( dummyFrustum );
#ifndef ZMR // ZMRCHANGE: Don't use cubemaps, they fuck up map's own cubemaps. No reason to use them anyway.
    pRenderContext->BindLocalCubemap( NULL );
#endif
    /*
    vgui::surface()->DrawSetColor( Color(0,0,0,255) );
    vgui::surface()->DrawOutlinedRect( 0,0, GetWide(), GetTall() );
    */
}
