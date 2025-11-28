using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Diagnostics.CodeAnalysis;

namespace Game.Data.Entities;

public enum ECharacterGender : byte
{
    None = 0,
    Male = 1,
    Female = 2,
}

[Table("Characters")]
public class Character
{
    [Key]
    public int Id { get; set; }

    [Required, ForeignKey("UserId")]
    public int UserId { get; set; }
    public User User { get; set; } = null!;

    [Required, MaxLength(20)]
    public string Name { get; set; } = string.Empty;

    [Required]
    public ECharacterGender Gender { get; set; } = ECharacterGender.Male;

    [Required]
    public int Level { get; set; } = 1;

    [Required]
    public int Exp { get; set; } = 0;

    [Required]
    public int Meso { get; set; } = 0;

    public DateTime? LastLoginTime { get; set; }
    public DateTime? DeleteTime { get; set; }

    public List<CharacterAppearance> Appearances { get; set; } = new();
    public List<ItemInstances> ItemInstances { get; set; } = new();
}
