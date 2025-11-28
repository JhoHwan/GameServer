using System;
using System.Net.Sockets;
using System.Text;

namespace TestClient
{
    class Program
    {
        static async Task Main(string[] args)
        {
            Console.WriteLine("=== Test Client ===");

            TcpClient client = new TcpClient();

            try
            {
                Console.WriteLine("Connecting...");
                await client.ConnectAsync("127.0.0.1", 12345); // 서버 IP/포트
                Console.WriteLine("Connected!");

                NetworkStream stream = client.GetStream();

                // 수신 루프 별도 Task
                _ = Task.Run(() => RecvLoop(stream));

                while (true)
                {
                    Console.Write("보낼 텍스트: ");
                    string? input = Console.ReadLine();

                    if (string.IsNullOrEmpty(input))
                        continue;

                    if (input.Equals("exit", StringComparison.OrdinalIgnoreCase))
                    {
                        Console.WriteLine("클라이언트 종료");
                        break;
                    }

                    await SendPacket(stream, 1, Encoding.UTF8.GetBytes(input));
                }

                client.Close();
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error: " + ex);
            }
        }

        // ---------------------------
        // 패킷 전송
        // ---------------------------
        static async Task SendPacket(NetworkStream stream, ushort id, byte[] payload)
        {
            ushort size = (ushort)(4 + payload.Length); // 헤더 4바이트 + 바디

            byte[] header = new byte[4];
            BitConverter.TryWriteBytes(header.AsSpan(0, 2), size);
            BitConverter.TryWriteBytes(header.AsSpan(2, 2), id);

            byte[] sendBuffer = new byte[size];
            Buffer.BlockCopy(header, 0, sendBuffer, 0, 4);
            Buffer.BlockCopy(payload, 0, sendBuffer, 4, payload.Length);

            await stream.WriteAsync(sendBuffer, 0, sendBuffer.Length);

            Console.WriteLine($"[SEND] Id:{id}, Size:{size}, Payload:\"{Encoding.UTF8.GetString(payload)}\"");
        }

        // ---------------------------
        // 서버 메시지 수신
        // ---------------------------
        static async Task RecvLoop(NetworkStream stream)
        {
            byte[] buffer = new byte[4096];
            List<byte> recvBuffer = new List<byte>();

            try
            {
                while (true)
                {
                    int len = await stream.ReadAsync(buffer, 0, buffer.Length);
                    if (len == 0)
                    {
                        Console.WriteLine("서버 연결 종료");
                        return;
                    }

                    for (int i = 0; i < len; i++)
                        recvBuffer.Add(buffer[i]);

                    ProcessPackets(recvBuffer);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("RecvLoop error: " + ex.Message);
            }
        }

        // ---------------------------
        // 패킷 조립
        // ---------------------------
        static void ProcessPackets(List<byte> recvBuffer)
        {
            const int headerSize = 4;

            while (true)
            {
                if (recvBuffer.Count < headerSize)
                    return;

                ushort size = BitConverter.ToUInt16(recvBuffer.ToArray(), 0);

                if (recvBuffer.Count < size)
                    return;

                byte[] packet = recvBuffer.GetRange(0, size).ToArray();
                recvBuffer.RemoveRange(0, size);

                ushort id = BitConverter.ToUInt16(packet, 2);
                ReadOnlySpan<byte> payload = packet.AsSpan(4, size - 4);

                Console.WriteLine($"[RECV] Id:{id}, Size:{size}, Payload:\"{Encoding.UTF8.GetString(payload)}\"");
            }
        }
    }
}
