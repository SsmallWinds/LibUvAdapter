using Google.Protobuf.Examples.AddressBook;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;


namespace UvClient4CSharp
{
    public interface IUvClientCallBack
    {
        void OnConnection();
        void OnMsg(IntPtr msg);
        void OnError();
    }

    public class LibUvClient
    {
        private int _clientId = -1;
        private CppAdapter.QuoteCallBackDelegate _delegate = null;
        private IUvClientCallBack _callBack;

        public void Init(string ip, int port)
        {
            _delegate = new CppAdapter.QuoteCallBackDelegate(ClientCallBack);
            byte[] bip = System.Text.Encoding.UTF8.GetBytes(ip);
            _clientId = CppAdapter.CreateClient(ref bip[0], port, _delegate);
        }

        public void SetCallBack(IUvClientCallBack callBack)
        {
            _callBack = callBack;
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

        public void ClientCallBack(int clientId, int type, IntPtr msg, int size)
        {
            var msgType = (MsgType)type;
            Console.WriteLine(msgType + ":" + msg);

            if (msgType == MsgType.ON_CONNECTION)
            {
                _callBack?.OnConnection();
            }
            else if (msgType == MsgType.ON_MSG)
            {
                byte[] buf = new byte[size];
                Marshal.Copy(msg, buf, 0, size);

                var reqResult = SearchRequest.Parser.ParseFrom(buf);
                _callBack?.OnMsg(msg);
            }
            else if (msgType == MsgType.ON_ERROR)
            {
                _callBack?.OnError();
            }
        }
    }
}
