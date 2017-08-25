#include "baseviewport.h"
#include "hudelement.h"


class CZMBoxSelect : public vgui::Panel

{
public:
    DECLARE_CLASS_SIMPLE( CZMBoxSelect, vgui::Panel );

    CZMBoxSelect( vgui::Panel* pParent );


    //virtual bool ShouldDraw() OVERRIDE;
    //virtual void Init() OVERRIDE;
    virtual void Paint() OVERRIDE;
    
    //inline void SetDraw( bool b ) { m_bDraw = b; };

    void SetStart( int, int );
    void SetEnd( int, int );

    inline void GetBox( int* x1, int* y1, int* x2, int* y2 )
    {
        *x1 = start_x;
        *y1 = start_y;
        *x2 = end_x;
        *y2 = end_y;
    }

    inline bool ShouldSelect()
    {
        return abs(end_x - start_x) > 2 && abs(end_y - start_y) > 2;
    }
private:
    //bool m_bDraw;
    
    int start_x, start_y;
    int end_x, end_y;
};
