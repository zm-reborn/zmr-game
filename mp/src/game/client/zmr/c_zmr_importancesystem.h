#pragma once


#include <vgui/IImage.h>

class C_ZMImportanceSystem
{
public:
    C_ZMImportanceSystem();
    ~C_ZMImportanceSystem();

    void InitImages();
    void ResetCached();

    vgui::IImage* GetPlayerImportanceImageIndex( int playerIndex );

protected:
    int ComputePlayerImportance( int playerIndex );
    vgui::IImage* ImportanceIndexToImage( int index );
    int ImportanceNameToIndex( const char* name );

private:
    vgui::IImage* m_pImageTrusted;
    vgui::IImage* m_pImagePlaytester;


    int m_iCachedImportance[MAX_PLAYERS];
};

extern C_ZMImportanceSystem g_ZMImportanceSystem;
