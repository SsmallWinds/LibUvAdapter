// LibUvTest.cpp : 定义控制台应用程序的入口点。
//



#include "stdafx.h"
#include "uv.h"
#include <thread>
#include <iostream>
#include "RJTcpServer.h"
#include <Windows.h>
#include "SearchRequest.pb.h"


using namespace std;

#define DEFAULT_PORT 7000
#define DEFAULT_BACKLOG 128

uv_loop_t *loop;
sockaddr_in addr;

int64_t counter = 0;
void wait_for_a_while(uv_idle_t* handle)
{
	counter++;
	cout << "counter++;" << counter << endl;
	if (counter >= 10)
	{
		printf("uv_idle_stop(handle);\n");
		uv_idle_stop(handle);
	}
}

void test1()
{
	uv_loop_t *loop = uv_loop_new();

	printf("quit1...");

	uv_run(loop, UV_RUN_DEFAULT);

	printf("quit2...");
	getchar();
}

void test2()
{
	uv_idle_t idler;
	uv_idle_init(uv_default_loop(), &idler);
	cout << "uv_idle_start" << endl;
	uv_idle_start(&idler, wait_for_a_while);

	printf("Idling...\n");
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void free_write_req(uv_write_t *req) {
	write_req_t *wr = (write_req_t*)req;
	//free(wr->buf.base);
	free(wr);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

void echo_write(uv_write_t *req, int status) {
	if (status) {
		fprintf(stderr, "Write error %s\n", uv_strerror(status));
	}

	free_write_req(req);
}
int no = 0;

void ClientClose(uv_handle_t* handle)
{
	uv_tcp_t* pClient = (uv_tcp_t*)handle;
	delete pClient;
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
	if (nread > 0) {
		std::cout << "read:" << buf->base << std::endl;
		write_req_t *req = (write_req_t*)malloc(sizeof(write_req_t));

		char msg[31];
		sprintf_s(msg, "Had read the value [%d] send!", no);
		req->buf = uv_buf_init(msg, sizeof(msg));
		int wr = uv_write((uv_write_t*)req, client, &req->buf, 1, echo_write);

		if (wr != 0)
		{
			uv_close((uv_handle_t*)client, ClientClose);
			free((uv_tcp_t*)client);
		}

		free(buf->base);
		return;
	}
	if (nread < 0) {
		if (nread != UV_EOF)
			fprintf(stderr, "Read error %s\n", uv_err_name(nread));
		uv_close((uv_handle_t*)client, NULL);
		free((uv_tcp_t*)client);
	}

	free(buf->base);
}


void on_new_connection(uv_stream_t *server, int status) {
	if (status < 0) {
		fprintf(stderr, "New connection error %s\n", uv_strerror(status));
		// error!
		return;
	}

	cout << "on_new_connection" << server->data << endl;

	uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);
	if (uv_accept(server, (uv_stream_t*)client) == 0) {
		no++;
		uv_read_start((uv_stream_t*)client, alloc_buffer, echo_read);
	}
	else {
		uv_close((uv_handle_t*)client, NULL);
	}
}


int test3()
{
	loop = uv_default_loop();

	uv_tcp_t server;
	uv_tcp_init(loop, &server);

	uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &addr);

	uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
	int r = uv_listen((uv_stream_t*)&server, DEFAULT_BACKLOG, on_new_connection);
	if (r) {
		fprintf(stderr, "Listen error %s\n", uv_strerror(r));
		return 1;
	}
	return uv_run(loop, UV_RUN_DEFAULT);
}

void idle_cb(uv_idle_t *handle)
{
	printf("Idle callback\n");
	counter++;

	if (counter >= 5)
	{
		uv_stop(uv_default_loop());
		printf("uv_stop() called\n");
	}
}

void prep_cb(uv_prepare_t *handle)
{
	printf("Prep callback\n");
}

void test4()
{
	uv_idle_t idler;
	uv_prepare_t prep;

	uv_idle_init(uv_default_loop(), &idler);
	uv_idle_start(&idler, idle_cb);

	//一次循环中其他任务完成后执行
	uv_prepare_init(uv_default_loop(), &prep);
	uv_prepare_start(&prep, prep_cb);

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void AsyncCallBack(uv_async_t* handle)
{
	cout << "AsyncCallBack..." << endl;
}

uv_async_t m_async_handle;

void Test()
{
	while (true)
	{
		Sleep(1000);
		cout << "Test..." << endl;
		char* buf = new char[128];
		m_async_handle.data = buf;
		uv_async_send(&m_async_handle);
		delete buf;
		cout << "After ..." << endl;
	}
};

class Server :IRJTcpServerCallBack
{
public:
	 Server()
	{
		m_server = new RJTcpServer();
		m_server->m_callback = this;
		int result = m_server->Init(7000);
		if (result)
		{
			cout << "server creat error:" << result << endl;
		}
	};

	 ~Server()
	{
		delete m_server;
	};

public:
	RJTcpServer* m_server;

	void OnMsg(uv_tcp_t* client, const char* msg, int size)
	{

	};
	void OnNewConnection(uv_tcp_t* client)
	{
		SearchRequest req;

		req.set_query("test_query");
		req.set_page_number(2);
		req.set_result_per_page(3);
		req.set_corpus(::Corpus::NEWS);

		int lenth = req.ByteSize();
		char* buf = new char[16 + lenth];

		memcpy(buf, "testtest", 16);

		req.SerializePartialToArray(buf + 16, lenth);
		m_server->Send(client, buf, 16 + lenth);

	};
	void OnDisconnection(uv_tcp_t* client)
	{

	};
};

int main()
{
	//loop = uv_default_loop();
	//uv_async_init(loop, &m_async_handle, AsyncCallBack);

	//std::thread thread(&Test);

	//uv_run(loop, UV_RUN_DEFAULT);

	//printf("getchar\n");

	/*RJTcpServer server;
	server.Init(7000);

	Sleep(5000);



	SearchRequest req;

	req.set_query("test_query");
	req.set_page_number(2);
	req.set_result_per_page(3);
	req.set_corpus(::Corpus::NEWS);

	int lenth = req.ByteSize();
	char* buf = new char[16 + lenth];

	memcpy(buf, "testtest", 16);

	req.SerializePartialToArray(buf + 16, lenth);

	std::cout << 16 + lenth << endl;

	server.Broadcast(buf, 16 + lenth);

	delete[] buf;*/

	Server server;
	getchar();
	return 0;
}



