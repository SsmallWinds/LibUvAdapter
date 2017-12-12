using Google.Protobuf.Examples.AddressBook;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UvClient4CSharp
{
    public interface ILibUvClientAdapterCallBack
    {
        void OnConnection();
        void OnMsg(string cmd, byte[] value);
        void OnError();
    }

    public class LibUvClientAdapter : IUvClientCallBack
    {
        private LibUvClient _client;
        private ILibUvClientAdapterCallBack _callBack;

        public LibUvClientAdapter()
        {
            _client = new LibUvClient();
            _client.SetCallBack(this);
        }

        public void Init(string ip, int port)
        {
            _client.Init(ip, port);
        }

        //发送的字节数组，前16个字节是命令
        public void Send(string cmd, byte[] value)
        {
            byte[] cmdBytes = Encoding.Default.GetBytes(cmd);
            byte[] msg = new byte[16 + value.Length];
            Array.Copy(cmdBytes, 0, msg, 0, cmdBytes.Length);
            Array.Copy(value, 0, msg, 16, value.Length);
            _client.Send(msg);
        }

        public void SetCallBack(ILibUvClientAdapterCallBack callBack)
        {
            _callBack = callBack;
        }

        public void OnConnection()
        {
            _callBack?.OnConnection();
        }

        public void OnError()
        {
            _callBack?.OnError();
        }

        //接收到的是数据，前16个字节是命令,后面的字节是实际的内容
        public void OnMsg(byte[] msg)
        {
            try
            {
                var sourceCmd = Encoding.Default.GetString(msg, 0, 16);
                var cmd = sourceCmd.Substring(0, sourceCmd.IndexOf('\0'));

                byte[] value = new byte[msg.Length - 16];
                Array.Copy(msg, 16, value, 0, msg.Length - 16);

                _callBack?.OnMsg(cmd, value);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }
    }
}
