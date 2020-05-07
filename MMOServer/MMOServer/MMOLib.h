#pragma once

#include <sdkddkver.h>
#include <WinSock2.h>
#include <windows.h>
#include <WS2tcpip.h>
#include <process.h>
#include <fstream>
#include <cstring>
#include <string>
#include <map>
#include <unordered_map>
#include <stack>
#include <tchar.h>
#include <strsafe.h>
#include <queue>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32")

#include "header.h"
#include "CrashDump.h"
#include "MemoryPool.h"
#include "MemoryPoolTLS.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"
#include "Packet.h"
#include "PacketPtr.h"
#include "RingBuffer.h"
#include "TextParser.h"
#include "Profiler.h"
#include "Log.h"
#include "Session.h"
#include "MMOServer.h"