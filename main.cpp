#include <iostream>
#include "co.h"
#include "coroutine.h"


////TODO:��������funtion or lamba���ʽ
//Awaiter _co_coroutine()
//{
//	//����һ��awaiter
//	return Awaiter();
//}
//
////TODO:��Ҫһ������ֵ ����aw yield�ķ���ֵ ��������ģ�庯��
//void _co_resume(Awaiter&& aw)
//{
//	//����awaiter 
//	aw.Resume();
//
//	//�ȴ�����ֵ
//
//}
//
////���� coroutine���� coroutine ����Ϊ����״̬�� ����Ϊ����ֵ
//void _co_yield(void* p)
//{
//
//}

Awaiter ResumeCo(Awaiter&& aw)
{
	if (!aw.handle.done())
	{
		aw.Resume();

		co_await aw;
	}

	co_return {};
}

Awaiter& Resume(Awaiter&& aw)
{
	ResumeCo(std::move(aw)).Resume();

	return aw;
}

Awaiter& Corotine(Awaiter&& aw)
{
	return aw;
}

Awaiter my_oroutine2()
{
	std::cout << "my_oroutine 11" << std::endl;
	co_yield 11;

	std::cout << "my_oroutine 22" << std::endl;
	co_yield 22;

	std::cout << "my_oroutine 33" << std::endl;
	co_return 33;
}

Awaiter my_oroutine()
{
	std::cout << "my_oroutine 1" << std::endl;
	co_yield 1;

	//����һ����Э��coco ջ����
	auto coco = Corotine(my_oroutine2());
	co_await Resume(std::move(coco));//�ȴ�coco����

	std::cout << "my_oroutine 2" << std::endl;
	co_yield 2;

	co_await Resume(std::move(coco));//�ȴ�coco����
	std::cout << "coco2:" << coco.GetVal() << std::endl;

	std::cout << "my_oroutine 3" << std::endl;
	co_yield 3;

	co_await Resume(std::move(coco));//�ȴ�coco����
	std::cout << "coco3:" << coco.GetVal() << std::endl;

	std::cout << "my_oroutine 4" << std::endl;
	co_yield 4;

	co_await Resume(std::move(coco));//�ȴ�coco����
	std::cout << "coco4:" << coco.GetVal() << std::endl;

	co_return 5;
}

int main1()
{
	auto aw = Corotine(my_oroutine());

	std::cout << Resume(std::move(aw)).GetVal() << std::endl;
	std::cout << Resume(std::move(aw)).GetVal() << std::endl;
	std::cout << Resume(std::move(aw)).GetVal() << std::endl;
	std::cout << Resume(std::move(aw)).GetVal() << std::endl;
	std::cout << Resume(std::move(aw)).GetVal() << std::endl;
	std::cout << Resume(std::move(aw)).GetVal() << std::endl;
	std::cout << Resume(std::move(aw)).GetVal() << std::endl;
	return 0;
}

//======================================================================
//
//template<typename T>
//Awaitable<T> resumeco(Awaitable<T>&& aw)
//{
//	std::cout << "resumeco start:" << &aw << std::endl;
//
//	if (!aw.promise().done())
//	{
//		co_await aw;
//	}
//
//	co_return {};
//}

template<typename T>
Awaitable<T>&& resume(Awaitable<T>&& aw)
{
	aw.resume();

	return std::move(aw);
}

//��װһ��Э�̺������awaitable
template<typename T>
Awaitable<T> create_corotine(Awaitable<T>&& aw)
{
	//����һ����ֵ����,����Դ��awת��
	return Awaitable<T>(std::move(aw));
}

Awaitable<std::string> coroutine1()
{
	std::cout << "coroutine1 1" << std::endl;
	co_yield std::string("coroutine1 no done");

	std::cout << "coroutine1 2" << std::endl;
	co_return std::string("coroutine1 done");
}

Awaitable<std::string> coroutine2()
{
	std::cout << "coroutine2 1" << std::endl;
	co_yield std::string("coroutine2 no done");

	std::cout << "coroutine2 2" << std::endl;
	co_return std::string("coroutine2 done");
}




void test1()
{
	auto aw1 = create_corotine(coroutine1());
	aw1.resume();
	std::cout << aw1.get_value() << std::endl;

	aw1.resume();
	std::cout << aw1.get_value() << std::endl;
}

void test2()
{
	auto aw1 = create_corotine(coroutine1());
	auto aw2 = create_corotine(coroutine2());

	aw1.resume();
	aw2.resume();
	std::cout << aw1.get_value() << std::endl;
	std::cout << aw2.get_value() << std::endl;

	aw1.resume();
	aw2.resume();
	std::cout << aw1.get_value() << std::endl;
	std::cout << aw2.get_value() << std::endl;
}

#include <chrono>
#include <thread>
Awaitable<std::string> coroutine3()
{
	std::cout << "coroutine3 start" << std::endl;

	co_yield std::string("coroutine3 do someting...");

	std::cout << "coroutine3 continue" << std::endl;

	//std::this_thread::sleep_for(std::chrono::seconds(3)); // ˯��3����

	co_return std::string("coroutine3 done");
}

void test3()
{
	std::cout << "test3 start" << std::endl;

	auto aw1 = create_corotine(coroutine3());

	aw1.resume();
	std::cout << aw1.get_value() << std::endl;

	aw1.resume();
	std::cout << aw1.get_value() << std::endl;

	std::cout << "test3 over" << std::endl;
}

Awaitable<std::string> coroutine4_1()
{
	co_return std::string("coroutine4_1 done");
}

Awaitable<std::string> coroutine4()
{
	std::cout << "coroutine4 start" << std::endl;

	co_yield std::string("coroutine4 do someting...");

	std::cout << co_await coroutine4_1() << std::endl;

	std::cout << "coroutine4 continue" << std::endl;

	co_return std::string("coroutine4 done");
}

Awaitable<std::string> coroutine_work()
{
	std::cout << "coroutine_work start" << std::endl;
	
	Coroutine co1(coroutine3());

	auto wait_co2 = create_corotine(coroutine4());

	std::cout << co_await co1.awaitable() << std::endl;

	std::cout << co_await wait_co2 << std::endl;

	std::cout << co_await co1.awaitable() << std::endl;

	std::cout << co_await wait_co2 << std::endl;

	std::cout << "coroutine_work end" << std::endl;

	co_yield std::string("coroutine_work yield.");

	co_return std::string("coroutine_work done");
}

void test4()
{
	auto aw1 = create_corotine(coroutine_work());

	aw1.resume();
	std::cout << aw1.get_value() << std::endl;

	aw1.resume();
	std::cout << aw1.get_value() << std::endl;

	std::cout << "test3 over" << std::endl;
}

void test_await()
{
	std::cout << "test_await start" << std::endl;
	
	auto aw1 = create_corotine(coroutine_work());

	//����coroutine_workЭ��
	aw1.resume();

	//��ȡcoroutine_work����ֵ
	std::cout << aw1.get_value() << std::endl;

	std::cout << "test_await over" << std::endl;
}

int main()
{
	//test1();
	//std::cout << "=========================" << std::endl;
	//test2();
	//std::cout << "=========================" << std::endl;
	//test3();
	//std::cout << "=========================" << std::endl;
	//test4();

	//test_await();

	Coroutine co(coroutine_work());
	std::cout << co.get_val() << std::endl;
	co.resume();
	std::cout << co.get_val() << std::endl;
}


