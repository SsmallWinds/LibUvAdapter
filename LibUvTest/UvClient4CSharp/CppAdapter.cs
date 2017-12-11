using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace UvClient4CSharp
{
    public enum MsgType
    {
        ON_CONNECTION = 0,
        ON_MSG,
        ON_ERROR
    }

    public class CppAdapter
    {
        public delegate void QuoteCallBackDelegate(int clientId, int type, string msg, int size);

        public const string DllPath = "Common.dll";

        [DllImport(DllPath, EntryPoint = "CreateClient", CallingConvention = CallingConvention.StdCall)]
        public extern static int CreateClient(QuoteCallBackDelegate call);

        [DllImport(DllPath, EntryPoint = "ReleaseClient", CallingConvention = CallingConvention.Cdecl)]
        public extern static int ReleaseClient(int clientId);

        [DllImport(DllPath, EntryPoint = "SendMsg", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SendMsg(int clientId, ref byte msg, int size);

    }
}
