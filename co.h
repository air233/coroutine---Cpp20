#pragma once
#include <coroutine>
#include <iostream>

//std::coroutine_handle<T> _coro;

/*
* 
协程构建：
1.调用协程函数
2.系统库 调用 new corotuine state 协程状态
3.  构造协程对象 promise.   result::promise_type
4.  调用promise.get_return_object得到返回对象result
5.  启动协程 调用promise.initial_suspend返回一个awaiter 根据awaiter定义行为
6. co_await promise.initial_suspend -> awaiter
	-> std::suspend_always 挂起
	-> std::suspend_never  继续运行

co_await:
1.协程函数调用 co_wait awaiter  => 构造 awiter
2.系统调用 awaiter.await_ready()
	-> true 表示已准备好 协程继续运行
	-> false 未准备好，通知系统调用 awaiter.awaiter_resume函数
	
	awaiter.awaiter_resume() 返回结果作为co_await的返回值
	-> void or true 挂起 还可以返回一个协程句柄 用于恢复这个协程
	-> false 继续运行

3.handle.resume 协程恢复 

co_yield:
1. 协程函数调用 co_yield cy_ret
2. co_wait promise.yield_value(cy_ret)
3. 记录cy_ret 根据yield_value的返回值
	-> std::suspend_always 挂起
	-> std::suspend_never  继续运行


co_return:
1. co_return co_ret;
2. promise.return_void() or promise.return_value(cr_ret)
3. co_await promise.final_suspend(void) 都不会挂起协程 根据final_suspend返回值
	-> std::suspend_always 
	-> std::suspend_never  需要自己清理handle 调用co_hadnle.destory()

promise:
承诺对象的表现形式必须是result::promise_type，result为协程函数的返回值。
承诺对象是一个实现若干接口，用于辅助协程，构造协程函数返回值；提交传递co_yield，co_return的返回值。明确协程启动阶段是否立即挂起；以及协程内部发生异常时的处理方式。
其接口包括：
	auto get_return_object() ：用于生成协程函数的返回对象。
	
	auto initial_suspend()：用于明确初始化后，协程函数的执行行为，返回值为等待体（awaiter），用co_wait调用其返回值。返回值为std::suspend_always 表示协程启动后立即挂起（不执行第一行协程函数的代码），返回std::suspend_never 表示协程启动后不立即挂起。（当然既然是返回等待体，你可以自己在这儿选择进行什么等待操作）
	
	void return_value(T v)：调用co_return v后会调用这个函数，可以保存co_return的结果
	
	auto yield_value(T v)：调用co_yield后会调用这个函数，可以保存co_yield的结果，其返回其返回值为std::suspend_always表示协程会挂起，如果返回std::suspend_never表示不挂起。
	
	auto final_suspend() noexcept：
		在协程退出是调用的接口，返回std::suspend_never ，自动销毁 coroutine state 对象。
		若 final_suspend 返回 std::suspend_always 则需要用户自行调用 handle.destroy() 进行销毁。但值得注意的是返回std::suspend_always并不会挂起协程。


cooroutine handle:
协程句柄（coroutine handle）是一个协程的标示，用于操作协程恢复，销毁的句柄。

协程句柄的表现形式是std::coroutine_handle<promise_type>​，其模板参数为承诺对象（promise）类型。

句柄有几个重要函数：
	resume()函数可以恢复协程。
	done()函数可以判断协程是否已经完成。
	返回false标示协程还没有完成，还在挂起。

协程句柄和承诺对象之间是可以相互转化的。
std::coroutine_handle<promise_type>::from_promise ：这是一个静态函数，可以从承诺对象（promise）得到相应句柄。
std::coroutine_handle<promise_type>::promise() 函数可以从协程句柄coroutine handle得到对应的承诺对象（promise）

等待体（awaiter）:
co_wait 关键字会调用一个等待体对象(awaiter)。
这个对象内部也有3个接口。根据接口co_wait  决定进行什么操作。
	bool await_ready()：等待体是否准备好了，返回 false ，表示协程没有准备好，立即调用await_suspend。返回true，表示已经准备好了。
	auto await_suspend(std::coroutine_handle<> handle): 如果要挂起，调用的接口。
							其中handle参数就是调用等待体的协程，其返回值有3种可能
							void|true 返回true 立即挂起，返回false 不挂起。
							返回某个协程句柄（coroutine handle），立即恢复对应句柄的运行。
	auto await_resume() ：协程挂起后恢复时，调用的接口。返回值作为co_wait 操作的返回值。
	
	std::suspend_never类，不挂起的的特化等待体类型。
	std::suspend_always类，挂起的特化等待体类型。
*/


struct _Promise
{
	auto get_return_object()
	{
		std::coroutine_handle<_Promise>::from_promise(*this);
	}

	auto initial_suspend() 
	{
		return std::suspend_always{};
	}

	auto final_suspend() noexcept
	{
		return std::suspend_always{};
	}

	auto unhandled_exception()
	{
		std::terminate();
	}

	//void return_void() noexcept 
	//{}

	//TODO:
	// 
	void return_value(int v)
	{
		val = v;
	}

	auto yield_value(int v)
	{
		val = v;
		return std::suspend_always{};
	}

	int val;
};

struct AwaiterBase
{
	using promise_type = _Promise;

	//通过await_ready()判断是否需要等待，如果返回true，就表示不需要等待，如果返回false，就表示需要等待。
	//如果不需要等待，则立刻执行await_resume，否则先执行await_suspend，然后进入等待，
	//调用co_await awaitable(); 的函数会在这里暂停运行，但是不会影响所在线程的执行。
	bool await_ready() 
	{ 
		return false; 
	}

	//我们可以在await_suspend函数中通过传统的回调函数法执行一些异步操作，
	//然后在回调函数中调用std::coroutine_handle<>的resume函数主动恢复。
	auto await_suspend(std::coroutine_handle<> handle) 
	{
		//TODO:
		handle.resume();

		return true;
	};

	//返回co_wait的返回值 这里为void
	//auto await_resume()
	//{
	//	return std::suspend_never{};
	//}

	auto await_resume()
	{
		return _p.val;
	}

	promise_type _p;
};

struct Awaiter : AwaiterBase
{
	Awaiter() = default; // 默认构造函数

	Awaiter(const Awaiter & other) : handle(other.handle) {}

	struct promise_type : public _Promise 
	{
		Awaiter get_return_object() 
		{
			return Awaiter{ std::coroutine_handle<promise_type>::from_promise(*this) };
		}
	};

	Awaiter(std::coroutine_handle<promise_type> h) : handle(h) {}

	void Resume()
	{
		handle.resume();
	}

	~Awaiter() 
	{
		if (handle) handle.destroy();
	}

	int GetVal()
	{
		return handle.promise().val;
	}


	std::coroutine_handle<promise_type> handle;
};
