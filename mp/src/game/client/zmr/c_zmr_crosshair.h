#pragma once




//
abstract_class CZMCrosshairPartMat
{
public:
    CZMCrosshairPartMat();

protected:
    void DrawMaterial( const char* pszMat, int x0, int y0, int x1, int y1 );

private:
    int m_nTexId;

};
//

//
abstract_class CZMBaseCrosshair : public CZMCrosshairPartMat
{
public:
    CZMBaseCrosshair();
    ~CZMBaseCrosshair();

    virtual void LoadValues( KeyValues* kv );
    virtual void WriteValues( KeyValues* kv ) const;

    virtual void Draw() = 0;
    virtual void GetDrawPosition( float& flPosX, float& flPosY ) const;


    float GetOffsetFromCenter() const { return m_flOffsetFromCenter; }
    // Width = Y, Length = X
    float GetOutlineWidth() const { return m_flOutlineSize; } 
    float GetDotSize() const { return m_flDotSize; }
    
    const Color& GetMainColor() const;
    const Color& GetOutlineColor() const;


    // Set to -1, -1 to disable override
    void SetOverrideCenter( float x, float y )
    {
        m_flOverrideCenterX = x;
        m_flOverrideCenterY = y;
    }


    bool DisplayInMenu() const { return m_bDisplayInMenu; }
    const char* GetName() const { return m_szName; }
    const char* GetMenuName() const { return m_szMenuName; }

protected:
    virtual void DrawDot();



    float m_flOverrideCenterX;
    float m_flOverrideCenterY;

private:
    bool m_bDisplayInMenu;
    char* m_szName;
    char* m_szMenuName;
    float m_flOutlineSize;
    float m_flOffsetFromCenter;
    float m_flDotSize;
    Color m_Color;
    Color m_OutlineColor;
};
//

//
class CZMEmptyCrosshair : public CZMBaseCrosshair
{
public:
    virtual void WriteValues( KeyValues* kv ) const OVERRIDE;
    virtual void Draw() OVERRIDE;

};
//


//
class CZMBaseDynamicCrosshair : public CZMBaseCrosshair
{
public:
    CZMBaseDynamicCrosshair();


    virtual void LoadValues( KeyValues* kv ) OVERRIDE;
    virtual void WriteValues( KeyValues* kv ) const OVERRIDE;


    virtual float GetMaxDynamicMove() const { return m_flDynamicMove; }
    virtual float GetDynamicMoveScale() const = 0;

    // Set to -1 to disable override.
    void SetOverrideDynamicScale( float scale ) { m_flOverrideDynamicScale = scale; }

protected:
    float m_flDynamicMove;
    float m_flOverrideDynamicScale;
};
//


//
class CZMDotCrosshair : public CZMBaseCrosshair
{
public:
    virtual void WriteValues( KeyValues* kv ) const OVERRIDE;

    virtual void Draw() OVERRIDE;
};
//


//
class CZMMaterialCrosshair : public CZMBaseCrosshair
{
public:
    CZMMaterialCrosshair();


    virtual void LoadValues( KeyValues* kv ) OVERRIDE;
    virtual void WriteValues( KeyValues* kv ) const OVERRIDE;

    virtual void Draw() OVERRIDE;


protected:
    int m_nTexMat0Id;
    char m_szTexture[192];
};
//


//
class CZMPistolCrosshair : public CZMBaseCrosshair
{
public:
    virtual void LoadValues( KeyValues* kv ) OVERRIDE;

    virtual void Draw() OVERRIDE;
};
//


//
class CZMAccuracyCrosshair : public CZMBaseDynamicCrosshair
{
public:
    virtual void LoadValues( KeyValues* kv ) OVERRIDE;
    virtual void WriteValues( KeyValues* kv ) const OVERRIDE;


    virtual void Draw() OVERRIDE;

    virtual float GetWidth() const;
    virtual float GetLength() const;

    virtual float GetDynamicMoveScale() const OVERRIDE;

protected:
    float m_flWidth;
    float m_flLength;
};
//


//
class CZMCrosshairSystem : private CAutoGameSystem
{
public:
    CZMCrosshairSystem();

    virtual bool Init() OVERRIDE;


    CUtlVector<CZMBaseCrosshair*>* GetCrosshairs() { return &m_vCrosshairs; }

    CZMBaseCrosshair* GetCrosshairByName( const char* name ) const;
    CZMBaseCrosshair* GetCrosshairByIndex( int index ) const;
    int FindCrosshairByName( const char* name ) const;

    void WriteCrosshairsToFile() const;


    static CZMBaseCrosshair* CreateCrosshairFromData( KeyValues* kv );

protected:
    void LoadFiles();
    void ReadCrosshairs( KeyValues* kv );
    int AddCrosshair( KeyValues* kv );

    CUtlVector<CZMBaseCrosshair*> m_vCrosshairs;
};

extern CZMCrosshairSystem g_ZMCrosshairs;

static CZMBaseCrosshair* ZMGetCrosshair( const char* name )
{
    static int iCrosshair = -1;
    if ( iCrosshair == -1 )
    {
        iCrosshair = g_ZMCrosshairs.FindCrosshairByName( name );

        if ( iCrosshair == -1 )
        {
            DevWarning( "Crosshair %s can't be found!\n", name );
        }
    }

    return g_ZMCrosshairs.GetCrosshairByIndex( iCrosshair );
}
//
