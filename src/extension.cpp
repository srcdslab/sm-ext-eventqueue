/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include "isaverestore.h"
#include "variant_t.h"
#include "eventqueue.h"
#include "CDetour/detours.h"
#include <list>

using AddressPool = std::list<EventQueuePrioritizedEvent_t*>;
EventQueue g_EventQueueExt;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_EventQueueExt);

IGameConfig *g_pGameConf = NULL;
CGlobalVars *gpGlobals = NULL;
CBaseEntityList *g_pEntityList = NULL;
CDetour* g_EventQueueRemove = NULL;
static AddressPool EventData;


inline const char* MakeStrCopy(const char* s, size_t len)
{
	char* tmp = new char[len];
	memcpy(tmp, s, len);
	return tmp;
}

inline string_t GetEntityName(CBaseEntity* pEntity)
{
	datamap_t * pMap = gamehelpers -> GetDataMap(pEntity);
	if(!pMap)
		return NULL_STRING;
	typedescription_t *td = gamehelpers->FindInDataMap(pMap, "m_iName");
	if(!td)
		return NULL_STRING;
	return *(string_t*)((uintptr_t)(pEntity) + td->fieldOffset[TD_OFFSET_NORMAL]);
}

void OnEventRemove(EventQueuePrioritizedEvent_t * event)
{
	for(auto begin = EventData.begin(); begin != EventData.end(); ++begin)
	{
		EventQueuePrioritizedEvent_t * cur_event = *begin;
		if(cur_event == event)
		{
			if(cur_event -> m_iTarget != NULL_STRING)
				delete STRING(cur_event -> m_iTarget);
			if(cur_event -> m_iTargetInput != NULL_STRING)
				delete STRING(cur_event -> m_iTargetInput);
			if((cur_event -> m_VariantValue).StringID() != NULL_STRING)
				delete STRING((cur_event -> m_VariantValue).StringID());
			EventData.erase(begin);
			break;
		}
	}
}

DETOUR_DECL_STATIC2(CEventQueueRemove, void, CUtlMemoryPool*, Allocator, void*, p)
{
	if(Allocator == EventQueuePrioritizedEvent_t::s_Allocator)
	{
		EventQueuePrioritizedEvent_t* Event = (EventQueuePrioritizedEvent_t*)p;
		OnEventRemove(Event);
	}
	DETOUR_STATIC_CALL(CEventQueueRemove)(Allocator, p);
}

//-----------------------------------------------------------------------------
// Purpose: private function, adds an event into the list
// Input  : *newEvent - the (already built) event to add
//-----------------------------------------------------------------------------
void CEventQueue::AddEvent( EventQueuePrioritizedEvent_t *newEvent )
{
	EventData.push_back(newEvent);
	// loop through the actions looking for a place to insert
	EventQueuePrioritizedEvent_t *pe;
	for ( pe = &m_Events; pe->m_pNext != NULL; pe = pe->m_pNext )
	{
		if ( pe->m_pNext->m_flFireTime > newEvent->m_flFireTime )
		{
			break;
		}
	}

	// insert
	newEvent->m_pNext = pe->m_pNext;
	newEvent->m_pPrev = pe;
	pe->m_pNext = newEvent;
	if ( newEvent->m_pNext )
	{
		newEvent->m_pNext->m_pPrev = newEvent;
	}
}

