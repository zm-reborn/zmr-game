#include "cbase.h"

#include "npcr_basenpc.h"
#include "npcr_component.h"


NPCR::EventComponent::EventComponent()
{
    m_bSelfCall = false;
}

NPCR::CComponent::CComponent( CEventDispatcher* pOwner, CBaseNPC* pNPC )
{
    m_pNPC = pNPC;
    m_pOwner = nullptr;

    if ( pOwner )
        pOwner->AddComponent( this );
}

NPCR::CComponent::~CComponent()
{
    if ( m_pOwner )
        m_pOwner->RemoveComponent( this );
    m_pOwner = nullptr;
    // Assume others take care of frees.
    //m_Components.PurgeAndDeleteElements();
}

NPCR::CComponent* NPCR::CComponent::GetFriendComponent() const
{
    if ( !m_pOwner )
        return nullptr;

    return m_pOwner->NextComponent( const_cast<CComponent*>( this ) );
}

NPCR::CBaseNPC* NPCR::CComponent::GetNPC() const
{
    return m_pNPC;
}

CBaseCombatCharacter* NPCR::CComponent::GetOuter() const
{
    return GetNPC()->GetCharacter();
}



NPCR::CEventListener::CEventListener( NPCR::CEventDispatcher* pOwner, NPCR::CBaseNPC* pNPC ) : NPCR::CComponent( pOwner, pNPC )
{

}



NPCR::CEventDispatcher::CEventDispatcher( NPCR::CEventDispatcher* pOwner, NPCR::CBaseNPC* pNPC ) : NPCR::CComponent( pOwner, pNPC )
{

}

void NPCR::CEventDispatcher::AddComponent( CComponent* c )
{
    if ( FindComponent( c ) != -1 )
        return;

    m_Components.AddToTail( c );
    c->m_pOwner = this;
    c->m_pNPC = m_pNPC;
}

void NPCR::CEventDispatcher::RemoveComponent( CComponent* c )
{
    m_Components.FindAndRemove( c );
}

int NPCR::CEventDispatcher::FindComponent( CComponent* c ) const
{
    return m_Components.Find( c );
}

NPCR::CComponent* NPCR::CEventDispatcher::NextComponent( CComponent* c ) const
{
    int index = FindComponent( c );
    if ( index >= 0 && (index + 1) < m_Components.Count() )
    {
        return m_Components[index + 1];
    }

    return nullptr;
}

NPCR::CComponent* NPCR::CEventDispatcher::PreviousComponent( CComponent* c ) const
{
    int index = FindComponent( c ) - 1;
    if ( index >= 0 && index < m_Components.Count() )
    {
        return m_Components[index];
    }
            
    return nullptr;
}
