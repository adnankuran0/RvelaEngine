#pragma once

#include "Event.h"
#include "Events.h"
#include "EventDispatcher.h"
#include "KeyEvents.h"
#include "MouseEvents.h"
#include "WindowEvents.h"
#include <vector>

class EventManager
{
public:
	static std::vector<Event*>& getEventQueue(); 
	static void pushEvent(Event* event); 
	static void clearEvents();
	static void dispatchEvents(const std::function<void(Event&)>& handler); 
private:
	static std::vector<Event*> m_EventQueue;
};

