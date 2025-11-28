using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Drawing;
using System.Text;

namespace Game.Data.Entities
{
    public enum EAppearanceSlot : byte
    {
        Hair = 1,
        Skin,
        Face,
    }

    [Table("CharacterAppearances")]
    public class CharacterAppearance
    {
        [Key]
        public int Id { get; set; }

        [Required, ForeignKey("CharacterId")]
        public int CharacterId { get; set; }
        public Character Character { get; set; } = null!;

        [Required]
        public int AppearanceItemId { get; set; }  

        [Required]
        public EAppearanceSlot Slot { get; set; } 

        public int? Color1 { get; set; }
    }
}
