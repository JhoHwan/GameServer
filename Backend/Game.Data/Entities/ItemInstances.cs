using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.ComponentModel.DataAnnotations;
using System.Text;

namespace Game.Data.Entities
{
    public enum EItemSlotType : byte
    {
        Inventory = 0,
        Equipment,
        Skin, 
        Storage,
    }

    public enum EEquipSlot : byte
    {
        Cap = 1,
        Cloth,
        Gloves,
        Pants,
        Shoes,
        EarRing,
        Belt,
        Pendant,
        Cape,
        LWeapon,
        RWeapon,
    }

    public enum ERarity : byte
    {
	    Normal = 1,
	    Rare,
	    Elite,
	    Excellent,
    };

    public class ItemInstances
    {
        [Key]
        public int Id { get; set; }

        [Required]
        public int CharacterId { get; set; }
        public Character Character { get; set; } = null!;

        [Required]
        public int ItemId { get; set; }

        [Required]
        public EItemSlotType SlotType { get; set; } = EItemSlotType.Inventory;

        public int SlotIndex { get; set; }
        public EEquipSlot? EquipSlot { get; set; }

        public ERarity Rarity { get; set; } = ERarity.Normal;
        public int Quantity { get; set; } = 1;

        public DateTime CreateTime { get; set; } = DateTime.UtcNow;
        public DateTime? DeleteTime { get; set; }
    }
}
