#include "cbase.h"

#include <vgui_controls/TextImage.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Tooltip.h>

#include "zmr_listpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;

#define ITEM_DEFAULT_BOTTOMMARGIN   1
#define ITEM_DEFAULT_HEIGHT         scheme()->GetProportionalScaledValueEx( GetScheme(), 20 )

/*
    Item
*/
CZMListRow::CZMListRow( CZMListSection* parent, int itemId, KeyValues* kv ) : vgui::Panel( parent, "" )
{
    m_pParentSection = parent;

    m_pKvData = kv;
    
    m_iItemId = itemId;
    m_nItemHeight = parent->GetDefaultItemHeight();
    m_nItemBottomMargin = ITEM_DEFAULT_BOTTOMMARGIN;
    m_nItemWidth = -1;
    m_hItemFont = NULL;
    m_ItemColor = COLOR_WHITE;


    SetPaintBackgroundEnabled( false );
    SetMouseInputEnabled( true );
}

CZMListRow::~CZMListRow()
{
    if ( m_pKvData )
    {
        m_pKvData->deleteThis();
        m_pKvData = nullptr;
    }

    m_vChildren.PurgeAndDeleteElements();
}

void CZMListRow::SetData( KeyValues* kv )
{
    if ( m_pKvData )
    {
        m_pKvData->deleteThis();
    }

    m_pKvData = kv;

    GetSection()->SetNeedsSorting( true );
    GetSection()->GetListPanel()->InvalidateLayout();
}

void CZMListRow::SetItemHeight( int h )
{
    m_nItemHeight = h;
    GetSection()->GetListPanel()->InvalidateLayout();
}

void CZMListRow::SetItemBottomMargin( int margin )
{
    m_nItemBottomMargin = margin;
    GetSection()->GetListPanel()->InvalidateLayout();
}

void CZMListRow::SetItemWidth( int w )
{
    m_nItemWidth = w;
    InvalidateLayout();
}

void CZMListRow::SetItemFont( HFont font )
{
    m_hItemFont = font;
    InvalidateLayout();
}

void CZMListRow::SetItemColor( Color clr )
{
    m_ItemColor = clr;
    InvalidateLayout();
}

//void CZMListRow::SetItemBgColor( Color clr )
//{
//    SetBgColor( clr );
//}

int CZMListRow::GetItemColumnCount() const
{
    return ( GetSection()->GetNumColumns() > 0 ) ? GetSection()->GetNumColumns() : 1;
}

void CZMListRow::OnMousePressed( vgui::MouseCode code )
{
    if ( m_iLastHovered == -1 )
        return;

#ifdef _DEBUG
    DevMsg( "On Row Item Pressed\n" );
#endif


    KeyValues* kv = nullptr;
    if ( GetData() )
    {
        kv = GetData()->MakeCopy();
        kv->SetName( "OnRowItemPressed" );
    }
    else
    {
        kv = new KeyValues( "OnRowItemPressed" );
    }


    kv->SetString( "pressed_name", GetSection()->GetColumnName( m_iLastHovered ) );
    kv->SetInt( "pressed_index", m_iLastHovered );


    Panel* pParent = GetSection()->GetParent()->GetParent();
    PostMessage( pParent, kv );
}

void CZMListRow::OnCursorMoved( int x, int y )
{
    int nCols = GetItemColumnCount();

    for ( int i = 0; i < nCols; i++ )
    {
        if ( GetSection()->GetColumnFlags( i ) & COLUMN_CLICKABLE )
        {
            Panel* pChild = m_vChildren[i];

            if ( pChild->IsCursorOver() )
            {
                SetCursor( dc_hand );
                m_iLastHovered = i;

                return;
            }
        }
    }

    SetCursor( dc_arrow );
    m_iLastHovered = -1;
}

void CZMListRow::PerformLayout()
{
    if ( !GetData() )
    {
        Assert( 0 );

        BaseClass::PerformLayout();
        return;
    }

    if ( GetItemColumnCount() < 1 )
    {
        Assert( 0 );

        BaseClass::PerformLayout();
        return;
    }


    LayoutColumnData();

    LayoutColumns();

    BaseClass::PerformLayout();
}

void CZMListRow::LayoutColumnData()
{
    int nCols = GetItemColumnCount();

    for ( int i = 0; i < nCols; i++ )
    {
        if ( i >= m_vChildren.Count() )
        {
            CreateNewChild( i );
        }

        Panel* pChild = m_vChildren[i];
        pChild->SetMouseInputEnabled( false );

        Label* pLabel = dynamic_cast<Label*>( pChild );

        if ( pLabel )
        {
            ColumnDataLabel( i, pLabel );
        }
    }
}