void CEventQueue::RemoveEvent( EventQueuePrioritizedEvent_t *pe )
{
	pe->m_pPrev->m_pNext = pe->m_pNext;
	if ( pe->m_pNext )
	{
		pe->m_pNext->m_pPrev = pe->m_pPrev;
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds the action into the correct spot in the priority queue, targeting entity via string name
//-----------------------------------------------------------------------------
void CEventQueue::AddEvent( const char *target, const char *targetInput, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID )
{
	// build the new event
	EventQueuePrioritizedEvent_t *newEvent = new EventQueuePrioritizedEvent_t;
	newEvent->m_flFireTime = gpGlobals->curtime + fireDelay;	// priority key in the priority queue
	newEvent->m_iTarget = MAKE_STRING( MakeStrCopy(target, strlen(target) + 1) );
	newEvent->m_pEntTarget = NULL;
	newEvent->m_iTargetInput = MAKE_STRING( MakeStrCopy(targetInput, strlen(targetInput) + 1) );
	newEvent->m_pActivator = pActivator;
	newEvent->m_pCaller = pCaller;
	newEvent->m_VariantValue = Value;
	newEvent->m_iOutputID = outputID;

	AddEvent( newEvent );
}

//-----------------------------------------------------------------------------
// Purpose: adds the action into the correct spot in the priority queue, targeting entity via pointer
//-----------------------------------------------------------------------------
void CEventQueue::AddEvent( CBaseEntity *target, const char *targetInput, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID )
{
	// build the new event
	EventQueuePrioritizedEvent_t *newEvent = new EventQueuePrioritizedEvent_t;
	newEvent->m_flFireTime = gpGlobals->curtime + fireDelay;	// primary priority key in the priority queue
	newEvent->m_iTarget = NULL_STRING;
	newEvent->m_pEntTarget = target;
	newEvent->m_iTargetInput = MAKE_STRING( MakeStrCopy(targetInput, strlen(targetInput) + 1) );
	newEvent->m_pActivator = pActivator;
	newEvent->m_pCaller = pCaller;
	newEvent->m_VariantValue = Value;
	newEvent->m_iOutputID = outputID;

	AddEvent( newEvent );
}

void CEventQueue::AddEvent( CBaseEntity *target, const char *action, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID )
{
	AddEvent( target, action, variant_t(), fireDelay, pActivator, pCaller, outputID );
}

//-----------------------------------------------------------------------------
// Purpose: Removes all pending events from the I/O queue that were added by the
//			given caller.
//
//			TODO: This is only as reliable as callers are in passing the correct
//				  caller pointer when they fire the outputs. Make more foolproof.
//-----------------------------------------------------------------------------
void CEventQueue::CancelEvents( CBaseEntity *pCaller )
{
	if (!pCaller)
		return;

	EventQueuePrioritizedEvent_t *pCur = m_Events.m_pNext;

	while (pCur != NULL)
	{
		bool bDelete = false;
		if (pCur->m_pCaller == pCaller)
		{
			// Pointers match; make sure everything else matches.
			if (!stricmp(STRING(GetEntityName(pCur->m_pCaller)), STRING(GetEntityName(pCaller))) &&
				!stricmp(gamehelpers -> GetEntityClassname(pCur->m_pCaller), gamehelpers -> GetEntityClassname(pCaller)))
			{
				// Found a matching event; delete it from the queue.
				bDelete = true;
			}
		}

		EventQueuePrioritizedEvent_t *pCurSave = pCur;
		pCur = pCur->m_pNext;

		if (bDelete)
		{
			RemoveEvent( pCurSave );
			delete pCurSave;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Removes all pending events of the specified type from the I/O queue of the specified target
//
//			TODO: This is only as reliable as callers are in passing the correct
//				  caller pointer when they fire the outputs. Make more foolproof.
//-----------------------------------------------------------------------------
void CEventQueue::CancelEventOn( CBaseEntity *pTarget, const char *sInputName )
{
	if (!pTarget)
		return;

	EventQueuePrioritizedEvent_t *pCur = m_Events.m_pNext;
	const char* sTargetName = STRING(GetEntityName(pTarget));
	const char* sTargetClassname = gamehelpers -> GetEntityClassname(pTarget);
	const char* ich = sInputName ? strchr(sInputName, '*') : NULL;
	
	while (pCur != NULL)
	{
		bool bDelete = false;	
		const char* sTarget = STRING(pCur->m_iTarget);
		const char* ch = sTarget ? strchr(sTarget, '*') : NULL;
		if ( pCur->m_pEntTarget == pTarget || 
			(!pCur->m_pEntTarget && sTarget && sTargetName && 
			((ch && strlen(sTargetName) >= strlen(sTarget) - 1 && !Q_strncmp(sTarget, sTargetName, ch - sTarget)) ||
			(!ch && (!stricmp(sTarget, sTargetName) || !stricmp(sTarget, sTargetClassname))))) )
		{
			const char* sInput = STRING(pCur->m_iTargetInput);
			if (!sInputName ||
				(sInput && ((ich && strlen(sInput) >= strlen(sInputName) - 1 && !Q_strncmp(sInput, sInputName, ich - sInputName)) || 
				(!ich && !stricmp(sInput, sInputName)))) )
			{
				// Found a matching event; delete it from the queue.
				bDelete = true;
			}
		}

		EventQueuePrioritizedEvent_t *pCurSave = pCur;
		pCur = pCur->m_pNext;

		if (bDelete)
		{
			RemoveEvent( pCurSave );
			delete pCurSave;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the target has any pending inputs.
// Input  : *pTarget - 
//			*sInputName - NULL for any input, or a specified one
//-----------------------------------------------------------------------------
bool CEventQueue::HasEventPending( CBaseEntity *pTarget, const char *sInputName )
{
	if (!pTarget)
		return false;

	EventQueuePrioritizedEvent_t *pCur = m_Events.m_pNext;
	const char* sTargetName = STRING(GetEntityName(pTarget));
	const char* sTargetClassname = gamehelpers -> GetEntityClassname(pTarget);
	const char* ich = sInputName ? strchr(sInputName, '*') : NULL;
	
	while (pCur != NULL)
	{
		const char* sTarget = STRING(pCur->m_iTarget);
		const char* ch = sTarget ? strchr(sTarget, '*') : NULL;
		if ( pCur->m_pEntTarget == pTarget || 
			(!pCur->m_pEntTarget && sTarget && sTargetName && 
			((ch && strlen(sTargetName) >= strlen(sTarget) - 1 && !Q_strncmp(sTarget, sTargetName, ch - sTarget)) ||
			(!ch && (!stricmp(sTarget, sTargetName) || !stricmp(sTarget, sTargetClassname))))) )
		{
			const char* sInput = STRING(pCur->m_iTargetInput);
			if (!sInputName ||
				(sInput && ((ich && strlen(sInput) >= strlen(sInputName) - 1 && !Q_strncmp(sInput, sInputName, ich - sInputName)) || 
				(!ich && !stricmp(sInput, sInputName)))) )
			{
				return true;
			}
		}

		pCur = pCur->m_pNext;
	}

	return false;
}

CEventQueue* g_EventQueue = NULL;

cell_t Native_AddEvent(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity* pTarget = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	if(!pTarget) return 0;
	
	char* pInputTarget;
	pContext->LocalToString(params[2], &pInputTarget);
	char* pParameter;
	pContext->LocalToStringNULL(params[3], &pParameter);
	float fDelay = *(float *)&params[4];
	CBaseEntity* pActivator = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[5]));
	CBaseEntity* pCaller = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[6]));
	int outputID = *(int *)&params[7];
	if(pParameter == NULL)
		g_EventQueue -> AddEvent(pTarget, pInputTarget, fDelay, pActivator, pCaller, outputID);
	else
	{
		variant_t value;
		value.SetString(MAKE_STRING(MakeStrCopy(pParameter, strlen(pParameter) + 1)));
		g_EventQueue -> AddEvent(pTarget, pInputTarget, value, fDelay, pActivator, pCaller, outputID);
	}
	return 0;
}

cell_t Native_AddEventByName(IPluginContext *pContext, const cell_t *params)
{
	char* pTarget;
	pContext->LocalToString(params[1], &pTarget);
	char* pInputTarget;
	pContext->LocalToString(params[2], &pInputTarget);
	char* pParameter;
	pContext->LocalToStringNULL(params[3], &pParameter);
	float fDelay = *(float *)&params[4];
	CBaseEntity* pActivator = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[5]));
	CBaseEntity* pCaller = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[6]));
	int outputID = *(int *)&params[7];
	variant_t Value;
	if(pParameter != NULL) Value.SetString(MAKE_STRING(MakeStrCopy(pParameter, strlen(pParameter) + 1)));
	g_EventQueue -> AddEvent(pTarget, pInputTarget, Value, fDelay, pActivator, pCaller, outputID);
	return 0;
}

