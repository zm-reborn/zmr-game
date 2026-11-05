#pragma once

#ifndef CLIENT_DLL
#include <tier1/UtlStringMap.h>

class CZMHandednessSystem : public CAutoGameSystem
{
public:
    CZMHandednessSystem();

    void PrecacheLeftHanded( const char* rightHandedViewModel );
    const char* GetLeftHandedModelName( const char* rightHandedViewModel ) const;
    int GetLeftHandedModelIndex( const char* rightHandedViewModel ) const;

    bool UsesLeftHanded( int playerEntIndex ) const;

    virtual void LevelShutdownPostEntity() override;
    virtual void LevelInitPreEntity() override;

private:
    void Clear( const char* reason );

    static void GetLeftHandedModelName( const char* rightHandedViewModel, char* dest, size_t destSizeInBytes );

    struct LeftHandedData_t
    {
        LeftHandedData_t()
        {
            lhName[0] = '\0';
            iModelIndex = -1;
        }

        char lhName[MAX_PATH];
        int iModelIndex;
    };

    CUtlStringMap<LeftHandedData_t> m_map;
};

CZMHandednessSystem& GetZMHandednessSystem();
#endif
