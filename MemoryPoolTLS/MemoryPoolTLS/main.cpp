#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <Windows.h>
#include <process.h>
#include <ctime>
#include <map>
#include "MemoryPool.h"
#include "MemoryPoolTLS.h"
#include "CrashDump.h"
#include "Profiler.h"

#define AMOUNT_MAX 100000
#define THREAD_MAX 30

#define DATA_SIZE 400

unsigned int WINAPI MemoryPoolThread(LPVOID lpParam);
unsigned int WINAPI MemoryPoolTLSThread(LPVOID lpParam);
unsigned int WINAPI NewDeleteThread(LPVOID lpParam);

unsigned int WINAPI testThread(LPVOID lpParam);




struct st_TEST_DATA
{
	unsigned char data[DATA_SIZE];
};

MemoryPool<st_TEST_DATA> *memory;
MemoryPoolTLS<st_TEST_DATA> *memoryTLS;

//CrashDump *Dump;



///////////////////////////////////////////////////////////
//����͸�
///////////////////////////////////////////////////////////
int spinTotal = 0;
int popTotal = 0;
int pushTotal = 0;
int testMode = 0;

int testCnt;
int testAmount;

int main()
{
	CrashDump();
	SYSTEM_INFO sysInfo;
	//timeBeginPeriod(1);
	GetSystemInfo(&sysInfo);
	WCHAR path[MAX_PATH];

	

	ProfileInit();

	//int threadCnt = 1;
	int threadCnt;
	HANDLE threadHandle[100];

	DWORD id;

	printf("set Test Thread Count(max %d)\n>> ",THREAD_MAX);
	scanf("%d", &threadCnt);
	printf("set Test Amount(max %d)\n>> ",AMOUNT_MAX);
	scanf("%d", &testAmount);
	printf("set Test Count\n>> ");
	scanf("%d", &testCnt);
	do
	{
		printf("set Test Mode(0:all alloc - all free 1:one alloc - one free)\n>>");
		scanf("%d", &testMode);
		if (testMode >= 0 &&testMode<=3)
		{
			break;
		}
	}
	while (1);
	
	if (threadCnt > THREAD_MAX)
		threadCnt = THREAD_MAX;
	
	if (testAmount > AMOUNT_MAX)
		testAmount = AMOUNT_MAX;

	

	//threadCnt = 4;
	//testAmount = AMOUNT_MAX;
	//testCnt = 10;
	//testMode = 1;
	

	//test
	//memoryTLS = new MemoryPoolTLS<st_TEST_DATA>(AMOUNT_MAX*threadCnt / 200);
	//for (int i = 0; i < threadCnt; i++)
	//{
	//	threadHandle[i] = (HANDLE)_beginthreadex(NULL, 0, testThread, NULL, 0, (unsigned int *)&id);
	//}
	//
	//while (1) 
	//{
	//	printf("Use Chunk Count %d\n",memoryTLS->GetChunkCount());
	//	Sleep(1000); 
	//}
	//test


	for (int i = 0; i < threadCnt; i++)
	{
		threadHandle[i] = (HANDLE)_beginthreadex(NULL, 0, NewDeleteThread, NULL, 0, (unsigned int *)&id);
	}
	
	DWORD state1 = WaitForMultipleObjects(threadCnt, threadHandle, TRUE, INFINITE);
	
	if (state1 != WAIT_OBJECT_0)
	{
		printf("NewDeleteThread close error\n");
		return -1;
	}
	
	memory = new MemoryPool<st_TEST_DATA>(AMOUNT_MAX*threadCnt/1000);
	
	
	for (int i = 0; i < threadCnt; i++)
	{
		threadHandle[i] = (HANDLE)_beginthreadex(NULL, 0, MemoryPoolThread, NULL, 0, (unsigned int *)&id);
	}
	
	DWORD state2 = WaitForMultipleObjects(threadCnt, threadHandle, TRUE, INFINITE);
	
	if (state2 != WAIT_OBJECT_0)
	{
		printf("MemoryPoolThread close error\n");
		return -1;
	}
	
	delete memory;
	memoryTLS = new MemoryPoolTLS<st_TEST_DATA>(10,false);

	for (int i = 0; i < threadCnt; i++)
	{
		threadHandle[i] = (HANDLE)_beginthreadex(NULL, 0, MemoryPoolTLSThread, NULL, 0, (unsigned int *)&id);
	}
	
	DWORD state3 = WaitForMultipleObjects(threadCnt, threadHandle, TRUE, INFINITE);
	
	if (state3 != WAIT_OBJECT_0)
	{
		printf("MemoryPoolTLSThread close error\n");
		return -1;
	}

	
	

	wsprintf(path, L"thread x%d_test %dx%d size%d.txt",threadCnt,testAmount,testCnt,sizeof(char)*DATA_SIZE);
	ProfileDataSumOutText(path);

	__int64 newdelete = ProfileGetDataSumTotalTime(L"NewDelete");
	__int64 newdeleteCall = ProfileGetDataSumCall(L"NewDelete");
	__int64 newdeleteData = ProfileGetDataSumTotalTime(L"NEWDEL_DAT_SET");
	__int64 mem = ProfileGetDataSumTotalTime(L"MemoryPool");
	__int64 memCall = ProfileGetDataSumCall(L"MemoryPool");
	__int64 memData = ProfileGetDataSumTotalTime(L"MEM_DAT_SET");
	__int64 memTLS = ProfileGetDataSumTotalTime(L"MemoryPoolTLS");
	__int64 memTLSCall = ProfileGetDataSumCall(L"MemoryPoolTLS");
	__int64 memTLSData = ProfileGetDataSumTotalTime(L"MEMTLS_DAT_SET");

	system("cls");
	printf("thread %d test %d*%d Data SIze %dByte\n",threadCnt,testAmount,testCnt, sizeof(char)*DATA_SIZE);
	printf("----------------------------------------------\n");
	printf("newDelete     (Avg)   : %10.4lf\n",(double)(newdelete-newdeleteData)/newdeleteCall / 1000000);
	printf("newDelete     (Total) : %10.4lf\n", (double)(newdelete - newdeleteData) / 1000000);
	printf("memoryPool    (Avg)   : %10.4lf\n", (double)(mem - memData) / memCall / 1000000);
	printf("memoryPool    (Total) : %10.4lf\n", (double)(mem - memData) / 1000000);
	printf("memoryPoolTLS (Avg)   : %10.4lf\n", (double)(memTLS - memTLSData) / memTLSCall / 1000000);
	printf("memoryPoolTLS (Total) : %10.4lf\n", (double)(memTLS - memTLSData) / 1000000);
	system("pause");

	return 0;
}


