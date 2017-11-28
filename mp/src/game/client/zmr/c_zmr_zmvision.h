#pragma once


class CZMVision
{
public:
    CZMVision() { m_Silhouettes.Purge(); };


    void RenderSilhouette();
    void UpdateLight();

    inline bool IsOn() { return m_bIsOn; };

    void TurnOff();
    void TurnOn();
    void Toggle();

    void AddSilhouette( C_BaseEntity* pEnt );
    void RemoveSilhouette( C_BaseEntity* pEnt );

protected:
    CUtlVector<C_BaseEntity*> m_Silhouettes;

    bool m_bIsOn;

private:
    void SetVision( bool bEnable );
    void SetVisionOff();
};

extern CZMVision g_ZMVision;
