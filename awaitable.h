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
			//��promise�󶨵���ǰAwaitable
			_promise->set_awaitable(this);
		}
	}

	//��ֹ����
	Awaitable(const Awaitable& other) = delete;
	Awaitable& operator=(const Awaitable& other) = delete;

	//�ƶ����캯��
	Awaitable(Awaitable&& other) noexcept : _promise(other._promise)
	{
		//�ƶ��� _promise����Ȩ�ı�
		if (_promise)
		{
			//����_promise�����õ�Awaitable
			_promise->set_awaitable(this);
		}

		//ԭAwaitable��promiseʧЧ
		promise_type* _new_promise = new promise_type((*other._promise));
		_new_promise->set_valid(false);//���Ϊ��Ч
		_new_promise->set_awaitable(&other);//��promise�󶨵�other
		other._promise = _new_promise;

		//std::cout << "move copy Awaitable new:" << this << ", valid:" << _promise->get_valid() << ", old:" << &other << ", valid:" << other._promise->get_valid() << std::endl;
	}

	~Awaitable()
	{
		//std::cout << "~Awaitable:" << this <<", promise:" << &_promise << ", valid:" << _promise->get_valid() << std::endl;
		
		//_promise���ڹ���ʱ ����������coroutine�������� ��final_suspend()����ֵ����
		// 
		//_promise������ЧʱЭ�̴��ڹ���״̬��ʱ����Ҫ�ͷ�
		if (_promise && _promise->get_valid() == false)
		{
			delete _promise;
			//std::cout << "~Awaitable delete:" << &_promise << std::endl;
			_promise = nullptr;
		}
	}

	bool await_ready()
	{
		//����false,��ʾ��Ҫ����
		return false;
	}

	//co_awaitʱ ���� ����ֵtrueΪ����Э��
	template<typename U>
	auto await_suspend(std::coroutine_handle<AwaitablePromise<U>> handle)
	{
		//����caller
		_caller = &handle.promise();

		//���ûص�
		if (_promise && _promise->get_valid() && !_promise->done())
		{
			_promise->resume();
		}

		//�����ó��� ���»ص�caller
		_caller->resume();

		return true;
	}

	T await_resume()
	{
		//��void����
		if constexpr (!std::is_same<T, void>::value) {
			return _promise->get_value();
		}
	}

	//����Э�� �����Ƿ�ɹ�
	bool resume()
	{
		//std::cout << "resume:" << this << std::endl;

		//_promise��Ч
		if (_promise == nullptr || _promise->get_valid() == 0)
		{
			return false;
		}

		//coδ����
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
		
		//���ɱ���_promise
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

	//������ȷ��ʼ����Э�̺�����ִ����Ϊ������ֵΪ�ȴ��壨awaiter������co_wait�����䷵��ֵ������ֵΪstd::suspend_always ��ʾЭ���������������𣨲�ִ�е�һ��Э�̺����Ĵ��룩������std::suspend_never ��ʾЭ���������������𡣣���Ȼ��Ȼ�Ƿ��صȴ��壬������Լ������ѡ�����ʲô�ȴ�������
	auto initial_suspend()
	{
		return std::suspend_always{};
	}

	//��Э���˳��ǵ��õĽӿڣ�����std::suspend_never�Զ����� coroutine state ����
	auto final_suspend() noexcept
	{
		//Э���˳��� promise�����Զ�����
		return std::suspend_never{};
	}

	//��Э���쳣ʱ����
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

//��Ҫ��awaitable
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
		//��ǰpromiseʧЧ
		set_valid(false);

		//������ǰpromise����ʱ ��Ҫ���󶨵�aw�����
		if(_aw)	_aw->promise_destory();

		//std::cout << "~AwaitablePromise:" << this << std::endl;
	}

	//��������Э�̺����ķ��ض���
	auto get_return_object()
	{
		set_valid(true);
		
		_co_ro = std::coroutine_handle<AwaitablePromise>::from_promise(*this);

		//std::cout << "get_return_object:" << this << std::endl;
		return Awaitable<T>(this);
	}

	//����co_return v������������������Ա���co_return�Ľ��
	void return_value(T value)
	{
		_val = value;
	}

	//����co_yield������������������Ա���co_yield�Ľ�����䷵���䷵��ֵΪstd::suspend_always��ʾЭ�̻�����������std::suspend_never��ʾ������
	auto yield_value(T value)
	{
		_val = value;

		return std::suspend_always{};//����
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
	//�洢ֵ
	T _val;

	//�󶨵�awaitable
	Awaitable<T>* _aw;

	//Э�̶���
	std::coroutine_handle<AwaitablePromise<T>> _co_ro;
};

//void ���л�
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