unsigned int WINAPI MemoryPoolThread(LPVOID lpParam)
{
	st_TEST_DATA *p[AMOUNT_MAX];
	
	if (testMode == 0)
	{
		for (int i = 0; i < testCnt; i++)
		{
			PRO_BEGIN(L"MemoryPool");

			//PRO_BEGIN(L"Mem_ALLOC");
			for (int j = 0; j < testAmount; j++)
			{
				p[j] = memory->Alloc(false);
			}
			//PRO_END(L"Mem_ALLOC");

			//p[0]->data[0] = 1;
			PRO_BEGIN(L"MEM_DAT_SET");
			for (int j = 0; j < testAmount; j++)
			{
				for (int index = 0; index < DATA_SIZE; index++)
				{
					p[j]->data[index] = index % 0xff;
				}
			}
			PRO_END(L"MEM_DAT_SET");

			//PRO_BEGIN(L"Mem_FREE");
			for (int j = 0; j < testAmount; j++)
			{
				memory->Free(p[j]);
			}
			//PRO_END(L"Mem_FREE");

			PRO_END(L"MemoryPool");
		}
	}
	else if (testMode == 1)
	{
		for (int i = 0; i < testCnt; i++)
		{
			PRO_BEGIN(L"MemoryPool");			
			for (int j = 0; j < testAmount; j++)
			{
				//PRO_BEGIN(L"Mem_ALLOC");
				p[j] = memory->Alloc(false);
				//PRO_END(L"Mem_ALLOC");

				PRO_BEGIN(L"MEM_DAT_SET");
				for (int index = 0; index < DATA_SIZE; index++)
				{
					p[j]->data[index] = index % 0xff;
				}
				PRO_END(L"MEM_DAT_SET");

				//PRO_BEGIN(L"Mem_FREE");
				memory->Free(p[j]);
				//PRO_END(L"Mem_FREE");
				
			}
			PRO_END(L"MemoryPool");
		}
	}
	else if (testMode == 2)
	{
		int allocCnt=0;
		srand(time(NULL));
		int ran;
		
		for (int t = 0; t < testCnt; t++)
		{
			PRO_BEGIN(L"MemoryPool");
			while (allocCnt < testAmount)
			{
				ran = rand() % 30;
				
				for (int i = 0; i < ran; i++)
				{
					p[i] = memory->Alloc(false);

					allocCnt++;

					if (allocCnt >= testAmount)
					{
						ran = i;
						break;
					}
				}

				for (int i = 0; i < ran; i++)
				{
					PRO_BEGIN(L"MEM_DAT_SET");
					for (int index = 0; index < DATA_SIZE; index++)
					{
						p[i]->data[index] = index % 0xff;
					}
					PRO_END(L"MEM_DAT_SET");
				}

				for (int i = 0; i < ran; i++)
				{
					memory->Free(p[i]);
				}
				
			}
			PRO_END(L"MemoryPool");
		}
	}
	else if (testMode == 3)
	{
		for (int i = 0; i < testCnt; i++)
		{
			PRO_BEGIN(L"MemoryPool");
			for (int j = 0; j < testAmount/10; j++)
			{
				//PRO_BEGIN(L"Mem_ALLOC");
				for (int k = 0; k < 10; k++)
				{
					p[j*10 + k] = memory->Alloc(false);
				}
				//PRO_END(L"Mem_ALLOC");

				PRO_BEGIN(L"MEM_DAT_SET");
				for (int k = 0; k < 10; k++)
				{
					for (int index = 0; index < DATA_SIZE; index++)
					{
						p[j * 10 + k]->data[index] = index % 0xff;
					}
				}
				PRO_END(L"MEM_DAT_SET");

				//PRO_BEGIN(L"Mem_FREE");
				for (int k = 0; k < 10; k++)
				{
					memory->Free(p[j * 10 + k]);
				}
				//PRO_END(L"Mem_FREE");

			}
			PRO_END(L"MemoryPool");
		}
	}
	

	return 0;
}

