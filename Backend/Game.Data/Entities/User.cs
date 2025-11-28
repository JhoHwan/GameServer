using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Text;

namespace Game.Data.Entities;

[Table("Users")]
public class User
{
    [Key]
    public int Id { get; set; }

    [Required]
    [EmailAddress]
    public string Email { get; set; } = string.Empty;

    [Required]
    public string PasswordHash { get; set; } = string.Empty;

    [Required]
    public DateTime CreateTime { get; set; } = DateTime.UtcNow;
    
    public List<Character> Characters { get; set; } = new List<Character>();

}