void CZMListRow::ColumnDataLabel( int column, Label* pLabel )
{
    Color clr = GetItemColor();
    if ( clr[3] < 1 )
        clr = GetFgColor();

    HFont usefont = GetSection()->GetDefaultFont();
    if ( GetItemFont() > NULL )
        usefont = GetItemFont();



    if ( usefont > NULL )
        pLabel->SetFont( usefont );

    pLabel->SetFgColor( clr );


    char temp[64];

    TextImage* pTextImage = nullptr;
    IImage* pImage = pLabel->GetImageAtIndex( 0 );

    const bool bIsImg = ( GetSection()->GetColumnFlags( column ) & COLUMN_IMG ) != 0;

    const bool bHasTooltip = ( GetSection()->GetColumnFlags( column ) & COLUMN_TOOLTIP ) != 0;
        
    const char* colName = GetSection()->GetColumnName( column );


    pTextImage = ( pImage || bIsImg ) ? dynamic_cast<TextImage*>( pImage ) : (new TextImage( "" ));

    if ( bIsImg )
    {
        // First image is always a text image. Remove it if we want an image there.
        if ( pTextImage )
        {
            //delete pTextImage; // Can't delete it because label destructor will do it for us.
            pLabel->SetImageAtIndex( 0, nullptr, 0 );

            pTextImage = nullptr;
        }

        pImage = nullptr;


        // Find the image and set it.
        int index = GetData()->GetInt( colName, -1 );

        ImageList* list = GetSection()->GetListPanel()->GetImageList();
        if ( list && list->IsValidIndex( index ) )
        {
            pImage = list->GetImage( index );
            pLabel->SetImageAtIndex( 0, pImage, 0 );
        }
    }

    if ( pTextImage )
    {
        pTextImage->SetText( GetData()->GetString( colName, "" ) );
        pTextImage->SetColor( clr );
        pTextImage->SetFont( usefont );

        pImage = pTextImage;
    }

    if ( bHasTooltip )
    {
        Q_snprintf( temp, sizeof( temp ), "%s_tooltip", colName );

        const char* tip = GetData()->GetString( temp, nullptr );

        if ( tip && *tip )
        {
            pLabel->GetTooltip()->SetText( tip );
            pLabel->SetMouseInputEnabled( true );
        }
        else
        {
            pLabel->SetMouseInputEnabled( false );
            //pLabel->SetTooltip( nullptr, nullptr );
        }
    }

    pLabel->SetImageAtIndex( 0, pImage, 0 );
}

void CZMListRow::LayoutColumns()
{
    int nCols = GetItemColumnCount();

    int x = GetWide();
    int lastx = 0;

    // Layout the columns that are on the right first.
    for ( int i = nCols - 1; i >= 0; i-- )
    {
        if ( GetSection()->GetColumnFlags( i ) & COLUMN_IMGALIGN_RIGHT )
        {
            LayoutColumn( i, x, lastx );
        }
        else break;
    }

    // Layout the rest of the columns normally.
    lastx = x;
    x = 0;
    for ( int i = 0; i < nCols; i++ )
    {
        if ( (GetSection()->GetColumnFlags( i ) & COLUMN_IMGALIGN_RIGHT) == 0 )
        {
            LayoutColumn( i, x, lastx );
        }
        else break;
    }
}

