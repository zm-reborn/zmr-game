#pragma once

class CZMInput : public CInput
{
public:
    CZMInput();
    ~CZMInput();


    enum ScrollState_t
    {
        SCROLL_NONE = 0,

        SCROLL_POSITIVE,
        SCROLL_NEGATIVE
    };

    struct AxisState_t
    {
        ScrollState_t iState;
        float flLastStateChange;
    };

    virtual void AdjustYaw( float speed, QAngle& viewangles ) OVERRIDE;


    void SetScreenScrollStatePitch( ScrollState_t state );
    void SetScreenScrollStateYaw( ScrollState_t state );

protected:
    AxisState_t m_StateYaw;
    AxisState_t m_StatePitch;
};

extern CZMInput* ZMInput();
