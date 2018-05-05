#pragma once

#include "npcr/npcr_basenpc.h"


// Uncomment if bot_* commands are defined somewhere else.
#define NPCR_BOT_CMDS


namespace NPCR
{
    class CPlayerMotor;

    abstract_class CPlayerCmdHandler : public NPCR::CBaseNPC
    {
    public:
        CPlayerCmdHandler( CBaseCombatCharacter* pNPC );
        ~CPlayerCmdHandler();

#define DECLARE_BTN(name)           void name( float hold = 0.0f ); \
                                    void Un##name(); \
                                    bool IsIn##name() const; \
                                private: \
                                    CountdownTimer m_##name; \
                                public:

#define DEFINE_BTN(clss,name)       void clss::name( float hold ) \
                                    { \
                                        m_##name.Start( hold ); \
                                    } \
                                    void clss::Un##name() \
                                    { \
                                        m_##name.Invalidate(); \
                                    } \
                                    bool clss::IsIn##name() const \
                                    { \
                                        return m_##name.HasStarted(); \
                                    }

#define HANDLE_BTN(name,inflag,cmd) if ( m_##name.HasStarted() ) \
                                    { \
                                        cmd.buttons |= inflag; \
                                         \
                                        if ( m_##name.IsElapsed() ) \
                                            m_##name.Invalidate(); \
                                    }

        //
        DECLARE_BTN( PressFire1 )
        DECLARE_BTN( PressFire2 )
        DECLARE_BTN( PressDuck )
        DECLARE_BTN( PressReload )
        DECLARE_BTN( PressUse )
        //


        CPlayerMotor* GetPlayerMotor() const;

        virtual CBaseMotor* CreateMotor() OVERRIDE;



        virtual void BuildPlayerCmd( CUserCmd& cmd );
    };

    template<typename PlayerClass>
    class CPlayer : public PlayerClass, public CPlayerCmdHandler
    {
    public:
        CPlayer() : CPlayerCmdHandler( this )
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
        virtual bool OverrideUserCmd( CUserCmd& com ) { return false; }


        virtual void PhysicsSimulate() OVERRIDE
        {
            Update();


            CUserCmd cmd;
            cmd.Reset();
            cmd.command_number = gpGlobals->tickcount;

            if ( !OverrideUserCmd( cmd ) )
            {
                BuildPlayerCmd( cmd );
            }

            cmd.viewangles = PlayerClass::EyeAngles();


            PlayerClass::ProcessUsercmds( &cmd, 1, 1, 0, false );

            PlayerClass::PhysicsSimulate();
        }
    };
}
