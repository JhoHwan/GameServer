using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using Microsoft.IdentityModel.Tokens;
using System.ComponentModel.DataAnnotations;
using System.IdentityModel.Tokens.Jwt;
using System.Security.Claims;
using System.Text;
using Game.Data;
using Game.WebAPI;
using Game.WebAPI.DTOs;
using Game.Data.Entities;

namespace Game.WebAPI.DTOs
{ 
    public class RegisterRequest
    {
        [Required, EmailAddress, MaxLength(50)]
        public string Email { get; set; } = string.Empty;

        [Required, MinLength(6), MaxLength(20)]
        public string Password { get; set; } = string.Empty;
    }

    public class LoginRequest
    {
        [Required, EmailAddress, MaxLength(50)]
        public string Email { get; set; } = string.Empty;

        [Required, MinLength(6), MaxLength(20)]
        public string Password { get; set; } = string.Empty;
    }
}


namespace Game.WebAPI.Controllers
{
    [ApiController]
    [Route("api/[controller]/[action]")]
    public class AuthController : ControllerBase
    {
        private readonly GameDbContext _db;
        private readonly IConfiguration _config;
        private readonly ILogger<AuthController> _logger;

        enum EAuthErrorCode
        {
            Success = 0,
            InvalidCreds,

        }

        public AuthController(GameDbContext db, IConfiguration config, ILogger<AuthController> logger)
        {
            _db = db;
            _config = config;
            _logger = logger;
        }

        [HttpPost]
        public async Task<IActionResult> Register([FromBody] RegisterRequest request)
        {
            _logger.LogInformation($"{DateTime.Now} - 회원가입 시도 : {request.Email}, {request.Password}");

            if (await _db.Users.AnyAsync(u => u.Email == request.Email))
            {
                return BadRequest(new APIResponse
                {
                    ErrorCode = EErrorCode.Auth_EmailDuplicated
                });
            }

            User user = new User
            {
                Email = request.Email,
                PasswordHash = BCrypt.Net.BCrypt.HashPassword(request.Password)
            };

            _db.Users.Add(user);

            try
            {
                await _db.SaveChangesAsync();
            }
            catch (DbUpdateException e)
            {
                _logger.LogError(e.InnerException?.Message ?? e.Message);
                return BadRequest(new APIResponse
                {
                    ErrorCode = EErrorCode.ServerInternal,
                });
            }
            catch (Exception ex)
            {
                _logger.LogError(ex.Message);
                return StatusCode(500, new APIResponse
                {
                    ErrorCode = EErrorCode.ServerInternal,
                });
            }

            _logger.LogInformation($"회원가입 성공 : {user.Email}");
            return Ok(new APIResponse{  });
        }

        [HttpPost]
        public async Task<IActionResult> Login([FromBody] DTOs.LoginRequest request)
        {
            _logger.LogInformation($"{DateTime.Now} - 로그인 시도 : {request.Email}, {request.Password}");

            User? user = await _db.Users.FirstOrDefaultAsync(u => u.Email == request.Email);
            if (user == null || !BCrypt.Net.BCrypt.Verify(request.Password, user.PasswordHash))
            {
                return Unauthorized(new APIResponse
                {
                    ErrorCode = EErrorCode.Auth_InvalidCreds
                });
            }

            string token = GenerateJwtToken(user);

            return Ok(new APIResponse
            {
                Data = new { token }
            });
        }

        private string GenerateJwtToken(User user)
        {
            IConfigurationSection jwtSection = _config.GetSection("Jwt");
            SymmetricSecurityKey key = new SymmetricSecurityKey(Encoding.UTF8.GetBytes(jwtSection["Key"]!));
            SigningCredentials creds = new SigningCredentials(key, SecurityAlgorithms.HmacSha256);

            Claim[] claims = new[]
        {
            new Claim(JwtRegisteredClaimNames.Sub, user.Id.ToString()),
            new Claim(JwtRegisteredClaimNames.Email, user.Email)
        };

            JwtSecurityToken token = new JwtSecurityToken(
            issuer: jwtSection["Issuer"],
            audience: jwtSection["Audience"],
            claims: claims,
            expires: DateTime.UtcNow.AddMinutes(double.Parse(jwtSection["ExpireMinutes"]!)),
            signingCredentials: creds
            );

            return new JwtSecurityTokenHandler().WriteToken(token);
        }
    }
}

