#pragma once

enum TYPE { SEND = 0, RECV };

struct MyOverlapped
{
	OVERLAPPED overlap;
	TYPE type;
};

class Session
{
public:
	Session(SOCKET s, SOCKADDR_IN &sAddr,DWORD ID);
	Session();

	void SetSessionInfo(SOCKET s, SOCKADDR_IN &sAddr, DWORD ID);
	~Session();

	DWORD &GetIOCount() { return IOCount; }
	RingBuffer &GetRecvQ() { return recvQ; }
	RingBuffer &GetSendQ() { return sendQ; }
	CHAR &GetSendFlag() { return sendFlag; }
	SOCKET GetSocket() { return sock; }
	BOOL &GetSocketActive() { return sockActive; }
	DWORD GetID() { return sessionID; }
	MyOverlapped &GetSendOverlap() { return sendOverlap; }
	MyOverlapped &GetRecvOverlap() { return recvOverlap; }

	BOOL RecvPost();
	BOOL SendPost();
	BOOL Release();
	void Lock();
	void Unlock();

	void SetSendPacketCnt(int cnt) { sendPacketCnt = cnt; }
	int GetSendPacketCnt() { return sendPacketCnt; }

private:
	SOCKET sock;
	DWORD sessionID;
	SOCKADDR_IN sockAddr;
	MyOverlapped sendOverlap;
	MyOverlapped recvOverlap;
	RingBuffer sendQ;
	RingBuffer recvQ;
	DWORD IOCount;
	CHAR sendFlag;
	BOOL sockActive;
	SRWLOCK sessionLock;
	int sendPacketCnt;

	//자동화 테스트용
//public:
//	RingBuffer &GetAutoSendQ() { return autoSendQ; }
//private:
//	RingBuffer autoSendQ;

};