void CZMListRow::LayoutColumn( int i, int& x, int lastx )
{
    int xpos = x;
    Panel* pChild = m_vChildren[i];
    Label* pLabel = dynamic_cast<Label*>( pChild );


    IImage* pImage = nullptr;
    if ( pLabel )
        pImage = pLabel->GetImageAtIndex( 0 );

    TextImage* pTextImage = dynamic_cast<TextImage*>( pImage );
    
    int colWidth = GetColumnWidth( i );
    int colOffset = GetSection()->GetColumnOffset( i );
    int flags = GetSection()->GetColumnFlags( i );


    if ( flags & COLUMN_STRETCH_RIGHT )
        colWidth = lastx - xpos;

    if ( colWidth < 1 )
        colWidth = GetWide();


    int cw, ch;

    if ( pImage )
    {
        pImage->GetContentSize( cw, ch );
    }
    else
    {
        cw = ch = 0;
    }

    int offset = 0;
    int wide = GetItemWidth() > 0 ? GetItemWidth() : colWidth;

    if ( flags & COLUMN_ALIGN_CENTER )
    {
        offset = wide / 2 - cw / 2;
    }
    else if ( flags & COLUMN_ALIGN_RIGHT )
    {
        offset = wide - cw;
    }

    offset += colOffset;

    if ( flags & COLUMN_IMGALIGN_RIGHT )
    {
        xpos -= wide;

        x -= wide;
    }
    else
    {
        x += wide;
    }


    // Text content has to be resized here.
    if ( pTextImage ) 
    {
        pTextImage->ResizeImageToContentMaxWidth( wide );
    }
    

    pChild->SetBounds( xpos, 0, wide, GetTall() );

    if ( pLabel )
        pLabel->SetImageBounds( 0, offset, wide );
}

void CZMListRow::CreateNewChild( int index, int type )
{
    if ( m_vChildren.IsValidIndex( index ) || index < 0 )
        return;


    while ( !m_vChildren.IsValidIndex( index ) )
    {
        m_vChildren.AddToTail( nullptr );
    }

    Panel* pChild = nullptr;

    // Add shit here if need be.
    switch ( type )
    {
    case 0 :
    default :
        pChild = new Label( this, "", L"" );
    }

    

    m_vChildren[index] = pChild;
}

int CZMListRow::GetColumnWidth( int col )
{
    return ( GetItemColumnCount() <= 1 ) ? GetWide() : GetSection()->GetColumnWidth( col );
}


/*
    Section
*/
CZMListSection::CZMListSection( CZMListPanel* parent, const char* name ) : BaseClass( parent, name )
{
    m_vItems.Purge();
    m_vColumns.Purge();

    m_nTopMargin = 0;
    m_nBottomMargin = 0;
    m_hDefaultFont = NULL;
    m_nDefaultItemHeight = ITEM_DEFAULT_HEIGHT;

    m_pSortFunc = nullptr;
    m_bNeedsSorting = false;

    m_pListPanel = parent;


    SetPaintBackgroundEnabled( false );
    //SetVisible( false );
}

CZMListSection::~CZMListSection()
{
    m_vItems.PurgeAndDeleteElements();
    m_vColumns.PurgeAndDeleteElements();
}

void CZMListSection::ReSortItems()
{
    if ( m_bNeedsSorting )
    {
        SortItems();
        m_bNeedsSorting = false;
    }
}

void CZMListSection::SortItems()
{
    if ( !m_pSortFunc ) return;


    CUtlVector<CZMListRow*> sorted;
    sorted.Purge();

    for ( int i = 0; i < m_vItems.Count(); i++ )
    {
        KeyValues* kv1 = m_vItems[i]->GetData();

        int j = 0;
        int numSorted = sorted.Count();
        for (; j < numSorted; j++ )
        {
            KeyValues* kv2 = sorted[j]->GetData();

            if ( m_pSortFunc( kv1, kv2 ) )
                break;
        }

        if ( j == numSorted )
        {
            sorted.AddToTail( m_vItems[i] );
        }
        else
        {
            sorted.InsertBefore( j, m_vItems[i] );
        }
    }

    m_vItems.Purge();
    m_vItems.CopyArray( sorted.Base(), sorted.Count() );
}

int CZMListSection::AddColumn( const char* name, int width, int flags, int xoffset )
{
    return m_vColumns.AddToTail( new CZMListColumn( name, width, flags, xoffset ) );
}

void CZMListSection::ClearRows()
{
    m_vItems.PurgeAndDeleteElements();
}

CZMListRow* CZMListSection::EraseItem( int index )
{
    Assert( m_vItems.IsValidIndex( index ) );

    CZMListRow* pRow = m_vItems[index];

    m_vItems.Remove( index );

    InvalidateLayout();

    return pRow;
}

void CZMListSection::RemoveItem( int index )
{
    DevMsg( "Removing item %i!\n", index );

    CZMListRow* pRow = m_vItems[index];

    m_vItems.Remove( index );

    delete pRow;

    GetListPanel()->InvalidateLayout();
}

int CZMListSection::AddItem( int itemId, KeyValues* kv )
{
    auto* row = SETUP_PANEL( new CZMListRow( this, itemId, kv ) );

    SetNeedsSorting( true );

    return m_vItems.AddToTail( row );
}

