#include "../BaseVSShader.h"


#include "SDK_screenspaceeffect_vs20.inc"


#include "zmr_test_pixelshader_ps20.inc"
#include "zmr_test_pixelshader_ps20b.inc"


ConVar zm_cl_vignettestrength( "zm_cl_vignettestrength", "1" );

BEGIN_SHADER( ZMR_Test, "Help for my shader." )
	BEGIN_SHADER_PARAMS
        SHADER_PARAM( FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "" )
        SHADER_PARAM( VIGNETTETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
	END_SHADER_PARAMS


	SHADER_INIT
	{
		if ( params[FBTEXTURE]->IsDefined() )
		{
			LoadTexture( FBTEXTURE );
		}
		if ( params[VIGNETTETEXTURE]->IsDefined() )
		{
			LoadTexture( VIGNETTETEXTURE );
		}
	}

    SHADER_INIT_PARAMS()
    {
    }


	bool NeedsFullFrameBufferTexture( IMaterialVar** params, bool bCheckSpecificToThisFrame ) const
	{
		return true;
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			Assert(0);
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
            //pShaderShadow->EnableAlphaWrites( true );

            pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
            pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );


			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0);


			DECLARE_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
			SET_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);


            SET_STATIC_PS2X_PIXEL_SHADER_NO_COMBOS(zmr_test_pixelshader)
		}


		DYNAMIC_STATE
		{
            BindTexture( SHADER_SAMPLER0, FBTEXTURE, -1 );
            BindTexture( SHADER_SAMPLER1, VIGNETTETEXTURE, -1 );


			DECLARE_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
			SET_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);


            const float strength = zm_cl_vignettestrength.GetFloat();
            pShaderAPI->SetPixelShaderConstant( 0, &strength, 1 );


            
            SET_DYNAMIC_PS2X_PIXEL_SHADER_NO_COMBOS(zmr_test_pixelshader)
		}

		Draw();
	}

END_SHADER