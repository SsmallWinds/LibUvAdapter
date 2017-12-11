#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include "LibUvHelper.h"

using namespace std;

#define DEFAULT_IP "0.0.0.0"
#define BACK_LOG 128

typedef struct
{
	uv_tcp_t client;
	BufferReader reader;
}uv_tcp_client;

class RJTcpServer
{
public:
	_EXPORT_ RJTcpServer();
	_EXPORT_ ~RJTcpServer();

private:
	vector<uv_tcp_client*> m_clients;
	mutex m_clients_lock;
	bool m_is_closing;
	uv_loop_t* m_pLoop;
	uv_tcp_t m_server;
	uv_async_t m_async_handle;
	thread* m_p_thread;

public:
	_EXPORT_ void Close();
	_EXPORT_ int Init(int port);
	_EXPORT_ void Send(uv_tcp_t* client, const char* msg, int size);
	_EXPORT_ void Broadcast(const char* msg, int size);

private:
	void OnMsg(uv_tcp_t* client, const char* msg, int size);
	void OnNewConnection(uv_tcp_t* client);


private:
	void RunThread();
	void RemoveClient(uv_tcp_t* client);
	static void DeleteWriteReq(uv_write_t* write);
	static void AfterSend(uv_write_t* req, int status);
	static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void HandleMsg(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
	static void ClientClose(uv_handle_t* handle);
	static void AcceptConnection(uv_stream_t* server, int status);
	static void AsyncCallBack(uv_async_t* handle);
	static void WalkCallBack(uv_handle_t* handle, void* arg);
};