cell_t Native_CancelEventOn(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity* pTarget = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	if(!pTarget) return 0;
	char* pInput;
	pContext->LocalToStringNULL(params[2], &pInput);
	g_EventQueue -> CancelEventOn(pTarget, pInput);
	return 0;
}

cell_t Native_CancelEvents(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity* pCaller = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	if(!pCaller) return 0;
	g_EventQueue -> CancelEvents(pCaller);
	return 0;
}

cell_t Native_HasEventPending(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity* pTarget = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	if(!pTarget) return 0;
	char* pInput;
	pContext->LocalToStringNULL(params[2], &pInput);
	return g_EventQueue -> HasEventPending(pTarget, pInput);
}

const sp_nativeinfo_t MyNatives[] =
{
	{ "EQ_AddEvent", Native_AddEvent },
	{ "EQ_AddEventByName", Native_AddEventByName },
	{ "EQ_CancelEventOn", Native_CancelEventOn },
	{ "EQ_CancelEvents", Native_CancelEvents },
	{ "EQ_HasEventPending", Native_HasEventPending },
	{ NULL, NULL }
};

bool EventQueue::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	char conf_error[255] = "";
	if(!gameconfs->LoadGameConfigFile("EventQueue.games", &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if(conf_error[0])
			snprintf(error, maxlength, "Could not read EventQueue.games.txt: %s", conf_error);

		return false;
	}
	
	if(!g_pGameConf->GetMemSig("g_EventQueue", (void **)&g_EventQueue) || !g_EventQueue)
	{
		snprintf(error, maxlength, "Failed to find g_EventQueue address.\n");
		return false;
	}
	
	if(!g_pGameConf->GetMemSig("EventQueuePrioritizedEvent_t::s_Allocator", (void **)&EventQueuePrioritizedEvent_t::s_Allocator) || !EventQueuePrioritizedEvent_t::s_Allocator)
	{
		snprintf(error, maxlength, "Failed to find EventQueuePrioritizedEvent_t::s_Allocator address.\n");
		return false;
	}
	
	if(!g_pGameConf->GetMemSig("CUtlMemoryPool::Free", (void **)&EventQueuePrioritizedEvent_t::Free) || !EventQueuePrioritizedEvent_t::Free)
	{
		snprintf(error, maxlength, "Failed to find CUtlMemoryPool::Free address.\n");
		return false;
	}
	
	if(!g_pGameConf->GetMemSig("CUtlMemoryPool::Alloc", (void **)&EventQueuePrioritizedEvent_t::Alloc) || !EventQueuePrioritizedEvent_t::Alloc)
	{
		snprintf(error, maxlength, "Failed to find CUtlMemoryPool::Alloc address.\n");
		return false;
	}
	
	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);
	
	g_EventQueueRemove = DETOUR_CREATE_STATIC(CEventQueueRemove, (void*)EventQueuePrioritizedEvent_t::Free);
	if(g_EventQueueRemove == NULL)
	{
		snprintf(error, maxlength, "Could not create detour for CUtlMemoryPool::Free");
		SDK_OnUnload();
		return false;
	}
	g_EventQueueRemove -> EnableDetour();
	
	g_pEntityList = (CBaseEntityList*)gamehelpers -> GetGlobalEntityList();
	return true;
}

bool EventQueue::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	gpGlobals = ismm -> GetCGlobals();
	return true;
}

void EventQueue::SDK_OnAllLoaded()
{
	sharesys->AddNatives(myself, MyNatives);
	sharesys->RegisterLibrary(myself, "Entity Events Queue");
}

void EventQueue::SDK_OnUnload()
{	
	//Remove all events added by this extension
	while(!EventData.empty())
	{
		EventQueuePrioritizedEvent_t* cur_event = EventData.front();
		cur_event->m_pPrev->m_pNext = cur_event->m_pNext;
		if ( cur_event->m_pNext )
		{
			cur_event->m_pNext->m_pPrev = cur_event->m_pPrev;
		}
		delete cur_event;	//Here we call our detour to handle this event properly
	}

	if(g_EventQueueRemove != NULL)
	{
		g_EventQueueRemove -> Destroy();
		g_EventQueueRemove = NULL;
	}
	g_EventQueue = NULL;
	gameconfs->CloseGameConfigFile(g_pGameConf);
	g_pEntityList = NULL;
	EventQueuePrioritizedEvent_t::s_Allocator = NULL;
	EventQueuePrioritizedEvent_t::Free = NULL;
	EventQueuePrioritizedEvent_t::Alloc = NULL;
}