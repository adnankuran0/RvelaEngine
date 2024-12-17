#pragma once
#include <unordered_map>
#include <vector>
#include <functional>
#include <string>

class EventManager
{
public:
	using Callback = std::function<void()>;

	void Subscribe(const std::string& eventName, Callback callback);
	void Dispatch(const std::string& eventName);
private:
	std::unordered_map<std::string, std::vector<Callback>> listeners;

};