using System.Net;
using System.Net.Sockets;

namespace Game.TcpDBServer.Network;

class Server
{
    private readonly Listener _listener;
    private HashSet<Session> _sessions = new();
    private readonly object _lock = new();

    CancellationTokenSource _cts;

    public Server(IPAddress iPAddress, int port, CancellationTokenSource cts)
    {
        _listener = new Listener(iPAddress, port);
        _listener.OnAccept += HandleSessionAccepted;
        _cts = cts;
    }

    public void Start()
    {
        _ = _listener.StartListenAsync(_cts.Token);
    }

    private void HandleSessionAccepted(TcpClient client)
    {
        Session session = new Session(client);
        Console.WriteLine("Server Connected");
        session.OnRecv += HandleRecvPacket;
        session.OnSend += bytesSent => Console.WriteLine("Sent Bytes: " + bytesSent);
        session.OnDisconnect += () =>
        {
            Console.WriteLine("Client Disconnected");
            lock (_lock) _sessions.Remove(session);
        };

        lock (_lock) _sessions.Add(session);

        _ = session.RecvAsync(_cts.Token);
    }

    private void HandleRecvPacket(ushort id, ReadOnlyMemory<byte> packet)
    {
        Console.WriteLine("Recv Bytes: " + packet.Length);
    }

    public void Stop()
    {
        _cts.Cancel();

        _listener.Stop();

        lock (_lock)
        {
            foreach (var session in _sessions)
            {
                session.DisConnect();
            }
            _sessions.Clear();
        }
    }
}