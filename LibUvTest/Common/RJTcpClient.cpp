#include "RJTcpClient.h"
#include <iostream>

using namespace std;

RJTcpClient::RJTcpClient()
{
	m_is_connected = false;
	m_is_closing = false;
	m_loop = nullptr;
	m_reader.SetCallBack(std::bind(&RJTcpClient::OnMsg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
		, &m_uv_client);
}


RJTcpClient::~RJTcpClient()
{
	m_is_connected = false;

	m_send_lock.lock();
	while (!m_send_buf.empty())
	{
		delete m_send_buf.front().base;
		m_send_buf.pop();
	}
	m_send_lock.unlock();

	CloseAll();

	if (m_p_thread)
	{
		m_p_thread->join();
		delete m_p_thread;
	}
	std::cout << "release success!" << std::endl;
}

int RJTcpClient::Connect()
{
	m_loop = uv_default_loop();
	int iret;

	m_async_handle.data = this;
	iret = uv_async_init(m_loop, &m_async_handle, AsyncCallBack);
	if (iret)
	{
		cout << "Init -> uv_async_init:" << GetUVError(iret) << endl;
		return -1;
	}

	iret = uv_tcp_init(m_loop, &m_uv_client);
	if (iret)
	{
		cout << "Init -> uv_tcp_init:" << GetUVError(iret) << endl;
		return -2;
	}

	sockaddr_in addr;
	iret = uv_ip4_addr(DEFALUT_IP, DEFALUT_PORT, &addr);
	if (iret)
	{
		cout << "Init -> uv_ip4_addr:" << GetUVError(iret) << endl;
		return -3;
	}

	m_uv_client.data = this;
	iret = uv_tcp_connect(&m_connect, &m_uv_client, (const struct sockaddr*)&addr, HandleConnection);
	if (iret)
	{
		cout << "Init -> uv_tcp_connect:" << GetUVError(iret) << endl;
		return -4;
	}

	m_p_thread = new std::thread(&RJTcpClient::RunThread, this);
	return 0;
}

void RJTcpClient::Send(const char* msg, int size)
{
	if (!m_is_connected || msg == nullptr || size <= 0)return;

	char* buf = new char[size + PACKAGE_HEAD_SIZE];
	Msg2Package(msg, size, buf);

	uv_buf_t uv_buf;
	uv_buf.base = buf;
	uv_buf.len = size + PACKAGE_HEAD_SIZE;

	m_send_lock.lock();
	m_send_buf.push(uv_buf);
	m_send_lock.unlock();
	uv_async_send(&m_async_handle);
}

void RJTcpClient::Close()
{
	if (m_is_closing)
		return;
	m_is_closing = true;
	m_is_connected = false;
	uv_async_send(&m_async_handle);
}

void RJTcpClient::OnConnection()
{
	cout << "OnConnection--Success" << endl;
	if (m_callback != nullptr) {
		m_callback(m_client_id, ON_CONNECTION, nullptr, 0);
	}
}

void RJTcpClient::OnMsg(uv_tcp_t * client, const char * msg, int size)
{
	cout << "OnMsg:" << msg << endl;
	if (m_callback != nullptr) {
		m_callback(m_client_id, ON_MSG, msg, size);
	}
}

void RJTcpClient::OnError()
{
	if (m_callback != nullptr) {
		m_callback(m_client_id, ON_ERROR, nullptr, 0);
	}
}

void RJTcpClient::RunThread()
{
	uv_run(m_loop, UV_RUN_DEFAULT);
}

void RJTcpClient::CloseAll()
{
	if (m_loop)
		uv_walk(m_loop, WalkCallBack, this);
}

void RJTcpClient::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

void RJTcpClient::HandleConnection(uv_connect_t* connect, int status)
{
	if (status)
	{
		cout << "HandleConnection:" << GetUVError(status) << endl;
		((RJTcpClient*)connect->handle->data)->OnError();
		return;
	}
	((RJTcpClient*)connect->handle->data)->m_is_connected = true;

	int iret = uv_read_start(connect->handle, AllocBuffer, HandleMsg);
	if (iret)
	{
		cout << "uv_read_start:" << GetUVError(iret) << endl;
		((RJTcpClient*)connect->handle->data)->OnError();
	}
	else
	{
		((RJTcpClient*)connect->handle->data)->OnConnection();
	}
}

void RJTcpClient::HandleMsg(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf)
{
	if (nread > 0)
	{
		((RJTcpClient*)client->data)->m_reader.SetBuffer(buf->base, nread);
	}
	else
	{
		cout << "HandleMsg:" << GetUVError(nread) << endl;
		((RJTcpClient*)client->data)->OnError();
	}

	free(buf->base);
}

//调用 async_send 的回调
void RJTcpClient::AsyncCallBack(uv_async_t* handle)
{
	RJTcpClient* client = (RJTcpClient*)handle->data;

	if (client->m_is_closing)
	{
		client->CloseAll();
		return;
	}

	client->m_send_lock.lock();

	while (!client->m_send_buf.empty())
	{
		write_req_t *req = new write_req_t();
		req->req.data = client;
		req->buf = uv_buf_init(client->m_send_buf.front().base, client->m_send_buf.front().len);
		int iret;
		iret = uv_write(&req->req, (uv_stream_t*)&client->m_uv_client, &req->buf, 1, AfterSend);
		if (iret)
		{
			cout << "AsyncCallBack->uv_write:" << GetUVError(iret) << endl;
			client->OnError();
			DeleteWriteReq((uv_write_t*)req);
		}
		client->m_send_buf.pop();
	}
	client->m_send_lock.unlock();
}

void RJTcpClient::AfterSend(uv_write_t* req, int status)
{
	if (status)
	{
		cout << "AfrerSend:" << GetUVError(status) << endl;
		((RJTcpClient*)req->data)->OnError();
	}
	DeleteWriteReq(req);
}

void RJTcpClient::DeleteWriteReq(uv_write_t * write)
{
	write_req_t* req = (write_req_t*)write;
	delete req->buf.base;
	delete req;
}

void RJTcpClient::WalkCallBack(uv_handle_t* handle, void* arg)
{
	if (!uv_is_closing(handle)) {
		uv_close(handle, nullptr);
	}
}
