#pragma once


class CZMCharCircle
{
public:
    CZMCharCircle(){}


    void Draw();

    void SetSize( float size );
    void SetMaterial( const char* name );
    void SetPos( const Vector& origin );
    void SetColor( float r, float g, float b );
    void SetAlpha( float a );
    void SetYaw( float yaw );
    
protected:
    float m_flSize;
    Vector m_vecOrigin;
    float m_flColor[4];
    IMaterial* m_pMaterial;
    float m_flYaw;

private:
    Vector m_Points[4];
};