int CZMListSection::AddItem( CZMListRow* pRow )
{
    pRow->SetSection( this );
    pRow->SetParent( this );

    SetNeedsSorting( true );

    return m_vItems.AddToTail( pRow );
}

void CZMListSection::SetDefaultItemHeight( int h )
{
    if ( h < 1 )
        h = ITEM_DEFAULT_HEIGHT;

    m_nDefaultItemHeight = h;
}

void CZMListSection::SetDefaultFont( HFont font )
{
    m_hDefaultFont = font;
}

const char* CZMListSection::GetColumnName( int col ) const
{
    if ( !m_vColumns.IsValidIndex( col ) )
        return "";

    return m_vColumns[col]->m_szName;
}

int CZMListSection::GetColumnWidth( int col ) const
{
    if ( !m_vColumns.IsValidIndex( col ) )
        return const_cast<CZMListSection*>( this )->GetWide();

    return m_vColumns[col]->m_nWidth;
}

int CZMListSection::GetColumnFlags( int col ) const
{
    if ( !m_vColumns.IsValidIndex( col ) )
        return 0;

    return m_vColumns[col]->m_Flags;
}

int CZMListSection::GetColumnOffset( int col ) const
{
    if ( !m_vColumns.IsValidIndex( col ) )
        return 0;

    return m_vColumns[col]->m_nOffsetX;
}

void CZMListSection::SetTopMargin( int margin )
{
    m_nTopMargin = margin;
    GetListPanel()->InvalidateLayout();
}

void CZMListSection::SetBottomMargin( int margin )
{
    m_nBottomMargin = margin;
    GetListPanel()->InvalidateLayout();
}

int CZMListSection::FindItemByKey( int iSymbolIndex, int iKeyData ) const
{
    int len = GetNumItems();
    for ( int i = 0; i < len; i++ )
    {
        if ( !m_vItems[i]->GetData() )
            continue;

        KeyValues* kv = m_vItems[i]->GetData()->FindKey( iSymbolIndex );

        if ( kv && kv->GetInt() == iKeyData )
            return i;
    }

    return -1;
}

int CZMListSection::FindItemById( int itemId ) const
{
    int len = GetNumItems();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_vItems[i]->GetItemId() == itemId )
            return i;
    }

    return -1;
}



/*
    Panel
*/
DECLARE_BUILD_FACTORY( CZMListPanel );

CZMListPanel::CZMListPanel( Panel* parent, const char* name ) : BaseClass( parent, name )
{
    m_nItemIds = 0;
    m_vSections.Purge();
    m_pImageList = new CZMImageList();


    SetMouseInputEnabled( false );
    SetPaintBackgroundEnabled( false );

    // Has to be set to load fonts correctly.
    SetScheme( vgui::scheme()->LoadSchemeFromFile( "resource/ClientScheme.res", "ClientScheme" ) );
}

CZMListPanel::~CZMListPanel()
{
    Clear();
}

