namespace Game.WebAPI;

public class APIResponse
{
    public bool Error { get; set; }
    public string Message { get; set; } = string.Empty;

    public Object? Data { get; set; } = null;
}
