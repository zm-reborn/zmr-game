#pragma once


class CZMPlayer;

class CZMFlashlightEffect
{
public:
    CZMFlashlightEffect( CZMPlayer* pPlayer );
    virtual ~CZMFlashlightEffect();


    virtual void UpdateLight();
    void TurnOn();
    void TurnOff();


    float GetFlashlightFov() const;
    float GetFlashlightNearZ() const;
    float GetFlashlightFarZ() const;

    static bool UseThirdpersonEffects();

    bool ShouldUseProjected() const;
    bool IsInThirdperson() const;
    bool LocalPlayerWatchingMe() const;

    bool IsOn() const { return m_bIsOn; }
    ClientShadowHandle_t GetFlashlightHandle() const { return m_FlashlightHandle; }
    bool HasExpensiveOn() const { return GetFlashlightHandle() != CLIENTSHADOW_INVALID_HANDLE; }


    void SetFlashlightHandle( ClientShadowHandle_t handle );
    void SetRightHandAttachment( int index );
    void PreferExpensive( bool state );
    
protected:
    void LightOff();
    void LightOffCheap();
    void LightOffNew();
    void BeamOff();

    void UpdateLightNew( const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, bool bIsThirdperson );
    void UpdateLightCheap( const Vector& vecPos, const Vector& vecDir );

    void UpdateBeam( const Vector& vecDir );

    bool m_bIsOn;

    // The projected texture handle
    ClientShadowHandle_t m_FlashlightHandle;

    // The flashlight beam used for thirdperson stuff.
    int m_iAttachmentRH;
    Beam_t* m_pFlashlightBeam;

    // Texture for flashlight
    CTextureReference m_FlashlightTexture;


    bool m_bPreferExpensive;
    bool m_bDoFade;
    float m_flFadeAlpha;


    CZMPlayer* m_pPlayer;
};
