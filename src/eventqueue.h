//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A global class that holds a prioritized queue of entity I/O events.
//			Events can be posted with a nonzero delay, which determines how long
//			they are held before being dispatched to their recipients.
//
//			The queue is serviced once per server frame.
//
//=============================================================================//

#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H
#ifdef _WIN32
#pragma once
#endif

class CUtlMemoryPool;
using Alloc_t = void*(*)(CUtlMemoryPool*, size_t);
using Free_t = void(*)(CUtlMemoryPool*, void*);

struct EventQueuePrioritizedEvent_t
{
	float m_flFireTime;
	string_t m_iTarget;
	string_t m_iTargetInput;
	EHANDLE m_pActivator;
	EHANDLE m_pCaller;
	int m_iOutputID;
	EHANDLE m_pEntTarget;  // a pointer to the entity to target; overrides m_iTarget

	variant_t m_VariantValue;	// variable-type parameter

	EventQueuePrioritizedEvent_t *m_pNext;
	EventQueuePrioritizedEvent_t *m_pPrev;

	DECLARE_SIMPLE_DATADESC();
	
	static Alloc_t Alloc;
	static Free_t Free;
	static CUtlMemoryPool* s_Allocator;
	inline void* operator new( size_t size ) { return Alloc(s_Allocator, size); }
	inline void* operator new( size_t size, int nBlockUse, const char *pFileName, int nLine ) { return Alloc(s_Allocator, size); }
	inline void  operator delete( void* p ) { Free(s_Allocator, p); }
	inline void  operator delete( void* p, int nBlockUse, const char *pFileName, int nLine ) { Free(s_Allocator, p); }
};

Alloc_t EventQueuePrioritizedEvent_t::Alloc;
Free_t EventQueuePrioritizedEvent_t::Free;
CUtlMemoryPool* EventQueuePrioritizedEvent_t::s_Allocator;

class CEventQueue
{
public:
	// pushes an event into the queue, targeting a string name (m_iName), or directly by a pointer
	void AddEvent( const char *target, const char *action, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID = 0 );
	void AddEvent( CBaseEntity *target, const char *action, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID = 0 );
	void AddEvent( CBaseEntity *target, const char *action, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID = 0 );

	void CancelEvents( CBaseEntity *pCaller );
	void CancelEventOn( CBaseEntity *pTarget, const char *sInputName );
	bool HasEventPending( CBaseEntity *pTarget, const char *sInputName );

private:

	void AddEvent( EventQueuePrioritizedEvent_t *event );
	void RemoveEvent( EventQueuePrioritizedEvent_t *pe );

	DECLARE_SIMPLE_DATADESC();
	EventQueuePrioritizedEvent_t m_Events;
	int m_iListCount;
};

#endif // EVENTQUEUE_H