void CZMListPanel::ApplySettings( KeyValues* inKv )
{
    BaseClass::ApplySettings( inKv );


    KeyValues* data = inKv->FindKey( "ListData" );

    if ( !data )
    {
        Warning( "Couldn't read List Panel keyvalue data!\n" );
        return;
    }
    

    KeyValues* section = data->GetFirstSubKey();

    if ( !section )
    {
        Warning( "Couldn't read List Panel section keyvalue data!\n" );
        return;
    }


    do
    {
        const char* szSectionName = section->GetName();
        const char* szDefaultFont = section->GetString( "default_font", "Default" );

        HFont font = scheme()->GetIScheme( GetScheme() )->GetFont( szDefaultFont, IsProportional() );


        int iSection = AddSection(
            szSectionName,
            font,
            scheme()->GetProportionalScaledValueEx( GetScheme(), section->GetInt( "default_itemheight" ) ) );
        SetSectionTopMargin( iSection, scheme()->GetProportionalScaledValueEx( GetScheme(), section->GetInt( "topmargin", 0 ) ) );
        SetSectionBottomMargin( iSection, scheme()->GetProportionalScaledValueEx( GetScheme(), section->GetInt( "bottommargin", 0 ) ) );


        // Create columns.
        KeyValues* cols = section->FindKey( "Columns" );

        if ( !cols )
        {
            Warning( "Couldn't read columns in %s!\n", szSectionName );
            continue;
        }

        KeyValues* col = cols->GetFirstSubKey();

        while ( col )
        {
            int flags = 0;
            
            flags |= ( col->GetInt( "align_center" ) ? COLUMN_ALIGN_CENTER : 0 );
            flags |= ( col->GetInt( "align_right" ) ? COLUMN_ALIGN_RIGHT : 0 );
            flags |= ( col->GetInt( "colalign_right" ) ? COLUMN_IMGALIGN_RIGHT : 0 );
            flags |= ( col->GetInt( "is_image" ) ? COLUMN_IMG : 0 );
            flags |=  ( col->GetInt( "stretch_right" ) ? COLUMN_STRETCH_RIGHT : 0 );
            flags |= ( col->GetInt( "clickable" ) ? COLUMN_CLICKABLE : 0 );
            flags |= ( col->GetInt( "is_tooltip" ) ? COLUMN_TOOLTIP : 0 );

            AddColumn(
                iSection,
                col->GetName(),
                scheme()->GetProportionalScaledValueEx( GetScheme(), col->GetInt( "width" ) ),
                flags,
                scheme()->GetProportionalScaledValueEx( GetScheme(), col->GetInt( "xoffset" ) ) );

            col = col->GetNextKey();
        }


        // Get items.
        KeyValues* items = section->FindKey( "Items" );

        if ( !items )
        {
            continue;
        }


        KeyValues* item = items->GetFirstSubKey();

        while ( item )
        {
            AddItem( iSection, item->MakeCopy() );

            item = item->GetNextKey();
        }
    }
    while ( (section = section->GetNextKey()) != nullptr );

    DevMsg( "Parsed List Panel data.\n" );
}

void CZMListPanel::PerformLayout()
{
    BaseClass::PerformLayout();


    int wide = GetWide();
    int absy = 0;
    int nSections = m_vSections.Count();

    for ( int i = 0; i < nSections; i++ )
    {
        auto* section = m_vSections[i];


        section->ReSortItems();

        //if ( !section->IsVisible() )
        //    continue;

        int nItems = section->GetNumItems();
        

        
        absy += section->GetTopMargin();
        
        // Layout the header items.

        // Section relative
        int sy = 0;
        int sh = 0;
        int systart = absy;

        for ( int j = 0; j < nItems; j++ )
        {
            auto* item = section->GetItem( j );

            int height = item->GetItemHeight();

            item->SetBounds( 0, sy, wide, height );
            item->InvalidateLayout();

            height += item->GetItemBottomMargin();

            sy += height; sh += height;
        }

        section->SetBounds( 0, systart, wide, sh );


        absy += sy + section->GetBottomMargin();
    }

    SetSize( wide, absy );


    PostMessage( GetParent(), new KeyValues( "OnListLayout", "name", GetName() ) );
}

void CZMListPanel::Clear()
{
    m_vSections.PurgeAndDeleteElements();
    m_nItemIds = 0;

    delete m_pImageList;
    m_pImageList = new CZMImageList();
}

int CZMListPanel::AddSection( const char* name, vgui::HFont defaultfont, int nDefItemHeight )
{
    auto* section = SETUP_PANEL( new CZMListSection( this, name ) );

    section->SetDefaultFont( defaultfont );

    if ( nDefItemHeight > 0 )
        section->SetDefaultItemHeight( nDefItemHeight );

    return m_vSections.AddToTail( section );
}

int CZMListPanel::GetSectionByName( const char* name ) const
{
    for ( int i = 0; i < m_vSections.Count(); i++ )
    {
        if ( Q_strcmp( name, const_cast<CZMListSection*>( m_vSections[i] )->GetName() ) == 0 )
            return i;
    }

    return -1;
}

void CZMListPanel::ClearRows( int section )
{
    Assert( m_vSections.IsValidIndex( section ) );

    m_vSections[section]->ClearRows();
}

void CZMListPanel::SetSectionSortingFunc( int section, ZMSectionSortFunc_t func )
{
    Assert( m_vSections.IsValidIndex( section ) );

    m_vSections[section]->SetSortingFunc( func );
}

void CZMListPanel::SetSectionTopMargin( int section, int margin )
{
    Assert( m_vSections.IsValidIndex( section ) );

    m_vSections[section]->SetTopMargin( margin );
}

void CZMListPanel::SetSectionBottomMargin( int section, int margin )
{
    Assert( m_vSections.IsValidIndex( section ) );

    m_vSections[section]->SetBottomMargin( margin );
}

