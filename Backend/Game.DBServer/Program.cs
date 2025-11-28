using Game.Data;
using Game.TcpDBServer.Network;
using Microsoft.EntityFrameworkCore;
using System;
using System.Collections.Generic;
using System.Text;

namespace DBServer
{
    public class Program
    {
        public static async Task Main(string[] args)
        {
        
            Console.WriteLine("=== DB Server ===");
            CancellationTokenSource cts = new CancellationTokenSource();
            Server server = new Server(System.Net.IPAddress.Loopback, 12345, cts);
            server.Start();

            Console.WriteLine("Press Enter to stop.");
            Console.ReadLine();
            server.Stop();

            Console.WriteLine("Server stopped.");
        }
    }
}
