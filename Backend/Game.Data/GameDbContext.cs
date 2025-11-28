using Game.Data.Entities;
using Microsoft.EntityFrameworkCore;
using System;
using System.Collections.Generic;
using System.Reflection.Emit;
using System.Text;

namespace Game.Data
{
    public class GameDbContext : DbContext
    {
        public DbSet<User> Users => Set<User>();
        public DbSet<Character> Characters => Set<Character>();
        public DbSet<CharacterAppearance> CharacterAppearances => Set<CharacterAppearance>();

        public DbSet<ItemInstances> ItemInstances => Set<ItemInstances>();

        public GameDbContext(DbContextOptions<GameDbContext> options)
            : base(options)
        {
        }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            base.OnModelCreating(modelBuilder);

            modelBuilder.Entity<Character>(entity =>
            {
                entity.HasKey(e => e.Id);
                entity.HasOne(e => e.User)
                      .WithMany(u => u.Characters)
                      .HasForeignKey(e => e.UserId)
                      .OnDelete(DeleteBehavior.Cascade);
            });

            modelBuilder.Entity<CharacterAppearance>(entity =>
            {
                entity.HasKey(a => a.Id);
                entity.HasOne(a => a.Character)
                      .WithMany(c => c.Appearances)
                      .HasForeignKey(a => a.CharacterId)
                      .OnDelete(DeleteBehavior.Cascade);
            });

            modelBuilder.Entity<ItemInstances>(entity =>
            {
                entity.HasKey(i => i.Id);
                entity.HasOne(i => i.Character)
                      .WithMany(c => c.ItemInstances)
                      .HasForeignKey(i => i.CharacterId)
                      .OnDelete(DeleteBehavior.Cascade);
            });
        }
    }

}