void CZMListPanel::SetSectionMouseInputEnabled( int section, bool state )
{
    Assert( m_vSections.IsValidIndex( section ) );

    auto* pSection = m_vSections[section];
    pSection->SetMouseInputEnabled( state );
    /*
    int nItems = pSection->GetNumItems();
    for ( int i = 0; i < nItems; i++ )
    {
        pSection->GetItem( i )->SetMouseInputEnabled( state );
    }
    */
}

int CZMListPanel::AddColumn( int section, const char* name, int width, int flags, int xoffset )
{
    Assert( m_vSections.IsValidIndex( section ) );

    return m_vSections[section]->AddColumn( name, width, flags, xoffset );
}

int CZMListPanel::AddItem( int section, KeyValues* kv )
{
    Assert( m_vSections.IsValidIndex( section ) );

    int item = m_vSections[section]->AddItem( m_nItemIds++, kv );
    
    InvalidateLayout();

    return item;
}

void CZMListPanel::SetItemWidth( int section, int itemIndex, int w )
{
    Assert( m_vSections.IsValidIndex( section ) );

    auto* pItem = m_vSections[section]->GetItem( itemIndex );
    Assert( pItem );

    pItem->SetItemWidth( w );
}

void CZMListPanel::SetItemHeight( int section, int itemIndex, int h )
{
    Assert( m_vSections.IsValidIndex( section ) );

    auto* pItem = m_vSections[section]->GetItem( itemIndex );
    Assert( pItem );

    pItem->SetItemHeight( h );
}

void CZMListPanel::SetItemFont( int section, int itemIndex, HFont font )
{
    Assert( m_vSections.IsValidIndex( section ) );

    auto* pItem = m_vSections[section]->GetItem( itemIndex );
    Assert( pItem );

    pItem->SetItemFont( font );
}

void CZMListPanel::SetItemColor( int section, int itemIndex, Color clr )
{
    Assert( m_vSections.IsValidIndex( section ) );

    auto* pItem = m_vSections[section]->GetItem( itemIndex );
    Assert( pItem );

    pItem->SetItemColor( clr );
}

void CZMListPanel::SetItemBgColor( int section, int itemIndex, Color clr )
{
    Assert( m_vSections.IsValidIndex( section ) );

    auto* pItem = m_vSections[section]->GetItem( itemIndex );
    Assert( pItem );

    pItem->SetBgColor( clr );
}

int CZMListPanel::ModifyItem( int itemId, int newSection, KeyValues* kv )
{
    int ret = -1;

    for ( int i = 0; i < m_vSections.Count(); i++ )
    {
        int item = m_vSections[i]->FindItemById( itemId );
        if ( item != -1 )
        {
            if ( i == newSection )
            {
                m_vSections[i]->GetItem( item )->SetData( kv );

                ret = item;
            }
            else
            {
                CZMListRow* pRow = m_vSections[i]->GetItem( item );
                
                int id = pRow->GetItemId();
                KeyValues* kv = pRow->GetData()->MakeCopy();

                m_vSections[i]->RemoveItem( item );

                ret = m_vSections[newSection]->AddItem( id, kv );
            }

            break;
        }
    }

    InvalidateLayout();

    return ret;
}

void CZMListPanel::RemoveItem( int section, int itemIndex )
{
    Assert( m_vSections.IsValidIndex( section ) );

    m_vSections[section]->RemoveItem( itemIndex );

    InvalidateLayout();
}

void CZMListPanel::RemoveItemById( int itemId )
{
    for ( int i = 0; i < m_vSections.Count(); i++ )
    {
        int item = m_vSections[i]->FindItemById( itemId );

        if ( item != -1 )
        {
            RemoveItem( i, item );
            break;
        }
    }

    InvalidateLayout();
}

int CZMListPanel::AddImage( IImage* pImage )
{
    if ( !m_pImageList )
    {
        return 0;
    }

    // Check if we already have this image.
    int len = m_pImageList->GetImageCount();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_pImageList->GetImage( i ) == pImage )
            return i;
    }

    return m_pImageList->AddImage( pImage );
}

int CZMListPanel::FindItemByKey( int iSymbolIndex, int iKeyData ) const
{
    for ( int i = 0; i < m_vSections.Count(); i++ )
    {
        int index = m_vSections[i]->FindItemByKey( iSymbolIndex, iKeyData );
        if ( index != -1 )
            return m_vSections[i]->GetItem( index )->GetItemId();
    }

    return -1;
}
