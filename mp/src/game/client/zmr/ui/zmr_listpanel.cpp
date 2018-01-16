#include "cbase.h"

#include <vgui_controls/TextImage.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ImageList.h>

#include "zmr_listpanel.h"


using namespace vgui;

#define ITEM_DEFAULT_BOTTOMMARGIN   1
#define ITEM_DEFAULT_HEIGHT         scheme()->GetProportionalScaledValueEx( GetScheme(), 20 )

/*
    Item
*/
CZMListRow::CZMListRow( CZMListSection* parent, int itemId, KeyValues* kv ) : vgui::Label( parent, nullptr, "<unset constructor>" )
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
}

CZMListRow::~CZMListRow()
{
#ifdef _DEBUG
    DevMsg( "~CZMListRow()\n" );
#endif

    if ( m_pKvData )
    {
        m_pKvData->deleteThis();
        m_pKvData = nullptr;
    }
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

int CZMListRow::GetItemColumnCount()
{
    return ( GetSection()->GetNumColumns() > 0 ) ? GetSection()->GetNumColumns() : 1;
}

void CZMListRow::PerformLayout()
{
    if ( !m_pKvData )
    {
        Assert( 0 );

        SetText( "<no kv>" );

        BaseClass::PerformLayout();
        return;
    }

    if ( GetItemColumnCount() < 1 )
    {
        Assert( 0 );

        SetText( "<no cols>" );

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


    Color clr = GetItemColor();
    if ( clr[3] < 1 )
        clr = GetFgColor();


    HFont usefont = GetFont();
    if ( GetItemFont() > NULL )
        usefont = GetItemFont();
    else if ( GetSection()->GetDefaultFont() > NULL )
        usefont = GetSection()->GetDefaultFont();


    SetFgColor( clr );
    SetFont( usefont );

    for ( int i = 0; i < nCols; i++ )
    {
        TextImage* pTextImage = nullptr;
        IImage* pImage = GetImageAtIndex( i );

        const bool bIsImg = ( GetSection()->GetColumnFlags( i ) & COLUMN_IMG ) != 0;
        
        const char* colName = GetSection()->GetColumnName( i );


        pTextImage = ( pImage || bIsImg ) ? dynamic_cast<TextImage*>( pImage ) : (new TextImage( "" ));

        if ( bIsImg )
        {
            // First image is always a text image. Remove it if we want an image there.
            if ( pTextImage )
            {
                delete pTextImage;
                pTextImage = nullptr;
            }

            pImage = nullptr;


            // Find the image and set it.
            int index = m_pKvData->GetInt( colName, -1 );

            ImageList* list = GetSection()->GetListPanel()->GetImageList();
            if ( list && list->IsValidIndex( index ) )
            {
                pImage = list->GetImage( index );
                SetImageAtIndex( i, pImage, 0 );
            }
        }

        if ( pTextImage )
        {
            pTextImage->SetText( m_pKvData->GetString( colName, "" ) );
            pTextImage->SetColor( clr );
            pTextImage->SetFont( usefont );

            pImage = pTextImage;
        }


        SetImageAtIndex( i, pImage, 0 );
    }
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


    IImage* pImage = GetImageAtIndex( i );
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
    
    SetImageBounds( i, xpos + offset, wide );
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

const char* CZMListSection::GetColumnName( int col )
{
    if ( !m_vColumns.IsValidIndex( col ) )
        return "";

    return m_vColumns[col]->m_szName;
}

int CZMListSection::GetColumnWidth( int col )
{
    if ( !m_vColumns.IsValidIndex( col ) )
        return GetWide();

    return m_vColumns[col]->m_nWidth;
}

int CZMListSection::GetColumnFlags( int col )
{
    if ( !m_vColumns.IsValidIndex( col ) )
        return 0;

    return m_vColumns[col]->m_Flags;
}

int CZMListSection::GetColumnOffset( int col )
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

int CZMListSection::FindItemByKey( int iSymbolIndex, int iKeyData )
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

int CZMListSection::FindItemById( int itemId )
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

        for ( int i = 0; i < nItems; i++ )
        {
            auto* item = section->GetItem( i );

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

int CZMListPanel::GetSectionByName( const char* name )
{
    for ( int i = 0; i < m_vSections.Count(); i++ )
    {
        if ( Q_strcmp( name, m_vSections[i]->GetName() ) == 0 )
            return i;
    }

    return -1;
}

void CZMListPanel::ClearRows( int section )
{
    m_vSections[section]->ClearRows();
}

void CZMListPanel::SetSectionSortingFunc( int section, ZMSectionSortFunc_t func )
{
    m_vSections[section]->SetSortingFunc( func );
}

void CZMListPanel::SetSectionTopMargin( int section, int margin )
{
    m_vSections[section]->SetTopMargin( margin );
}

void CZMListPanel::SetSectionBottomMargin( int section, int margin )
{
    m_vSections[section]->SetBottomMargin( margin );
}

int CZMListPanel::AddColumn( int section, const char* name, int width, int flags, int xoffset )
{
    return m_vSections[section]->AddColumn( name, width, flags, xoffset );
}

int CZMListPanel::AddItem( int section, KeyValues* kv )
{
    int item = m_vSections[section]->AddItem( m_nItemIds++, kv );
    
    InvalidateLayout();

    return item;
}

void CZMListPanel::SetItemWidth( int section, int itemIndex, int w )
{
    m_vSections[section]->GetItem( itemIndex )->SetItemWidth( w );
}

void CZMListPanel::SetItemHeight( int section, int itemIndex, int h )
{
    m_vSections[section]->GetItem( itemIndex )->SetItemHeight( h );
}

void CZMListPanel::SetItemFont( int section, int itemIndex, HFont font )
{
    m_vSections[section]->GetItem( itemIndex )->SetItemFont( font );
}

void CZMListPanel::SetItemColor( int section, int itemIndex, Color clr )
{
    m_vSections[section]->GetItem( itemIndex )->SetItemColor( clr );
}

void CZMListPanel::SetItemBgColor( int section, int itemIndex, Color clr )
{
    m_vSections[section]->GetItem( itemIndex )->SetBgColor( clr );
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

int CZMListPanel::FindItemByKey( int iSymbolIndex, int iKeyData )
{
    for ( int i = 0; i < m_vSections.Count(); i++ )
    {
        int index = m_vSections[i]->FindItemByKey( iSymbolIndex, iKeyData );
        if ( index != -1 )
            return m_vSections[i]->GetItem( index )->GetItemId();
    }

    return -1;
}
