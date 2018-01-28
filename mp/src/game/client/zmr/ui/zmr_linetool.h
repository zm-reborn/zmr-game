#pragma once

class CZMLineTool : public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMLineTool, vgui::Panel );

    CZMLineTool( vgui::Panel* pParent );


    virtual void Paint() OVERRIDE;

    void SetStart( int x, int y );
    void SetEnd( int x, int y );

    inline void GetLine( int& x1, int& y1, int& x2, int& y2 )
    {
        x1 = start_x;
        y1 = start_y;
        x2 = end_x;
        y2 = end_y;
    }

    bool IsValidLine( int grace = 0 );

private:
    int start_x, start_y;
    int end_x, end_y;
};
