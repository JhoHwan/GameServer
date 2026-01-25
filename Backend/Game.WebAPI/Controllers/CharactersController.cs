using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using Microsoft.IdentityModel.Tokens;
using Game.Data;
using Game.Data.Entities;
using System.ComponentModel.DataAnnotations;
using System.IdentityModel.Tokens.Jwt;
using Game.WebAPI.DTOs;

namespace Game.WebAPI.DTOs
{
    public class AppearanceDto
    {
        public EAppearanceSlot Slot { get; set; }
        public int AppearanceItemId { get; set; }
        public int? Color1 { get; set; }
    }

    public class CharacterInfo
    {
        public string Name { get; set; } = string.Empty;
        public ECharacterGender Gender { get; set; }
        public int Level { get; set; }

        public List<AppearanceDto> Appearances { get; set; } = new();

        public DateTime? LastLoginTime { get; set; }
    }


    public class CharacterCreateRequest
    {
        public string Name { get; set; } = string.Empty;

        public ECharacterGender Gender { get; set; }

        public Dictionary<EAppearanceSlot, AppearanceDto> Appearances { get; set; } = new();
    }
}

namespace Game.WebAPI.Controllers
{
    [ApiController]
    [Authorize]
    [Route("api/[controller]")]
    public class CharactersController : ControllerBase
    {
        private readonly GameDbContext _db;
        private readonly IConfiguration _config;
        private readonly ILogger<CharactersController> _logger;

        public CharactersController(GameDbContext db, IConfiguration config, ILogger<CharactersController> logger)
        {
            _db = db;
            _config = config;
            _logger = logger;
        }

        [HttpGet]
        public async Task<IActionResult> GetCharacters()
        {
            int userId = int.Parse(User.FindFirst(JwtRegisteredClaimNames.Sub)!.Value);

            var characters = await _db.Characters
        .Where(c => c.UserId == userId && c.DeleteTime == null)
        .Select(c => new CharacterInfo
        {
            Name = c.Name,
            Gender = c.Gender,
            Level = c.Level,
            LastLoginTime = c.LastLoginTime,
            Appearances = c.Appearances
                .Select(a => new AppearanceDto
                {
                    Slot = a.Slot,
                    AppearanceItemId = a.AppearanceItemId,
                    Color1 = a.Color1
                })
                .ToList()
        })
        .ToListAsync();

            if (characters.Count == 0)
            {
                return NotFound(new APIResponse
                {
                    ErrorCode = EErrorCode.Char_NoCharacter
                });
            }

            return Ok(new APIResponse
            {
                Data = new { characters }
            });
        }

        [HttpPost]
        public async Task<IActionResult> CreateCharacter([FromBody] CharacterCreateRequest request)
        {
            if (await _db.Characters.AnyAsync(c => c.Name == request.Name && c.DeleteTime == null))
            {
                return BadRequest(new APIResponse
                {
                    ErrorCode = EErrorCode.Char_ExistingCharName
                });
            }

            int userId = int.Parse(User.FindFirst(JwtRegisteredClaimNames.Sub)!.Value);

            var character = new Character
            {
                UserId = userId,
                Name = request.Name,
                Gender = request.Gender,
            };

            foreach (var (slot, appearance) in request.Appearances)
            {
                character.Appearances.Add(new CharacterAppearance
                {
                    Slot = slot,
                    AppearanceItemId = appearance.AppearanceItemId,
                    Color1 = appearance.Color1,
                });
            }

            _db.Characters.Add(character);

            try
            {
                await _db.SaveChangesAsync();
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "캐릭터 생성 중 DB 오류");
                return StatusCode(500, new APIResponse
                {
                    ErrorCode = EErrorCode.ServerInternal
                });
            }

            return Ok(new APIResponse
            {
                ErrorCode = EErrorCode.None,
                Data = new
                {
                    character.Id,
                    character.Name,
                    Gender = (int)character.Gender
                }
            });
        }

        [HttpDelete("{name}")]
        public async Task<IActionResult> DeleteCharacter([FromRoute] string name)
        {
            int userId = int.Parse(User.FindFirst(JwtRegisteredClaimNames.Sub)!.Value);

            var character = await _db.Characters
                .SingleOrDefaultAsync(c => c.Name == name && c.UserId == userId && c.DeleteTime == null);

            if (character == null)
            {
                return BadRequest(new APIResponse
                {
                    //Error = true,
                    //Message = "존재하지 않거나 이미 삭제된 캐릭터"
                });
            }

            character.DeleteTime = DateTime.UtcNow;
            await _db.SaveChangesAsync();

            return Ok(new APIResponse
            {
            });
        }
    }
}
