#pragma once

#include <assert.h>
#include "awaitable.h"

//��װһ��Э�̺������awaitable
template<typename T>
inline Awaitable<T> create_awaitable(Awaitable<T>&& aw)
{
	//����һ����ֵ����,����Դ��awת��
	return Awaitable<T>(std::move(aw));
}


//��װһ��Э�̶���
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

