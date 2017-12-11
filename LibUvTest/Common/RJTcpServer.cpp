#include "RJTcpServer.h"

RJTcpServer::RJTcpServer()
{
}


RJTcpServer::~RJTcpServer()
{
}

//msgָ����ڴ�ֱ�ӿ�����new�����ڴ��У�msg�ɷ������Լ�ά��
void RJTcpServer::Send(uv_tcp_t* client, const char* msg, int size)
{
	write_req_t *req = new write_req_t();
	req->req.data = client;

	char* buf = new char[size + PACKAGE_HEAD_SIZE];
	Msg2Package(msg, size, buf);

	req->buf = uv_buf_init(buf, size + PACKAGE_HEAD_SIZE);
	int iret;
	iret = uv_write(&req->req, (uv_stream_t*)client, &req->buf, 1, AfterSend);
	if (iret)
	{
		cout << "AcceptConnection->uv_read_start:" << GetUVError(iret) << endl;
		uv_close((uv_handle_t*)client, ClientClose);
		RemoveClient(client);
		DeleteWriteReq((uv_write_t*)req);
	}
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
}

void RJTcpServer::OnNewConnection(uv_tcp_t* client)
{
	std::cout << "OnNewConnection:" << client << std::endl;
}

void RJTcpServer::RemoveClient(uv_tcp_t * client)
{
	m_clients_lock.lock();
	for (auto i = m_clients.begin(); i != m_clients.end(); i++)
	{
		if (&(*i)->client == client)
		{
			m_clients.erase(i);
			break;
		}
	}
	m_clients_lock.unlock();
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
		uv_close((uv_handle_t*)client, ClientClose);
		((RJTcpServer*)client->data)->RemoveClient((uv_tcp_t*)client);
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



void RJTcpServer::Init()
{
	sockaddr_in addr;
	m_pLoop = uv_default_loop();
	m_server.data = this;
	uv_tcp_init(m_pLoop, &m_server);
	uv_ip4_addr(DEFAULT_IP, DEFAULT_PORT, &addr);
	uv_tcp_bind(&m_server, (const sockaddr*)&addr, 0);

	int iret = uv_listen((uv_stream_t*)&m_server, BACK_LOG, AcceptConnection);
	if (iret)
	{
		cout << "Init->uv_listen:" << GetUVError(iret) << endl;
	}

	uv_run(m_pLoop, UV_RUN_DEFAULT);
}