unsigned int WINAPI MemoryPoolTLSThread(LPVOID lpParam)
{
	st_TEST_DATA *p[AMOUNT_MAX];
	if (testMode == 0)
	{
		for (int i = 0; i < testCnt; i++)
		{
			PRO_BEGIN(L"MemoryPoolTLS");

			//PRO_BEGIN(L"MemTLS_ALLOC");
			for (int j = 0; j < testAmount; j++)
			{
				p[j] = memoryTLS->Alloc();
			}
			//PRO_END(L"MemTLS_ALLOC");

			//p[0]->data[0] = 1;
			PRO_BEGIN(L"MEMTLS_DAT_SET");
			for (int j = 0; j < testAmount; j++)
			{
				for (int index = 0; index < DATA_SIZE; index++)
				{
					p[j]->data[index] = index % 0xff;
				}
			}
			PRO_END(L"MEMTLS_DAT_SET");

			//PRO_BEGIN(L"MemTLS_FREE");
			for (int j = 0; j < testAmount; j++)
			{
				memoryTLS->Free(p[j]);
			}
			//PRO_END(L"MemTLS_FREE");

			PRO_END(L"MemoryPoolTLS");
		}
	}
	else if (testMode == 1)
	{
		for (int i = 0; i < testCnt; i++)
		{
			PRO_BEGIN(L"MemoryPoolTLS");
			for (int j = 0; j < testAmount; j++)
			{
				//PRO_BEGIN(L"MemTLS_ALLOC");
				p[j] = memoryTLS->Alloc();
				//PRO_END(L"MemTLS_ALLOC");

				PRO_BEGIN(L"MEMTLS_DAT_SET");
				for (int index = 0; index < DATA_SIZE; index++)
				{
					p[j]->data[index] = index % 0xff;
				}
				PRO_END(L"MEMTLS_DAT_SET");

				//PRO_BEGIN(L"MemTLS_FREE");
				memoryTLS->Free(p[j]);
				//PRO_END(L"MemTLS_FREE");
			}
			PRO_END(L"MemoryPoolTLS");
		}
	}
	else if (testMode == 2)
	{
		int allocCnt = 0;
		srand(time(NULL));
		int ran;
		for (int t = 0; t < testCnt; t++)
		{
			PRO_BEGIN(L"MemoryPoolTLS");
			while (allocCnt < testAmount)
			{
				ran = rand() % 30;
				
				for (int i = 0; i < ran; i++)
				{
					p[i] = memoryTLS->Alloc();

					allocCnt++;

					if (allocCnt >= testAmount)
					{
						ran = i;
						break;
					}
				}

				for (int i = 0; i < ran; i++)
				{
					PRO_BEGIN(L"MEMTLS_DAT_SET");
					for (int index = 0; index < DATA_SIZE; index++)
					{
						p[i]->data[index] = index % 0xff;
					}
					PRO_END(L"MEMTLS_DAT_SET");
				}

				for (int i = 0; i < ran; i++)
				{
					memoryTLS->Free(p[i]);
				}
				
			}
			PRO_END(L"MemoryPoolTLS");
		}
	}
	else if (testMode == 3)
	{
		for (int i = 0; i < testCnt; i++)
		{
			PRO_BEGIN(L"MemoryPoolTLS");
			for (int j = 0; j < testAmount / 10; j++)
			{
				//PRO_BEGIN(L"Mem_ALLOC");
				for (int k = 0; k < 10; k++)
				{
					p[j * 10 + k] = memoryTLS->Alloc();
				}
				//PRO_END(L"Mem_ALLOC");

				PRO_BEGIN(L"MEMTLS_DAT_SET");
				for (int k = 0; k < 10; k++)
				{
					for (int index = 0; index < DATA_SIZE; index++)
					{
						p[j * 10 + k]->data[index] = index % 0xff;
					}
				}
				PRO_END(L"MEMTLS_DAT_SET");

				//PRO_BEGIN(L"Mem_FREE");
				for (int k = 0; k < 10; k++)
				{
					memoryTLS->Free(p[j * 10 + k]);
				}
				//PRO_END(L"Mem_FREE");

			}
			PRO_END(L"MemoryPoolTLS");
		}
	}
	
	

	return 0;
}

