using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Design;

namespace Game.Data
{
    public class GameDbContextFactory : IDesignTimeDbContextFactory<GameDbContext>
    {
        public GameDbContext CreateDbContext(string[] args)
        {
            var optionsBuilder = new DbContextOptionsBuilder<GameDbContext>();

            // 실제 사용하는 MySQL 연결 문자열
            var connectionString = "server=localhost;user=root;password=1234;database=game_db;";

            optionsBuilder.UseMySql(
                connectionString,
                ServerVersion.AutoDetect(connectionString));

            return new GameDbContext(optionsBuilder.Options);
        }
    }
}
