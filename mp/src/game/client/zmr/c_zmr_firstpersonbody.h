#pragma once

#include "c_baseanimatingoverlay.h"


class C_ZMFirstPersonBody : public C_BaseAnimatingOverlay
{
public:
    DECLARE_CLASS( C_ZMFirstPersonBody, C_BaseAnimatingOverlay );

    bool Initialize( C_BasePlayer* pOwner );
    void AttachToEntity( C_BasePlayer* pOwner );


    virtual void BuildTransformations( CStudioHdr* pHdr, Vector* pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList& boneComputed ) OVERRIDE;

    virtual const Vector& GetRenderOrigin() OVERRIDE;
    virtual const QAngle& GetRenderAngles() OVERRIDE;
};
