using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;

namespace Game.TcpDBServer.Network;

[Serializable]
[StructLayout(LayoutKind.Sequential, Pack = 1)]
struct PacketHeader
{
    public const int HeaderSize = 4;

    public ushort Size;
    public ushort Id;

    PacketHeader(ushort size, ushort id)
    {
        Size = size;
        Id = id;
    }

    public static PacketHeader BytesToHeader(byte[] data)
    {

        ushort size = BitConverter.ToUInt16(data, 0);
        ushort id   = BitConverter.ToUInt16(data, 2);

        return new PacketHeader(size, id);
    }

    public static byte[] HeaderToBytes(PacketHeader header)
    {
        byte[] arr = new byte[4];
        BitConverter.TryWriteBytes(arr.AsSpan(0, 2), header.Size);
        BitConverter.TryWriteBytes(arr.AsSpan(2, 2), header.Id);
        return arr;
    }
}

public class Session
{
    readonly private TcpClient _client;
    public event Action<ushort, ReadOnlyMemory<byte>>? OnRecv;
    public event Action<int>? OnSend;
    public event Action? OnDisconnect;

    private List<byte> _recvBuffer = new List<byte>();

    public Session(TcpClient client)
    {
        _client = client;
    }

    public async Task RecvAsync(CancellationToken cancellationToken)
    {
        NetworkStream stream = _client.GetStream();
        byte[] buffer = new byte[1024];
        int bytesRead = 0;

        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length, cancellationToken);
            }
            catch (Exception ex)
            {
                HandleError(ex);
                break;
            }

            if (bytesRead == 0)
            {
                DisConnect();
                break;
            }

            for (int i = 0; i < bytesRead; i++)
            {
                _recvBuffer.Add(buffer[i]);
            }

            CreatePacket();
        }
    }

    private void CreatePacket()
    {
        int headerSize = PacketHeader.HeaderSize;

        while (true)
        {
            if (_recvBuffer.Count < headerSize)
                break;

            byte[] headerBytes = _recvBuffer.GetRange(0, headerSize).ToArray();
            PacketHeader packetHeader = PacketHeader.BytesToHeader(headerBytes);
            if (_recvBuffer.Count < packetHeader.Size)
                break;

            byte[] packetBytes = _recvBuffer.GetRange(0, packetHeader.Size).ToArray();
            _recvBuffer.RemoveRange(0, packetHeader.Size);

            OnRecv?.Invoke(packetHeader.Size, packetBytes);
        }
    }

    public async Task SendAsync(byte[] sendBuffer, CancellationToken cancellationToken)
    {
        NetworkStream stream = _client.GetStream();

        try
        {
            await stream.WriteAsync(sendBuffer, 0, sendBuffer.Length, cancellationToken);
            OnSend?.Invoke(sendBuffer.Length);
        }
        catch (Exception ex) when (ex is OperationCanceledException || ex is IOException)
        {
            _client.Close();
            Console.WriteLine("Failed to send message. Client may be disconnected.");
        }
    }

    public void DisConnect()
    {
        OnDisconnect?.Invoke();
        _client.Close();
    }

    private void HandleError(Exception ex)
    {
        switch (ex)
        {
            // 1) 강제 종료(ConnectionReset) — 너무 흔하므로 가볍게 처리
            case IOException ioEx when ioEx.InnerException is SocketException se &&
                                        se.SocketErrorCode == SocketError.ConnectionReset:
                Console.WriteLine("Client forcefully closed connection (10054 ConnectionReset).");
                DisConnect();
                break;

            // 2) 서버 Shutdown or Cancellation
            case OperationCanceledException:
                Console.WriteLine("Recv canceled.");
                DisConnect();
                break;

            // 3) 기타 네트워크 에러
            case IOException ioEx:
                Console.WriteLine($"IO error: {ioEx.Message}");
                DisConnect();
                break;

            // 4) 예기치 않은 에러
            default:
                Console.WriteLine($"Unexpected error: {ex}");
                DisConnect();
                break;
        }
    }
}

