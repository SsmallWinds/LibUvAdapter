using System;
using System.Runtime.InteropServices;

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
        public delegate void QuoteCallBackDelegate(int clientId, int type, IntPtr msg, int size);

        public const string DllPath = "Common.dll";

        [DllImport(DllPath, EntryPoint = "CreateClient", CallingConvention = CallingConvention.StdCall)]
        public extern static int CreateClient(ref byte ip, int port, QuoteCallBackDelegate call);

        [DllImport(DllPath, EntryPoint = "ReleaseClient", CallingConvention = CallingConvention.Cdecl)]
        public extern static int ReleaseClient(int clientId);

        [DllImport(DllPath, EntryPoint = "SendMsg", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SendMsg(int clientId, ref byte msg, int size);

    }
}
