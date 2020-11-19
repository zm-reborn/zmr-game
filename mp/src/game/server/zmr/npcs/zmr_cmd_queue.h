#pragma once


#include "cbase.h"


enum ZombieCommandType_t : unsigned char
{
    COMMAND_NONE = 0,

    COMMAND_MOVE,
    COMMAND_SWAT,
    COMMAND_CEILINGAMBUSH,

    COMMAND_MAX
};

abstract_class CZMCommandBase
{
public:
    CZMCommandBase( CZMPlayer* pCommander ) { m_hCommander.Set( pCommander ); }

    virtual ZombieCommandType_t     GetCommandType() const { return COMMAND_NONE; }

    CZMPlayer*                      GetCommander() const { return m_hCommander.Get(); }

    //virtual bool                    IsSameCommand( CZMCommandBase* pCom ) { return GetCommandType() == pCom->GetCommandType(); }

    virtual float                   GetExpireTime() const { return 0.0f; }


    virtual CBaseEntity*            GetObjectTarget() const { return nullptr; }
    virtual void                    SetObjectTarget( CBaseEntity* pEnt ) {}

    virtual const Vector&           GetVectorTarget() const { return vec3_origin; }
    virtual void                    SetVectorTarget( const Vector& vecPos ) {}

private:
    CHandle<CZMPlayer> m_hCommander;
};

class CZMCommandMove : public CZMCommandBase
{
public:
    CZMCommandMove( CZMPlayer* pCommander, const Vector& vecPos ) : CZMCommandBase( pCommander )
    {
        m_vecTarget = vecPos;
    }

    virtual ZombieCommandType_t     GetCommandType() const OVERRIDE { return COMMAND_MOVE; }

    virtual const Vector&           GetVectorTarget() const OVERRIDE { return m_vecTarget; }
    virtual void                    SetVectorTarget( const Vector& vecPos ) OVERRIDE { m_vecTarget = vecPos; }

private:
    Vector m_vecTarget;
};

class CZMCommandSwat : public CZMCommandBase
{
public:
    CZMCommandSwat( CZMPlayer* pCommander, CBaseEntity* pEnt, bool bBreak = false ) : CZMCommandBase( pCommander )
    {
        Assert( pEnt );
        m_hSwatObject.Set( pEnt );
        m_bBreak = bBreak;
    }

    virtual ZombieCommandType_t     GetCommandType() const OVERRIDE { return COMMAND_SWAT; }

    /*
    virtual bool                    IsSameCommand( CZMCommandBase* pCom )
    {
        if ( GetCommandType() != pCom->GetCommandType() )
        {
            return false;
        }

        // Allow different objects to be queued separately.
        //return GetObjectTarget() == static_cast<CZMCommandSwat*>( pCom )->GetObjectTarget();
        return true;
    }
    */


    virtual CBaseEntity*            GetObjectTarget() const OVERRIDE { return m_hSwatObject.Get(); }
    virtual void                    SetObjectTarget( CBaseEntity* pEnt ) OVERRIDE { m_hSwatObject.Set( pEnt ); }

    bool    BreakObject() const { return m_bBreak; }

private:
    CHandle<CBaseEntity> m_hSwatObject;
    bool m_bBreak;
};

class CZMCommandCeilingAmbush : public CZMCommandBase
{
public:
    CZMCommandCeilingAmbush( CZMPlayer* pCommander ) : CZMCommandBase( pCommander ) {}


    virtual ZombieCommandType_t     GetCommandType() const OVERRIDE { return COMMAND_CEILINGAMBUSH; }

    virtual float                   GetExpireTime() const OVERRIDE { return 3.0f; }
};

struct ZMCommandSlot_t
{
    ZMCommandSlot_t( CZMCommandBase* pCom )
    {
        Assert( pCom );
        pCommand = pCom;
        
        UpdateTimer( pCommand->GetExpireTime() );
    }

    ~ZMCommandSlot_t()
    {
        delete pCommand;
    }

    void UpdateTimer( float flExpire = 0.0f )
    {
        if ( flExpire > 0.0f )
        {
            expire.Start( flExpire );
        }
        else
        {
            expire.Invalidate();
        }
    }

    CountdownTimer expire;
    CZMCommandBase* pCommand;
};

class CZMCommandQueue
{
public:
    bool QueueCommand( CZMCommandBase* pCommand )
    {
        Assert( pCommand->GetCommandType() != COMMAND_NONE );

        int index = FindCommandIndex( pCommand->GetCommandType() );
        if ( index != -1/* && m_vCommands[index]->pCommand->IsSameCommand( pCommand ) */)
        {
            delete m_vCommands[index]->pCommand;
            m_vCommands[index]->pCommand = pCommand;
            m_vCommands[index]->UpdateTimer( pCommand->GetExpireTime() );
            return false;
        }


        m_vCommands.AddToTail( new ZMCommandSlot_t( pCommand ) );

        return true;
    }

    CZMCommandBase* NextCommand( CZMCommandBase* pCom = nullptr ) const
    {
        if ( pCom )
        {
            int i = FindCommandIndex( pCom );
            return i != -1 ? m_vCommands[i]->pCommand : nullptr;
        }
        else
        {
            return m_vCommands.Count() > 0 ? m_vCommands[0]->pCommand : nullptr;
        }
    }

    void UpdateExpire()
    {
        for ( int i = 0; i < m_vCommands.Count(); i++ )
        {
            ZMCommandSlot_t* slot = m_vCommands[i];
            if ( slot->expire.HasStarted() && slot->expire.IsElapsed() )
            {
                m_vCommands.Remove( i );
                --i;

                delete slot;
            }
        }
    }

    bool RemoveCommand( ZombieCommandType_t com )
    {
        bool bRemoved = false;
        while ( true )
        {
            int index = FindCommandIndex( com );
            if ( index != -1 )
            {
                auto* slot = m_vCommands[index];

                m_vCommands.Remove( index );
                bRemoved = true;

                delete slot;
            }
            else break;
        }
        
        return bRemoved;
    }

    bool RemoveCommand( CZMCommandBase* pCom )
    {
        int index = FindCommandIndex( pCom );
        if ( index != -1 )
        {
            m_vCommands.Remove( index );
            return true;
        }
        
        return false;
    }

    int FindCommandIndex( ZombieCommandType_t com ) const
    {
        for ( int i = 0; i < m_vCommands.Count(); i++ )
        {
            if ( m_vCommands[i]->pCommand->GetCommandType() == com )
                return i;
        }

        return -1;
    }

    int FindCommandIndex( CZMCommandBase* pCom ) const
    {
        for ( int i = 0; i < m_vCommands.Count(); i++ )
        {
            if ( m_vCommands[i]->pCommand == pCom )
                return i;
        }

        return -1;
    }

private:
    CUtlVector<ZMCommandSlot_t*> m_vCommands;
};
