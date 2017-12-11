using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace UvClient4CSharp
{
    class Program
    {
        static void Main(string[] args)
        {
            LibUvClient client = new LibUvClient();
            client.Init();

            Thread.Sleep(1000);

            client.Send("Hello");

            Thread.Sleep(1000);

           client.Release();

            Console.ReadLine();
        }
    }
}
