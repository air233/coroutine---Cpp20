#pragma once

#include <assert.h>
#include "awaitable.h"

//包装一个协程函数获得awaitable
template<typename T>
inline Awaitable<T> create_awaitable(Awaitable<T>&& aw)
{
	//返回一个右值对象,将资源从aw转移
	return Awaitable<T>(std::move(aw));
}


//包装一个协程对象
template <typename T>
class Coroutine {
public:
	Coroutine(Awaitable<T>&& aw);
	~Coroutine();
	Awaitable<T>& awaitable();

	T get_val()
	{
		return _aw.get_value();
	}

	bool resume()
	{
		return _aw.resume();
	}

private:
	Awaitable<T> _aw;
};

template <typename T>
inline Awaitable<T>& Coroutine<T>::awaitable()
{
	return _aw;
}

template <typename T>
inline Coroutine<T>::Coroutine(Awaitable<T>&& aw):_aw(std::move(aw))
{
	_aw.resume();
}

template <typename T>
inline Coroutine<T>::~Coroutine()
{

}

