using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace Game.Data.Migrations
{
    /// <inheritdoc />
    public partial class AlertCharacterAppearanceTable : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "Color2",
                table: "CharacterAppearances");

            migrationBuilder.DropColumn(
                name: "Color3",
                table: "CharacterAppearances");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<int>(
                name: "Color2",
                table: "CharacterAppearances",
                type: "int",
                nullable: true);

            migrationBuilder.AddColumn<int>(
                name: "Color3",
                table: "CharacterAppearances",
                type: "int",
                nullable: true);
        }
    }
}
