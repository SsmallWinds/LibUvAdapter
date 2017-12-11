using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UvClient4CSharp
{
    public class LibUvClientAdapter : IUvClientCallBack
    {
        private LibUvClient _client;

        public LibUvClientAdapter()
        {
            _client = new LibUvClient();
            _client.SetCallBack(this);
        }

        public void OnConnection()
        {

        }

        public void OnError()
        {

        }

        public void OnMsg(IntPtr msg)
        {

        }
    }
}
