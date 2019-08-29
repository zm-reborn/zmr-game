#pragma once


class CZMListPanel;
class CZMListSection;
class CZMListRow;


enum
{
    COLUMN_ALIGN_CENTER = ( 1 << 0 ),
    COLUMN_ALIGN_RIGHT = ( 1 << 1 ),

    // Make the text-image align to the right.
    COLUMN_IMGALIGN_RIGHT = ( 1 << 2 ),

    // This is an image.
    COLUMN_IMG = ( 1 << 3 ),

    // Stretch this column to the first column aligned to the right.
    COLUMN_STRETCH_RIGHT = ( 1 << 4 ),

    COLUMN_CLICKABLE = ( 1 << 5 ),

    // Has a tooltip.
    COLUMN_TOOLTIP = ( 1 << 6 ),
};

// Fixes a small stupid leak.
class CZMImageList : public vgui::ImageList
{
public:
    CZMImageList() : vgui::ImageList( false ) {};
    ~CZMImageList() { if ( GetImage( 0 ) ) delete GetImage( 0 ); };
};

class CZMListRow : public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMListRow, vgui::Panel );

    CZMListRow( CZMListSection* pParentSection, int itemId, KeyValues* kv );
    ~CZMListRow();


    virtual void OnMousePressed( vgui::MouseCode code ) OVERRIDE;
    virtual void OnCursorMoved( int x, int y ) OVERRIDE;
    virtual void PerformLayout() OVERRIDE;


    int         GetItemId() const { return m_iItemId; };

    int         GetItemHeight() const { return m_nItemHeight; };
    void        SetItemHeight( int h );

    int         GetItemBottomMargin() const { return m_nItemBottomMargin; };
    void        SetItemBottomMargin( int margin );

    vgui::HFont GetItemFont() const { return m_hItemFont; };
    // Set to 0 to ignore this. Use section's default font by default.
    void        SetItemFont( vgui::HFont font );

    int         GetItemWidth() const { return m_nItemWidth; };
    // Set to -1 to ignore this. Uses column width by default.
    void        SetItemWidth( int w );

    Color       GetItemColor() const { return m_ItemColor; };
    void        SetItemColor( Color clr );

    //void        SetItemBgColor( Color clr );

    int         GetItemColumnCount() const;

    KeyValues*  GetData() const { return m_pKvData; };
    void        SetData( KeyValues* kv );

    CZMListSection* GetSection() const { return m_pParentSection; };
    void            SetSection( CZMListSection* pSection ) { m_pParentSection = pSection; };

protected:
    void    CreateNewChild( int index, int type = 0 );

    int     GetColumnWidth( int col );

    void    LayoutColumnData();
    void    ColumnDataLabel( int column, vgui::Label* pLabel );

    void    LayoutColumns();
    void    LayoutColumn( int i, int& x, int lastx );

    CZMListSection* m_pParentSection;
    KeyValues*      m_pKvData;
    int             m_iItemId;
    int             m_nItemHeight;
    int             m_nItemBottomMargin;
    int             m_nItemWidth;
    vgui::HFont     m_hItemFont;
    Color           m_ItemColor;

    CUtlVector<vgui::Panel*> m_vChildren;
    int m_iLastHovered;
};


typedef bool (*ZMSectionSortFunc_t)( KeyValues* kv1, KeyValues* kv2 );

