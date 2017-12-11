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
            client.Init("127.0.0.1",7000);

            Thread.Sleep(1000);

            client.Send("Hello");

            Thread.Sleep(1000);

            Console.ReadLine();
        }
    }
}
