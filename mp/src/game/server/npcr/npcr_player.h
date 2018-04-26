#pragma once

#include "npcr/npcr_basenpc.h"

namespace NPCR
{
    template<typename PlayerClass>
    class CPlayer : public PlayerClass, public NPCR::CBaseNPC
    {
    public:
        CPlayer() : NPCR::CBaseNPC( this )
        {
        }
        
        ~CPlayer()
        {
        }

        virtual void PostConstructor( const char* szClassname ) OVERRIDE
        {
            PlayerClass::PostConstructor( szClassname );

            CBaseNPC::PostConstructor();
        }
        
        //CPlayerMotor* GetPlayerMotor() const { return static_cast<CPlayerMotor*>( CBaseNPC::GetMotor() ); }


        virtual bool IsNetClient() const OVERRIDE { return false; }
        virtual bool IsFakeClient() const OVERRIDE { return true; }
        virtual bool IsBot() const OVERRIDE { return true; }

        virtual CBaseNPC* MyNPCRPointer() OVERRIDE { return this; }

        
        template<typename BotClass>
        static PlayerClass* CreateBot( const char* playername )
        {
            ClientPutInServerOverride( &BotClass::BotPutInServer );
            edict_t* pEdict = engine->CreateFakeClientEx( playername );
            ClientPutInServerOverride( nullptr );

            if ( !pEdict )
            {
                Warning( "Failed to create Bot.\n" );
                return nullptr;
            }


            PlayerClass* pPlayer = dynamic_cast<PlayerClass*>( CBaseEntity::Instance( pEdict ) );

            return pPlayer;
        }
        
        
        virtual bool RemoveNPC() OVERRIDE
        {
            // We can't be removed as an entity. Kick instead.
            int userid = GetUserID();
            if ( userid != -1 )
            {
                engine->ServerCommand( UTIL_VarArgs( "kickid %i", userid ) );
                return true;
            }
            
            return false;
        }


        // Return true if you've overridden the user cmd.
        virtual bool OverrideUserCmd( CUserCmd* pCom ) { return false; }


        virtual void PhysicsSimulate() OVERRIDE
        {
            Update();


            CUserCmd cmd;
            cmd.Reset();
            cmd.command_number = gpGlobals->tickcount;

            if ( !OverrideUserCmd( &cmd ) /*&& !GetPlayerMotor()->GetBuiltUserCmd( &cmd )*/ )
            {
                ;
            }

            PlayerClass::ProcessUsercmds( &cmd, 1, 1, 0, false );

            PlayerClass::PhysicsSimulate();
        }
    };
}
