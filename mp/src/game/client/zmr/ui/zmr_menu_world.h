#pragma once


#include "cdll_util.h"
#include "baseviewport.h"
#include "iclientmode.h"
#include <vgui_controls/Frame.h>

#include "zmr/zmr_util.h"


abstract_class CZMWorldMenu
{
public:
    bool GetScreenPos( int& out_x, int& out_y )
    {
        Vector screen;
        int x, y;

        if ( ZMClientUtil::WorldToScreen( m_vecWorldPos, screen, x, y ) )
        {
            x -= m_nOffsetCenterX;
            y -= m_nOffsetCenterY;


            if ( (x + m_nLimitMinX) < 0 ) x = -m_nLimitMinX;
            if ( (y + m_nLimitMinY) < 0 ) y = -m_nLimitMinY;
            if ( (x+m_nLimitMaxX) >= ScreenWidth() ) x = ScreenWidth() - m_nLimitMaxX;
            if ( (y+m_nLimitMaxY) >= ScreenHeight() ) y = ScreenHeight() - m_nLimitMaxY;

            out_x = x;
            out_y = y;

            return true;
        }

        return false;
    }


protected:
    inline void SetWorldPos( Vector pos ) { m_vecWorldPos = pos; };
    inline void SetOffset( int x, int y ) { m_nOffsetCenterX = x; m_nOffsetCenterY = y; };

    inline void SetLimits( int minx, int miny, int maxx, int maxy )
    {
        m_nLimitMinX = minx;
        m_nLimitMinY = miny;
        m_nLimitMaxX = maxx;
        m_nLimitMaxY = maxy;
    }

private:
    Vector m_vecWorldPos;
    int m_nOffsetCenterX;
    int m_nOffsetCenterY;

    int m_nLimitMinX;
    int m_nLimitMinY;
    int m_nLimitMaxX;
    int m_nLimitMaxY;
};