unsigned int WINAPI NewDeleteThread(LPVOID lpParam)
{
	st_TEST_DATA *p[AMOUNT_MAX];
	if (testMode == 0)
	{
		for (int i = 0; i < testCnt; i++)
		{
			PRO_BEGIN(L"NewDelete");

			//PRO_BEGIN(L"New");
			for (int j = 0; j < testAmount; j++)
			{
				p[j] = new st_TEST_DATA;
			}
			//PRO_END(L"New");

			//p[0]->data[0] = 1;
			PRO_BEGIN(L"NEWDEL_DAT_SET");
			for (int j = 0; j < testAmount; j++)
			{
				for (int index = 0; index < DATA_SIZE; index++)
				{
					p[j]->data[index] = index % 0xff;
				}
			}
			PRO_END(L"NEWDEL_DAT_SET");

			//PRO_BEGIN(L"Delete");
			for (int j = 0; j < testAmount; j++)
			{
				delete p[j];
			}
			//PRO_END(L"Delete");

			PRO_END(L"NewDelete");
		}
	}
	else if (testMode == 1)
	{
		for (int i = 0; i < testCnt; i++)
		{
			PRO_BEGIN(L"NewDelete");
			for (int j = 0; j < testAmount; j++)
			{
				//PRO_BEGIN(L"New");
				p[j] = new st_TEST_DATA;
				//PRO_END(L"New");

				PRO_BEGIN(L"NEWDEL_DAT_SET");
				for (int index = 0; index < DATA_SIZE; index++)
				{
					p[j]->data[index] = index % 0xff;
				}
				PRO_END(L"NEWDEL_DAT_SET");

				//PRO_BEGIN(L"Delete");
				delete p[j];
				//PRO_END(L"Delete");				
			}
			PRO_END(L"NewDelete");
		}
	}
	else if (testMode == 2)
	{
		int allocCnt = 0;
		srand(time(NULL));
		int ran;

		for (int t = 0; t < testCnt; t++)
		{
			PRO_BEGIN(L"NewDelete");
			while (allocCnt < testAmount)
			{
				ran = rand() % 30;
				
				for (int i = 0; i < ran; i++)
				{
					p[i] = new st_TEST_DATA;

					allocCnt++;

					if (allocCnt >= testAmount)
					{
						ran = i;
						break;
					}
				}

				for (int i = 0; i < ran; i++)
				{
					PRO_BEGIN(L"NEWDEL_DAT_SET");
					for (int index = 0; index < DATA_SIZE; index++)
					{
						p[i]->data[index] = index % 0xff;
					}
					PRO_END(L"NEWDEL_DAT_SET");
				}

				for (int i = 0; i < ran; i++)
				{
					delete p[i];
				}
				
			}
			PRO_END(L"NewDelete");
		}
	}
	else if (testMode == 3)
	{
		for (int i = 0; i < testCnt; i++)
		{
			PRO_BEGIN(L"NewDelete");
			for (int j = 0; j < testAmount / 10; j++)
			{
				//PRO_BEGIN(L"Mem_ALLOC");
				for (int k = 0; k < 10; k++)
				{
					p[j * 10 + k] = new st_TEST_DATA;
				}
				//PRO_END(L"Mem_ALLOC");

				PRO_BEGIN(L"NEWDEL_DAT_SET");
				for (int k = 0; k < 10; k++)
				{
					for (int index = 0; index < DATA_SIZE; index++)
					{
						p[j * 10 + k]->data[index] = index % 0xff;
					}
				}
				PRO_END(L"NEWDEL_DAT_SET");

				//PRO_BEGIN(L"Mem_FREE");
				for (int k = 0; k < 10; k++)
				{
					delete p[j * 10 + k];
				}
				//PRO_END(L"Mem_FREE");

			}
			PRO_END(L"NewDelete");
		}
	}
	

	return 0;
}


unsigned int WINAPI testThread(LPVOID lpParam)
{
	st_TEST_DATA *p[1000];
	MemoryPoolTLS<st_TEST_DATA> test;
	st_TEST_DATA *tt;
	while (1)
	{
		//for (int i = 0; i < 200; i++)
		//{
		//	tt = test.Alloc();
		//	memoryTLS->Free(tt);
		//	volatile int test = 1;
		//}
		for (int i = 0; i < 1000; i++)
		{
			p[i]=memoryTLS->Alloc();

			for (int index = 0; index < DATA_SIZE; index++)
			{
				p[i]->data[index] = index;
			}

			if (p[i] == NULL)
			{
				CrashDump::Crash();
			}
		}

		Sleep(100);

		for (int i = 0; i < 1000; i++)
		{
			if (!memoryTLS->Free(p[i]))
			{
				CrashDump::Crash();
			}
		}

		
	}
}