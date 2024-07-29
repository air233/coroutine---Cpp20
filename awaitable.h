#pragma once
#include <coroutine>
#include <memory>

template<typename T>
class AwaitablePromise;

template<typename T>
class Awaitable
{
public:
	using promise_type = AwaitablePromise<T>;

	Awaitable(promise_type* promise) noexcept : _promise(promise)
	{
		//std::cout << "Awaitable:" << this << std::endl;

		if (_promise)
		{
			//将promise绑定到当前Awaitable
			_promise->set_awaitable(this);
		}
	}

	//禁止拷贝
	Awaitable(const Awaitable& other) = delete;
	Awaitable& operator=(const Awaitable& other) = delete;

	//移动构造函数
	Awaitable(Awaitable&& other) noexcept : _promise(other._promise)
	{
		//移动后 _promise所有权改变
		if (_promise)
		{
			//更新_promise的引用的Awaitable
			_promise->set_awaitable(this);
		}

		//原Awaitable的promise失效
		promise_type* _new_promise = new promise_type((*other._promise));
		_new_promise->set_valid(false);//标记为无效
		_new_promise->set_awaitable(&other);//新promise绑定到other
		other._promise = _new_promise;

		//std::cout << "move copy Awaitable new:" << this << ", valid:" << _promise->get_valid() << ", old:" << &other << ", valid:" << other._promise->get_valid() << std::endl;
	}

	~Awaitable()
	{
		//std::cout << "~Awaitable:" << this <<", promise:" << &_promise << ", valid:" << _promise->get_valid() << std::endl;
		
		//_promise处于挂起时 生命周期由coroutine库来控制 由final_suspend()返回值控制
		// 
		//_promise若还有效时协程处于挂起状态此时不需要释放
		if (_promise && _promise->get_valid() == false)
		{
			delete _promise;
			//std::cout << "~Awaitable delete:" << &_promise << std::endl;
			_promise = nullptr;
		}
	}

	bool await_ready()
	{
		//返回false,表示需要挂起
		return false;
	}

	//co_await时 调用 返回值true为挂起协程
	template<typename U>
	auto await_suspend(std::coroutine_handle<AwaitablePromise<U>> handle)
	{
		//保存caller
		_caller = &handle.promise();

		//设置回调
		if (_promise && _promise->get_valid() && !_promise->done())
		{
			_promise->resume();
		}

		//调用让出后 重新回到caller
		_caller->resume();

		return true;
	}

	T await_resume()
	{
		//非void类型
		if constexpr (!std::is_same<T, void>::value) {
			return _promise->get_value();
		}
	}

	//唤醒协程 返回是否成功
	bool resume()
	{
		//std::cout << "resume:" << this << std::endl;

		//_promise无效
		if (_promise == nullptr || _promise->get_valid() == 0)
		{
			return false;
		}

		//co未结束
		if (!_promise->done())
		{
			_promise->resume();
		}

		return true;
	}

	T get_value()
	{
		//std::cout << "get_value:" << this << std::endl;

		return _promise->get_value();
	}

	bool valid()
	{
		return _promise->get_valid();
	}

	void promise_destory()
	{
		//std::cout << "promise_destory old:" << _promise <<", valid:" << _promise->get_valid() << std::endl;
		promise_type* _new_promise = new promise_type((*_promise));
		
		//生成备份_promise
		_promise = _new_promise;
		_promise->set_valid(false);
		_promise->set_awaitable(this);
	}

	promise_type* get_promise()
	{
		return _promise;
	}
private:

	promise_type* _caller { nullptr };
	promise_type* _promise{ nullptr };
};

//==================================================================
class PromiseBase
{
public:
	PromiseBase() :_valid(false) {}
	virtual ~PromiseBase() {}

	//用于明确初始化后，协程函数的执行行为，返回值为等待体（awaiter），用co_wait调用其返回值。返回值为std::suspend_always 表示协程启动后立即挂起（不执行第一行协程函数的代码），返回std::suspend_never 表示协程启动后不立即挂起。（当然既然是返回等待体，你可以自己在这儿选择进行什么等待操作）
	auto initial_suspend()
	{
		return std::suspend_always{};
	}

	//在协程退出是调用的接口，返回std::suspend_never自动销毁 coroutine state 对象。
	auto final_suspend() noexcept
	{
		//协程退出后 promise对象自动析构
		return std::suspend_never{};
	}

	//在协程异常时调用
	void unhandled_exception()
	{
		std::terminate();
	}

	void set_valid(bool valid)
	{
		_valid = valid;
	}

	bool get_valid()
	{
		return _valid;
	}

	bool _valid;
protected:
	//bool _valid;
};
//=====================================================

//需要绑定awaitable
template<typename T>
class AwaitablePromise : public PromiseBase
{
public:
	AwaitablePromise():PromiseBase(), _aw(nullptr)
	{
		//std::cout << "AwaitablePromise:" << this << std::endl;
	}

	AwaitablePromise(const AwaitablePromise& promise) : _co_ro(promise._co_ro), _aw(promise._aw), _val(promise._val)
	{
		_valid = promise._valid;

		//std::cout << "copy AwaitablePromise new:" << this <<", old:"<< &promise << std::endl;
	}

	~AwaitablePromise()
	{
		//当前promise失效
		set_valid(false);

		//析构当前promise对象时 需要将绑定的aw解除绑定
		if(_aw)	_aw->promise_destory();

		//std::cout << "~AwaitablePromise:" << this << std::endl;
	}

	//用于生成协程函数的返回对象。
	auto get_return_object()
	{
		set_valid(true);
		
		_co_ro = std::coroutine_handle<AwaitablePromise>::from_promise(*this);

		//std::cout << "get_return_object:" << this << std::endl;
		return Awaitable<T>(this);
	}

	//调用co_return v后会调用这个函数，可以保存co_return的结果
	void return_value(T value)
	{
		_val = value;
	}

	//调用co_yield后会调用这个函数，可以保存co_yield的结果，其返回其返回值为std::suspend_always表示协程会挂起，如果返回std::suspend_never表示不挂起。
	auto yield_value(T value)
	{
		_val = value;

		return std::suspend_always{};//挂起
	}

	void resume()
	{
		_co_ro.resume();
	}

	bool done()
	{
		return _co_ro.done();
	}

	void set_awaitable(Awaitable<T>* aw)
	{
		_aw = aw;
	}

	T get_value()
	{
		return _val;
	}

private:
	//存储值
	T _val;

	//绑定的awaitable
	Awaitable<T>* _aw;

	//协程对象
	std::coroutine_handle<AwaitablePromise<T>> _co_ro;
};

//void 特列化
template <>
class AwaitablePromise<void> : public PromiseBase
{
public:
	AwaitablePromise() :PromiseBase(), _aw(nullptr)
	{
	}

	AwaitablePromise(const AwaitablePromise& promise) : _co_ro(promise._co_ro), _aw(promise._aw)
	{
		_valid = promise._valid;
	}

	~AwaitablePromise()
	{
		set_valid(false);

		if (_aw)	_aw->promise_destory();
	}

	auto get_return_object()
	{
		_co_ro = std::coroutine_handle<AwaitablePromise<void>>::from_promise(*this);
		return Awaitable<void>(this);
	}

	void return_void()
	{

	}

	void set_awaitable(Awaitable<void>* aw)
	{
		_aw = aw;
	}

	void resume()
	{
		_co_ro.resume();
	}

	bool done()
	{
		return _co_ro.done();
	}

private:
	Awaitable<void>* _aw;
	std::coroutine_handle<AwaitablePromise<void>> _co_ro;
};


