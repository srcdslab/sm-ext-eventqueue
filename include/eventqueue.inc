#if defined _EventQueue_Included
 #endinput
#endif
#define _EventQueue_Included

/* Adds the event into the correct spot in the priority queue, targeting entity via string name
 *
 * @param target		Target name(could be full entity's name or wildcard or classname)
 * @param targetInput	Input name
 * @param param			Input parameter
 * @param delay			Input delay
 * @param activator		Input activator
 * @param caller		Input caller
 * @param outputID		Unknown
 *
 * @noreturn
*/
native void EQ_AddEventByName(const char[] target, const char[] targetInput, const char[] param = NULL_STRING, float delay = 0.0, int activator = -1, int caller = -1, int outputID = 0);

/* Adds the event into the correct spot in the priority queue, targeting entity via index
 *
 * @param target		Target entity index
 * @param targetInput	Input name
 * @param param			Input parameter
 * @param delay			Input delay
 * @param activator		Input activator
 * @param caller		Input caller
 * @param outputID		Unknown
 *
 * @noreturn
*/
native void EQ_AddEvent(int target, const char[] targetInput, const char[] param = NULL_STRING, float delay = 0.0, int activator = -1, int caller = -1, int outputID = 0);

/* Removes all pending events of the specified type from the I/O queue of the specified target
 *
 * @param target		Target entity index
 * @param sInputName	Input name(could be wildcard; if NULL_STRING then all events will be removed)
 *
 * @noreturn
*/
native void EQ_CancelEventOn(int target, const char[] sInputName = NULL_STRING);

/*  Removes all pending events from the I/O queue that were added by the given caller.
 *
 * @param caller		Caller entity index
 *
 * @noreturn
*/
native void EQ_CancelEvents(int caller);

/* Checks if the target has specified pending inputs
*
 * @param target		Target entity index
 * @param sInputName	Input name(could be wildcard; use NULL_STRING for any input)
 *
 * @return True if the target has specified pending inputs
*/
native bool EQ_HasEventPending(int target, const char[] sInputName = NULL_STRING);

/**
 * Do not edit below this line!
 */
public Extension __ext_eventqueue =
{
	name = "Entity Events Queue",
	file = "EventQueue.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public __ext_outputinfo_SetNTVOptional()
{
	MarkNativeAsOptional("EQ_AddEvent");
	MarkNativeAsOptional("EQ_AddEventByName");
	MarkNativeAsOptional("EQ_CancelEventOn");
	MarkNativeAsOptional("EQ_CancelEvents");
	MarkNativeAsOptional("EQ_HasEventPending");
}
#endif