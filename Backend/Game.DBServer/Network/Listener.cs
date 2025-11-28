using System;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace Game.TcpDBServer.Network;

public class Listener
{
    private readonly TcpListener _listener;
    private bool _isRunning = false;
    
    public event Action<TcpClient>? OnAccept;

    public Listener(IPAddress iPAddress, int port)
    {
        _listener = new TcpListener(iPAddress, port);
    }

    public async Task StartListenAsync(CancellationToken cancellationToken = default)
    {
        if (_isRunning)
        {
            throw new InvalidOperationException("Server is already running.");
        }

        _listener.Start();
        _isRunning = true;
        Console.WriteLine("Server started. Listening for connections...");

        try
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                var client = await _listener.AcceptTcpClientAsync(cancellationToken);

                try
                {
                    OnAccept?.Invoke(client);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"OnAccept handler error: {ex}");
                }
            }
        }
        catch (OperationCanceledException)
        {
                
        }
        finally
        {
            Stop();
        }
    }

    public void Stop()
    {
        _listener.Stop();
        _isRunning = false;
    }

}

