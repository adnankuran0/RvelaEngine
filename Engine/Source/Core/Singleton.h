#pragma once

template <typename T>
class Singleton
{
public:
	static T& Get() noexcept
	{
		return *s_Instance;
	}

	static bool Exists() noexcept { return s_Instance != nullptr; }

	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	Singleton(Singleton&&) = delete;
	Singleton& operator=(Singleton&&) = delete;

protected:
	Singleton()
	{
		assert(!s_Instance && "Only one instance allowed");
		s_Instance = static_cast<T*>(this);
	}
	~Singleton() { s_Instance = nullptr; }

private:
	inline static T* s_Instance = nullptr;
};


