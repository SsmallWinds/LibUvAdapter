#pragma once

#include <string>
#include "uv.h"
#include <queue>
#include <mutex>
#include <thread>
#include <iostream>
#include <functional>

#ifdef WIN32
#include<Winsock2.h>
#pragma comment(lib,"Ws2_32.lib")
#else
#include <arpa/inet.h>
#endif // WIN32


#define _EXPORT_ _declspec(dllexport)

#define ON_CONNECTION 0
#define ON_MSG 1
#define ON_ERROR 2

#define BufferReadCallBack std::function<void(uv_tcp_t*,const char*, int)>
#define RJClientCallBack std::function<void(int,int,const char*,int)>

typedef struct
{
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;

#pragma pack(push,1)//将当前字节对齐值设为1
typedef struct
{
	unsigned char proto_type[2] = { 'u','v' };
	uint32_t size;
}PACKAGE_HEAD;
#pragma pack(pop)

#define PACKAGE_HEAD_SIZE sizeof(PACKAGE_HEAD)

//
inline void Msg2Package(const char* msg, int size, char* buf)
{
	PACKAGE_HEAD head;
	head.size = htonl(size);//local 2 net
	memcpy(buf, &head, PACKAGE_HEAD_SIZE);
	memcpy(buf + PACKAGE_HEAD_SIZE, msg, size);
}


inline std::string GetUVError(int errcode)
{
	if (0 == errcode)
	{
		return "";
	}
	std::string err;
	auto tmpChar = uv_err_name(errcode);
	if (tmpChar)
	{
		err = tmpChar;
		err += ":";
	}
	else
	{
		err = "unknown system errcode " + std::to_string((long long)errcode);
		err += ":";
	}
	tmpChar = uv_strerror(errcode);
	if (tmpChar)
	{
		err += tmpChar;
	}
	return std::move(err);
}

class BufferReader
{
public:
	BufferReader()
	{
		m_index = 0;
		m_current_msg_size = 0;
	}

	~BufferReader()
	{
	};

private:
	BufferReadCallBack m_call_back;
	uv_tcp_t* m_p_tcp;
	int m_current_msg_size;
	int m_index;
	char m_msg_buffer[1024 * 1024]; /// TODO buffer长度不够动态分配

private:
	void OnMsg(const char* msg, int size)
	{
		if (m_call_back != nullptr)
			m_call_back(m_p_tcp, msg, size);
	}

public:

	void SetCallBack(BufferReadCallBack callback, uv_tcp_t* p_tcp)
	{
		m_call_back = callback;
		m_p_tcp = p_tcp;
	}

	void SetBuffer(const char* msg, int size)
	{
		memcpy(m_msg_buffer + m_index, msg, size);
		m_index += size;
		while (m_index >= PACKAGE_HEAD_SIZE)
		{
			int msg_size;
			memcpy(&msg_size, m_msg_buffer + 2, sizeof(int));//指针前两个字节是认证码
			m_current_msg_size = ntohl(msg_size); //net2local

			if (m_msg_buffer[0] != 'u' || m_msg_buffer[1] != 'v')
			{
				std::cout << "UNPACKAGE ERROR!!" << std::endl; ///TODO error dispose
			}

			if (m_index >= PACKAGE_HEAD_SIZE + m_current_msg_size)
			{
				OnMsg(m_msg_buffer + PACKAGE_HEAD_SIZE, m_current_msg_size);
				memcpy(m_msg_buffer, m_msg_buffer + PACKAGE_HEAD_SIZE + m_current_msg_size, m_index - PACKAGE_HEAD_SIZE - m_current_msg_size);
				m_index = m_index - PACKAGE_HEAD_SIZE - m_current_msg_size;
			}
			else
			{
				break;
			}
		}
	}
};
