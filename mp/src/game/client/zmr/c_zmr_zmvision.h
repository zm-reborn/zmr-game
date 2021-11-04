#pragma once


class CZMVision
{
public:
    CZMVision();


    void RenderSilhouette();
    void UpdateLight();

    inline bool IsOn() { return m_bIsOn; };

    void TurnOff();
    void TurnOn();
    void Toggle();

    void AddSilhouette( C_BaseEntity* pEnt );
    void RemoveSilhouette( C_BaseEntity* pEnt );

    void Init();

protected:
    CUtlVector<C_BaseEntity*> m_Silhouettes;

    bool m_bIsOn;

    ClientShadowHandle_t m_FlashlightHandle;
    // Texture for flashlight
    CTextureReference m_FlashlightTexture;

private:
    void SetVision( bool bEnable );
    void SetVisionOff();

    void DestroyFlashlight();
};

extern CZMVision g_ZMVision;
