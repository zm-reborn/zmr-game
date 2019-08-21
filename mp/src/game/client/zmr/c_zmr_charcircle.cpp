#include "cbase.h"
//#include "fx_quad.h"


#include "c_zmr_charcircle.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void CZMCharCircle::Draw()
{
    Vector p1( -m_flSize, -m_flSize, 0 );
    Vector p2( -m_flSize, m_flSize, 0 );
    Vector p3( m_flSize, m_flSize, 0 );
    Vector p4( m_flSize, -m_flSize, 0 );
    Vector normal( 0, 0, 1 );
    Vector pos;

    if ( m_flYaw != 0.0f )
    {
        pos = p1;
        VectorYawRotate( pos, m_flYaw, p1 );

        pos = p2;
        VectorYawRotate( pos, m_flYaw, p2 );

        pos = p3;
        VectorYawRotate( pos, m_flYaw, p3 );

        pos = p4;
        VectorYawRotate( pos, m_flYaw, p4 );
    }
    

    CMatRenderContextPtr pRenderContext( materials );

	IMesh* pMesh = pRenderContext->GetDynamicMesh( true, nullptr, nullptr, m_pMaterial );
	CMeshBuilder meshBuilder;

    meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

    pos = m_vecOrigin + p1;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( normal.Base() );
	meshBuilder.TexCoord2f( 0.0f, 1.0f, 1.0f );
	meshBuilder.Color4fv( m_flColor );
	meshBuilder.AdvanceVertex();

    pos = m_vecOrigin + p2;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( normal.Base() );
	meshBuilder.TexCoord2f( 0.0f, 0.0f, 1.0f );
	meshBuilder.Color4fv( m_flColor );
	meshBuilder.AdvanceVertex();


    pos = m_vecOrigin + p3;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( normal.Base() );
	meshBuilder.TexCoord2f( 0.0f, 0.0f, 0.0f );
	meshBuilder.Color4fv( m_flColor );
	meshBuilder.AdvanceVertex();


    pos = m_vecOrigin + p4;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( normal.Base() );
	meshBuilder.TexCoord2f( 0.0f, 1.0f, 0.0f );
	meshBuilder.Color4fv( m_flColor );
	meshBuilder.AdvanceVertex();


	meshBuilder.End();
	pMesh->Draw();
}

void CZMCharCircle::SetSize( float size )
{
    m_flSize = size;
}

void CZMCharCircle::SetMaterial( const char* name )
{
    m_pMaterial = materials->FindMaterial( name, TEXTURE_GROUP_CLIENT_EFFECTS );
        
    if ( m_pMaterial )
    {
        m_pMaterial->IncrementReferenceCount();
    }
}

void CZMCharCircle::SetPos( const Vector& origin )
{
    m_vecOrigin = origin;
}

void CZMCharCircle::SetColor( float r, float g, float b )
{
    m_flColor[0] = r;
    m_flColor[1] = g;
    m_flColor[2] = b;
}

void CZMCharCircle::SetAlpha( float a )
{
    m_flColor[3] = a;
}

void CZMCharCircle::SetYaw( float yaw )
{
    m_flYaw = yaw;
}
