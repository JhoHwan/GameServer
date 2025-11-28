#pragma once
template<typename B, typename T>
using Base_Check = typename enable_if<is_base_of<T, B>::value, bool>::type;

class Job
{
	using Function = function<void()>;

public:
	Job() = delete;
	Job(const Job&) = delete;
	Job(Job&&) = default;

	// Derived Member Function
	template<typename R, typename T, typename B, typename... Args, Base_Check<T, B> = NULL>
	Job(R(T::* func)(Args...), B* obj, Args... args);

	// Member Function
	template<typename R, typename Obj, typename... Args>
	Job(R(Obj::* func)(Args...), Obj* obj, Args... args);

	template<typename R, typename Obj, typename... Args>
	Job(R(Obj::* func)(Args...), shared_ptr<Obj> obj, Args... args);

	// Static or Function
	template<typename R, typename... Args>
	Job(R(*func)(Args...), Args... args);

	// Lamdba
	Job(Function&& func);
	
	void Execute() const;
	void operator() () const;

private:
	Function _func;
};

template<typename R, typename T, typename B, typename ...Args, Base_Check<T, B>>
inline Job::Job(R(T::* func)(Args...), B* obj, Args ...args)
{
	_func = std::move([obj, func, args...]() { (static_cast<T*>(obj)->*func)(args...); });
}
template<typename R, typename Obj, typename ...Args>
inline Job::Job(R(Obj::* func)(Args...), Obj* obj, Args ...args)
{
	_func = std::move([obj, func, args...]() { (obj->*func)(args...); });
}

template<typename R, typename ...Args>
inline Job::Job(R(*func)(Args...), Args ...args)
{
	_func = std::move([func, args...]() { (*func)(args...); });
}

template<typename R, typename Obj, typename... Args>
inline Job::Job(R(Obj::* func)(Args...), shared_ptr<Obj> obj, Args... args)
{
	_func = std::move([obj, func, args...]() { (obj.get()->*func)(args...); });
}
