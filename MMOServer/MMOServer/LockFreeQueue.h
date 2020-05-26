#pragma once

#include "CrashDump.h"

#include <Windows.h>
//#include <algorithm>
#include "MemoryPool.h"

template <class T>
class LockFreeQueue
{
private:
	struct NODE
	{
		NODE(T data)
		{
			next = NULL;
			item = data;
		}

		NODE()
		{
			next = NULL;
		}

		T item;
		NODE *next;
	};

	struct END_NODE
	{
		END_NODE()
		{
			node = NULL;// new NODE;
			//node=new NODE();
			//node->next = node;
			check = 0;
		}

		NODE *node;
		long long check;
	};
public:
	LockFreeQueue();
	LockFreeQueue(int maxCount);
	~LockFreeQueue();
	bool Enqueue(T data);
	bool Dequeue(T *data = NULL);
	int GetUseCount() { return _useCount; }
	int GetFreeCount() { return _maxCount - _useCount; }

	//이함수는 쓰레드에 안전하지 않음
	//단 이 서버에서는 1번에 1회의 send만을 시행하므로 문제는 발생하지 않을 것으로 예상
	int Peek(T *peekData, int size);

private:
	END_NODE * volatile _head;
	END_NODE * volatile _tail;

	unsigned long long _headCheckNum;
	unsigned long long _tailCheckNum;

	int _useCount;
	int _maxCount;

	MemoryPool<NODE> *queuePool;
};

template<class T>
LockFreeQueue<T>::LockFreeQueue()
	:_headCheckNum(0), _tailCheckNum(0), _useCount(0), _maxCount(0)
{
	queuePool = new MemoryPool<NODE>(1000, false);
	_head = new END_NODE;
	_head->node = queuePool->Alloc();
	_tail = new END_NODE;
	_tail->node = _head->node;
}

template<class T>
LockFreeQueue<T>::LockFreeQueue(int maxCount)
	:_headCheckNum(0), _tailCheckNum(0), _useCount(0), _maxCount(maxCount)
{
	queuePool = new MemoryPool<NODE>(maxCount, true);
	_head = new END_NODE;
	_head->node = queuePool->Alloc();
	_tail = new END_NODE;
	_tail->node = _head->node;
}

template<class T>
LockFreeQueue<T>::~LockFreeQueue()
{
	delete _head;
	delete _tail;
	delete queuePool;
}

template<class T>
bool LockFreeQueue<T>::Enqueue(T data)
{
	NODE *newNode = queuePool->Alloc();
	END_NODE tail;
	NODE *next;

	//추적용
	//ULONG trackTemp = InterlockedIncrement((LONG *)&trackCur);
	//InterlockedExchange64((LONG64 *)&track[trackTemp % TRACK_MAX], (LONG64)newNode);
	//추적용

	if (newNode == NULL)
		return false;

	newNode->item = data;
	//newNode->next = NULL;


	unsigned long long checkNum = InterlockedIncrement64((LONG64 *)&_tailCheckNum);


	while (1)
	{
		// = _tail;
		tail.check = _tail->check;
		tail.node = _tail->node;


		next = tail.node->next;

		if (next == NULL)
		{
			//tail의 next가 그대로 NULL일 때 진입
			if (InterlockedCompareExchangePointer((PVOID *)&tail.node->next, newNode, next) == NULL)
			{
				//_tail은 새로운 node가 됨
				//실패한다면 이 시점에 서는 _tail이 이미 변경됬음 => 변경된 곳에서 _tail을 맞춰줄 것을 기대한다
				//위의 interlock과는 독립적으로 돌아가기때문에 _tail뒤에는 이미 새로운 node가 들어왔을 가능성이 있음
				//(하지만 _tail은 여전히 현재 tail을 가리키므로 _tail의 next가 null이 아니게됨)
				InterlockedCompareExchange128((LONG64 *)_tail, checkNum, (LONG64)tail.node->next, (LONG64 *)&tail);

				break;
			}
		}
		//tail의 next가 NULL이 아닌 경우 tail을 옮겨줘야함
		//단 이경우에도 _tail은 atomic하게 변경되어야함
		else
		{

			InterlockedCompareExchange128((LONG64 *)_tail, checkNum, (LONG64)tail.node->next, (LONG64 *)&tail);
			checkNum = InterlockedIncrement64((LONG64 *)&_tailCheckNum);//_tail이 변경됨에 따라서 checkNum도 변경
		}
	}

	InterlockedIncrement((LONG *)&_useCount);

	return true;
}

