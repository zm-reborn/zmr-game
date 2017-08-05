#pragma once


class CZMCharCircle
{
public:
    CZMCharCircle(){}


    void Draw();

    void SetSize( float );
    void SetMaterial( const char* );
    void SetPos( const Vector& );
    void SetColor( float, float, float );
    void SetAlpha( float );
    void SetYaw( float );
    
protected:
    float m_flSize;
    Vector m_vecOrigin;
    float m_flColor[4];
    IMaterial* m_pMaterial;
    float m_flYaw;

private:
    Vector m_Points[4];
};