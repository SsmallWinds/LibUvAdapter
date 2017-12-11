#include "RJTcpClient.h"
#include <map>

#define _EXTERN_C_  extern "C"  _declspec(dllexport)


typedef void(_stdcall *ClientCallBack)(int clientId, int type, char* msg, int size);

struct RJClientStruct
{
	RJTcpClient client;
	ClientCallBack callback;
};

static std::map<int, RJClientStruct*> g_clients;
static int g_index = 0;

void OnClientCallBack(int clientId, int type, const char* msg, int size)
{
	auto iter = g_clients.begin();
	while (iter != g_clients.end())
	{
		if (iter->first == clientId)
		{
			if (iter->second->callback != nullptr)
			{
				iter->second->callback(clientId, type, (char*)msg, size);
				return;
			}
		}
	}
}

///����ֵС��0ʱ������ʧ�ܣ����򴴽��ɹ�������������ReleaseClient�ͷ���Դ
_EXTERN_C_ int _stdcall CreateClient(char* ip, int port, ClientCallBack callback)
{
	g_index += 1;
	int clientId = g_index;
	RJClientStruct* client = new RJClientStruct();
	client->client.m_client_id = clientId;
	client->client.m_callback = OnClientCallBack;
	client->callback = callback;

	int result = client->client.Connect(ip, port);
	if (result < 0)
	{
		delete client;
		return result;
	}

	g_clients.insert(std::pair<int, RJClientStruct*>(clientId, client));
	return clientId;
}

_EXTERN_C_ int ReleaseClient(int clientId)
{
	auto iter = g_clients.find(clientId);

	if (iter != g_clients.end())
	{
		delete iter->second;
		g_clients.erase(iter);
	}
	return 0;
}
_EXTERN_C_ int SendMsg(int clientId, char* msg, int size)
{
	auto iter = g_clients.begin();
	while (iter != g_clients.end())
	{
		if (iter->first == clientId)
		{
			if (iter->second->callback != nullptr)
			{
				iter->second->client.Send(msg, size);
				return 0;
			}
		}
	}
	return 0;
}