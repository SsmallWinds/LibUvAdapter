#include "RJTcpServer.h"

RJTcpServer::RJTcpServer()
{
	m_pLoop = nullptr;
	m_p_thread = nullptr;
	m_is_closing = false;
}


RJTcpServer::~RJTcpServer()
{
	m_send_buf_lock.lock();
	while (!m_send_buf.empty())
	{
		delete m_send_buf.front().buf.base;
		m_send_buf.pop();
	}
	m_send_buf_lock.unlock();

	Close();

	if (m_p_thread)
	{
		m_p_thread->join();
		delete m_p_thread;
	}

	if (m_pLoop)
		uv_loop_close(m_pLoop);

	m_clients_lock.lock();
	for (auto i = m_clients.begin(); i != m_clients.end(); i++)
	{
		delete *i;
	}
	m_clients_lock.unlock();
	std::cout << "server release success!" << std::endl;
}

//msg指向的内存直接拷贝到new出的内存中，msg由发送者自己维护
void RJTcpServer::Send(uv_tcp_t* client, const char* msg, int size)
{
	if (client == nullptr || msg == nullptr || size <= 0)return;

	char* buf = new char[size + PACKAGE_HEAD_SIZE];
	Msg2Package(msg, size, buf);

	uv_tcp_send_buf uv_buf;
	uv_buf.buf.base = buf;
	uv_buf.buf.len = size + PACKAGE_HEAD_SIZE;
	uv_buf.client = client;

	m_send_buf_lock.lock();
	m_send_buf.push(uv_buf);
	m_send_buf_lock.unlock();
	uv_async_send(&m_async_handle);
}

void RJTcpServer::Broadcast(const char * msg, int size)
{
	m_clients_lock.lock();
	for (auto client : m_clients)
	{
		Send(&client->client, msg, size);
	}
	m_clients_lock.unlock();
}

void RJTcpServer::OnMsg(uv_tcp_t* client, const char* msg, int size)
{
	std::cout << "Client-" << client << ':' << msg << std::endl;
	std::cout << msg + 16 << std::endl;
}

void RJTcpServer::OnNewConnection(uv_tcp_t* client)
{
	std::cout << "OnNewConnection:" << client << std::endl;
}

void RJTcpServer::OnDisconnection(uv_tcp_t* client)
{
	std::cout << "OnDisconnection:" << client << std::endl;
}


void RJTcpServer::RemoveClient(uv_tcp_t* client)
{
	std::lock_guard<std::mutex>lock(m_clients_lock);
	for (auto i = m_clients.begin(); i != m_clients.end(); i++)
	{
		if (&(*i)->client == client)
		{
			m_clients.erase(i);
			break;
		}
	}
	OnDisconnection(client);
}

bool RJTcpServer::CheckClient(uv_tcp_t * client)
{
	std::lock_guard<std::mutex>lock(m_clients_lock);
	for (auto i = m_clients.begin(); i != m_clients.end(); i++)
	{
		if (&(*i)->client == client)
		{
			return true;
		}
	}
	return false;
}

void RJTcpServer::DeleteWriteReq(uv_write_t * write)
{
	write_req_t* req = (write_req_t*)write;
	delete req->buf.base;
	delete req;
}

void RJTcpServer::AfterSend(uv_write_t* req, int status)
{
	if (status)
	{
		uv_tcp_t* client = (uv_tcp_t*)req->data;
		cout << "AfrerSend:" << GetUVError(status) << endl;
		uv_close((uv_handle_t*)client, ClientClose);
		((RJTcpServer*)client->data)->RemoveClient(client);
	}
	DeleteWriteReq(req);
}

void RJTcpServer::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

void RJTcpServer::HandleMsg(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf)
{
	if (nread > 0)
	{
		((uv_tcp_client*)client)->reader.SetBuffer(buf->base, nread);
	}
	else
	{
		cout << "HandleMsg:" << GetUVError(nread) << endl;
		((RJTcpServer*)client->data)->RemoveClient((uv_tcp_t*)client);
		uv_close((uv_handle_t*)client, ClientClose);
	}

	free(buf->base);
}

void RJTcpServer::ClientClose(uv_handle_t* handle)
{
	uv_tcp_client* pClient = (uv_tcp_client*)handle;
	delete pClient;
}

