#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <thread>
#include "RJTcpClient.h"

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 7000

struct sockaddr_in dest;
uv_timer_t*  timer;
uv_loop_t* loop;
uv_write_t write_req;

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
	//TODO :::: free buf 
	if (nread > 0)
	{
		std::cout << "read:" << buf->base << std::endl;
	}
	else
	{

	}
}

void write_cb(uv_write_t* req, int status)
{
	if (status < 0) {
		printf("%s\n", uv_strerror(status));
		return;
	}
	//free(req);
}

void timer_cb(uv_timer_t* timer)
{
	uv_buf_t buff = uv_buf_init("hellohappynihaowoshi", 21);

	uv_connect_t* connect = (uv_connect_t*)timer->data;
	uv_stream_t* tcp = connect->handle;

	int buf_count = 1;

	uv_write(&write_req, tcp, &buff, buf_count, write_cb);
}

void on_connect(uv_connect_t* connect, int status)
{
	if (status < 0) {
		printf("connect error: %s!\n", uv_strerror(status));
		return;
	}
	printf("connect success!");

	uv_read_start(connect->handle, alloc_buffer, echo_read);

	timer = (uv_timer_t*)calloc(sizeof(uv_timer_t), 1);
	timer->data = connect;
	uv_timer_init(loop, timer);

	uv_timer_start(timer, timer_cb, 0, 300);

}

void RunTest() {

	Sleep(2000);

	uv_buf_t buff = uv_buf_init("RunTest", 21);

	uv_connect_t* connect = (uv_connect_t*)timer->data;
	uv_stream_t* tcp = connect->handle;

	int buf_count = 1;
	uv_write(&write_req, tcp, &buff, buf_count, write_cb);
}

int main(int argc, char **argv) {


	//char x[10]{ '1','2','3','4','5','6','7','8','\0' };
	//
	//for (int i = 0; i < 10; i++) {
	//	std::cout << x[i] << std::endl;
	//}

	//memcpy(x, x + 3, 4);

	//for (int i = 0; i < 10; i++) {
	//	std::cout << x[i] << std::endl;
	//}

	//getchar();

	/*loop = uv_default_loop();
	uv_tcp_t  socket;
	uv_tcp_init(loop, &socket);

	uv_connect_t connect;

	uv_ip4_addr(DEFAULT_IP, DEFAULT_PORT, &dest);
	int ret = uv_tcp_connect(&connect, &socket, (const struct sockaddr*)&dest, on_connect);
	if (ret) {
		fprintf(stderr, "Connect error: %s\n", uv_strerror(ret));
		return 0;
	}*/

	std::cout << sizeof(RJTcpClient) <<std::endl;

	RJTcpClient client;

	//std::thread thread(std::bind(&RJTcpClient::Init, &client));

	client.Init();
	Sleep(1000);

	std::string test;

	for (int j = 0; j < 20; j++)
	{
		test.append("xyx--");
	}

	for (int i = 0; i < 10; i++)
	{
		std::string s = std::to_string(i);
		s.append(test);
		client.Send(s.c_str(), s.size() + 1);
	}

	/*while (true)
	{

		for (int i = 0; i < 10; i++)
		{
			std::string s = std::to_string(i);
			s.append(test);
			client.Send(s.c_str(), s.size() + 1);
		}

		Sleep(1000);
	}*/

	getchar();
	//std::thread th(&RunTest);

	uv_run(loop, UV_RUN_DEFAULT);
	return 0;
}