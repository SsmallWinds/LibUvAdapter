using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UvClient4CSharp
{
    public class LibUvClient
    {
        private int _clientId = -1;
        private CppAdapter.QuoteCallBackDelegate _delegate = null;

        public void Init()
        {
            _delegate = new CppAdapter.QuoteCallBackDelegate(ClientCallBack);
            byte[] bip = System.Text.Encoding.UTF8.GetBytes("127.0.0.1");
            _clientId = CppAdapter.CreateClient(ref bip[0], 7000, _delegate);
        }

        public void Send(string msg)
        {
            if (_clientId < 0) return;
            byte[] bmsg = System.Text.Encoding.UTF8.GetBytes(msg);
            CppAdapter.SendMsg(_clientId, ref bmsg[0], msg.Length);
        }

        public void Release()
        {
            if (_clientId < 0) return;
            CppAdapter.ReleaseClient(_clientId);
        }

        public void ClientCallBack(int clientId, int type, string msg, int size)
        {
            Console.WriteLine((MsgType)type + ":" + msg);
        }
    }
}
