namespace Game.WebAPI;

public enum EErrorCode
{
    None = 0,

    // ==========================================
    // 0 ~ 999 : 공통 시스템 에러 (어디서든 발생 가능)
    // ==========================================
    Unknown = 1,            // 알 수 없는 오류
    ServerInternal = 2,     // DB Error
    InvalidParam = 3,       // 빈칸이 있거나 형식이 틀림 (로그인/가입 둘 다 씀)

    // ==========================================
    // 1000 ~ 1999 : 인증 (Auth) - 로그인 & 회원가입 & 토큰
    // ==========================================
    Auth_InvalidCreds = 1001,    // (로그인용) 없는 유저
    Auth_AccountLocked = 1002,   // (로그인용) 정지된 계정

    Auth_EmailDuplicated = 1010,    // (가입용) 이미 있는 이메일


    Char_NoCharacter = 2001,
    Char_ExistingCharName = 2002,
}

public class APIResponse
{
    public EErrorCode ErrorCode { get; set; } = EErrorCode.None;
    public Object? Data { get; set; } = null;
}