template<class T>
bool LockFreeQueue<T>::Dequeue(T *data)
{
	//isEmpty를 atomic할 수 있게
	if (InterlockedDecrement((LONG *)&_useCount) < 0)
	{
		InterlockedIncrement((LONG *)&_useCount);
		return false;
	}


	T popData;// = (T)NULL;
	END_NODE h;
	NODE *next;
	END_NODE tail;
	//NODE *newHead = NULL;
	unsigned long long checkNum = InterlockedIncrement64((LONG64 *)&_headCheckNum);//이 pop행위의 checkNum은 함수 시작 시에 결정


	while (1)
	{
		//if (_head->node == _tail->node&&InterlockedCompareExchange((LONG *)&_head->node->next, NULL, NULL) == NULL)
		//{
		//	volatile int test = 1;
		//	CrashDump::Crash();
		//}

		//END_NODE h;
		h.check = _head->check;
		h.node = _head->node;
		next = h.node->next;

		//tail이 밀리지 않았을 때 모든 head를 빼내게 될 경우 tail이 유실될 수 있다.
		tail;// = _tail;
		tail.check = _tail->check;
		tail.node = _tail->node;

		if (tail.node->next != NULL)
		{

			unsigned long long tailCheckNum = InterlockedIncrement64((LONG64 *)&_tailCheckNum);//_tail이 변경됨에 따라서 checkNum도 변경

			InterlockedCompareExchange128((LONG64 *)_tail, tailCheckNum, (LONG64)tail.node->next, (LONG64 *)&tail);

			continue;
		}




		//
		//if(h.node->next==NULL)
		if (next == NULL)
			continue;

		//memcpy(&popData, &(h.node->next->item), sizeof(T));
		//memcpy(&popData, &next->item, sizeof(T));
		popData = next->item;

		if (InterlockedCompareExchange128((LONG64 *)_head, checkNum, (LONG64)h.node->next, (LONG64 *)&h))
		{
			//queuePool.Free(h.node);//<-이놈이 free가 되었는데 이 값이 재 queue에서 불려와 재사용될 때 문제가 생기는 것같음
			if (data != NULL)
				*data = popData;
			break;
		}
	}

	/*
	while (!InterlockedCompareExchange128((LONG64 *)_head,checkNum,(LONG64)_head->node->next,(LONG64 *)&h))
	{
		//tail이 밀리지 않았을 때 모든 head를 빼내게 될 경우 tail이 유실될 수 있다.
		END_NODE tail;// = _tail;
		tail.check = _tail->check;
		tail.node = _tail->node;
		if (tail.node->next != NULL)
		{
			unsigned long long tailCheckNum = InterlockedIncrement64((LONG64 *)&_tailCheckNum);//_tail이 변경됨에 따라서 checkNum도 변경
			if (InterlockedCompareExchange128((LONG64 *)_tail, tailCheckNum, (LONG64)tail.node->next, (LONG64 *)&tail))
			{
				volatile int test = 1;
				NODE *temp = _tail->node;
				if (temp->next == temp)
				{
					CrashDump::Crash();
				}
			}
		}

		newHead = h.node->next;

		//newHead가 null인 경우는 _head가 재사용됬을 때뿐이다. => useCount가 0인데 dequeue한 경우엔 while문 안으로 진입이 안됨
		//따라서 newHead가 null인 경우 자연스럽게 _head와 h가 일치할 수 없고(_head는 이미 바뀌었으므로) while루프가 돌게된다.
		//newHead가 null인 경우에도 _head가 h와 동일하지 않다면 _head가 재사용된 것이므로 popData가 잘못되서 반환될 가능성은 없다
		if(newHead!=NULL)
			popData = newHead->item;
	}
	*/
	//추적용
	//ULONG trackTemp = InterlockedIncrement((LONG *)&trackCur);
	//InterlockedExchange64((LONG64 *)&track[trackTemp % TRACK_MAX], (LONG64)h.node | 0x1000000000000000);
	//추적용

	queuePool->Free(h.node);


	return true;
}

template<class T>
int LockFreeQueue<T>::Peek(T *peekData, int size)
{
	NODE *cur = _head->node;
	int i;
	NODE *curNext;
	for (i = 0; i < _useCount; i++)
	{
		curNext = cur->next;
		peekData[i] = cur->next->item;

		cur = cur->next;
	}

	return i;
}