void RJTcpServer::AcceptConnection(uv_stream_t* server, int status)
{
	if (status)
	{
		cout << "AcceptConnection:" << GetUVError(status) << endl;
		return;
	}

	RJTcpServer* rj_server = (RJTcpServer*)server->data;

	int iret;
	uv_tcp_client* client = new uv_tcp_client();
	client->reader.SetCallBack(std::bind(&RJTcpServer::OnMsg, rj_server, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
		, &client->client);
	client->client.data = rj_server;

	iret = uv_tcp_init(rj_server->m_pLoop, &client->client);
	if (iret)
	{
		cout << "AcceptConnection->uv_tcp_init:" << GetUVError(iret) << endl;
		delete client;
		return;
	}

	iret = uv_accept(server, (uv_stream_t*)client);
	if (iret)
	{
		cout << "AcceptConnection->uv_accept:" << GetUVError(iret) << endl;
		uv_close((uv_handle_t*)client, ClientClose);
		return;
	}

	iret = uv_read_start((uv_stream_t*)client, AllocBuffer, HandleMsg);
	if (iret)
	{
		cout << "AcceptConnection->uv_read_start:" << GetUVError(iret) << endl;
		uv_close((uv_handle_t*)client, ClientClose);
		return;
	}
	rj_server->m_clients_lock.lock();
	rj_server->m_clients.push_back(client);
	rj_server->m_clients_lock.unlock();
	rj_server->OnNewConnection(&client->client);
}

void RJTcpServer::AsyncCallBack(uv_async_t * handle)
{
	RJTcpServer* server = (RJTcpServer*)handle->data;

	//遍历当前的连接并关闭
	if (server->m_is_closing)
	{
		uv_walk(server->m_pLoop, WalkCallBack, server);
		return;
	}

	server->m_send_buf_lock.lock();

	while (!server->m_send_buf.empty())
	{
		uv_tcp_send_buf& buf = server->m_send_buf.front();

		if (!server->CheckClient(buf.client))
		{
			delete buf.buf.base;
		}
		else
		{
			write_req_t *req = new write_req_t();
			req->buf = uv_buf_init(buf.buf.base, buf.buf.len);
			int iret;
			iret = uv_write(&req->req, (uv_stream_t*)buf.client, &req->buf, 1, AfterSend);
			if (iret)
			{
				cout << "AsyncCallBack->uv_write:" << GetUVError(iret) << endl;
				server->RemoveClient(buf.client);
				uv_close((uv_handle_t*)buf.client, ClientClose);
				DeleteWriteReq((uv_write_t*)req);
			}
		}
		server->m_send_buf.pop();
	}
	server->m_send_buf_lock.unlock();
}

void RJTcpServer::WalkCallBack(uv_handle_t* handle, void* arg)
{
	if (!uv_is_closing(handle)) {
		uv_close(handle, nullptr);
	}
}

///关闭所有监听器，包括监听的所有TcpClient
void RJTcpServer::Close()
{
	if (m_is_closing)
		return;

	m_is_closing = true;

	if (m_pLoop)
	{
		uv_async_send(&m_async_handle);
	}
}

int RJTcpServer::Init(int port)
{
	int iret;
	sockaddr_in addr;
	m_pLoop = uv_default_loop();

	m_async_handle.data = this;
	iret = uv_async_init(m_pLoop, &m_async_handle, AsyncCallBack);
	if (iret)
	{
		cout << "Init -> uv_async_init:" << GetUVError(iret) << endl;
		return -1;
	}
	m_server.data = this;
	iret = uv_tcp_init(m_pLoop, &m_server);
	if (iret)
	{
		cout << "Init->uv_listen:" << GetUVError(iret) << endl;
		return -2;
	}
	iret = uv_ip4_addr(DEFAULT_IP, port, &addr);
	if (iret)
	{
		cout << "Init->uv_listen:" << GetUVError(iret) << endl;
		return -3;
	}
	iret = uv_tcp_bind(&m_server, (const sockaddr*)&addr, 0);
	if (iret)
	{
		cout << "Init->uv_listen:" << GetUVError(iret) << endl;
		return -4;
	}
	iret = uv_listen((uv_stream_t*)&m_server, BACK_LOG, AcceptConnection);
	if (iret)
	{
		cout << "Init->uv_listen:" << GetUVError(iret) << endl;
		return -5;
	}

	m_p_thread = new thread(&RJTcpServer::RunThread, this);
	return 0;
}

void RJTcpServer::RunThread()
{
	uv_run(m_pLoop, UV_RUN_DEFAULT);
}
