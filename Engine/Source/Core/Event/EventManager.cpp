#include "EventManager.h"
std::vector<Event*> EventManager::m_EventQueue = std::vector<Event*>();

std::vector<Event*>& EventManager::getEventQueue()
{
	return m_EventQueue;
}

void EventManager::pushEvent(Event* event)
{
	m_EventQueue.push_back(event);
}

void EventManager::clearEvents()
{
	m_EventQueue.clear();
}

void EventManager::dispatchEvents(const std::function<void(Event&)>& handler)
{ 
	EventDispatcher::Dispatch(m_EventQueue, handler); 
}