class CZMListSection : public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMListSection, vgui::Panel );

    CZMListSection( CZMListPanel* parent, const char* name );
    ~CZMListSection();



    void    ReSortItems();
    void    SetNeedsSorting( bool state ) { m_bNeedsSorting = state; };

    int         AddColumn( const char* name, int width, int flags, int xoffset );

    void        ClearRows();
    // Doesn't free row.
    CZMListRow* EraseItem( int index );
    // Frees row.
    void        RemoveItem( int index );

    int         GetNumItems() const { return m_vItems.Count(); };
    CZMListRow* GetItem( int index ) const { return m_vItems[index]; };

    const char* GetColumnName( int col ) const;
    int         GetColumnWidth( int col ) const;
    int         GetColumnFlags( int col ) const;
    int         GetColumnOffset( int col ) const;
    int         GetNumColumns() const { return m_vColumns.Count(); };

    int             AddItem( int itemId, KeyValues* kv );
    int             AddItem( CZMListRow* pRow );


    int     GetDefaultItemHeight() const { return m_nDefaultItemHeight; };
    void    SetDefaultItemHeight( int h );

    void    SetSortingFunc( ZMSectionSortFunc_t func ) { m_pSortFunc = func; };

    int     GetBottomMargin() const { return m_nBottomMargin; };
    void    SetBottomMargin( int margin );
    int     GetTopMargin() const { return m_nTopMargin; };
    void    SetTopMargin( int margin );

    vgui::HFont GetDefaultFont() const { return m_hDefaultFont; };
    void        SetDefaultFont( vgui::HFont font );

    CZMListPanel*   GetListPanel() const { return m_pListPanel; };

    int     FindItemByKey( int iSymbolIndex, int iKeyData ) const;
    int     FindItemById( int itemId ) const;

    class CZMListColumn
    {
    public:
        CZMListColumn( const char* name, int width, int flags, int xoffset )
        {
            V_strcpy_safe( m_szName, name );
            m_nWidth = width;
            m_Flags = flags;
            m_nOffsetX = xoffset;
        }

        char    m_szName[64];
        int     m_nWidth;
        int     m_Flags;
        int     m_nOffsetX;
    };

protected:
    void    SortItems();

    CUtlVector<CZMListRow*>     m_vItems;
    CUtlVector<CZMListColumn*>  m_vColumns;

    int                 m_nTopMargin;
    int                 m_nBottomMargin;
    vgui::HFont         m_hDefaultFont;
    int                 m_nDefaultItemHeight;
    CZMListPanel*       m_pListPanel;
    ZMSectionSortFunc_t m_pSortFunc;
    bool                m_bNeedsSorting;
};

class CZMListPanel : public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMListPanel, vgui::Panel );
    
    CZMListPanel( vgui::Panel* parent, const char* name );
    ~CZMListPanel();

    virtual void    ApplySettings( KeyValues* inKv ) OVERRIDE;
    virtual void    PerformLayout() OVERRIDE;

    void    Clear();

    int     AddSection( const char* name, vgui::HFont defaultfont = NULL, int nDefItemHeight = 0 );

    int     GetSectionByName( const char* name ) const;
    void    ClearRows( int section );

    void    SetSectionSortingFunc( int section, ZMSectionSortFunc_t func );
    void    SetSectionTopMargin( int section, int margin );
    void    SetSectionBottomMargin( int section, int margin );
    void    SetSectionMouseInputEnabled( int section, bool state );

    int     AddColumn( int section, const char* name, int width = 0, int flags = 0, int xoffset = 0 );

    int     AddItem( int section, KeyValues* kv );
    void    SetItemWidth( int section, int itemIndex, int w );
    void    SetItemHeight( int section, int itemIndex, int h );
    void    SetItemFont( int section, int itemIndex, vgui::HFont font );
    void    SetItemColor( int section, int itemIndex, Color clr );
    void    SetItemBgColor( int section, int itemIndex, Color clr );
    
    int     ModifyItem( int itemId, int newSection, KeyValues* kv );
    void    RemoveItem( int section, int itemIndex );
    void    RemoveItemById( int itemId );

    int                 AddImage( vgui::IImage* pImage );
    CZMImageList*       GetImageList() const { return m_pImageList; };

    int     FindItemByKey( int iSymbolIndex, int iKeyData ) const;

protected:
    CUtlVector<CZMListSection*>     m_vSections;
    CZMImageList*                   m_pImageList;


    int m_nItemIds;
    int m_nFamilyIds;
};
