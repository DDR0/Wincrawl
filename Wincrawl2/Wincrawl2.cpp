#include <windows.h>

#include <iostream>
#include <array>
#include <sstream>
#include <string>



class Tile
{
private:

public:
	static uint_fast16_t indexNumber;
	struct Dirs
	{
		Tile* north{ nullptr };
		Tile* east{ nullptr };
		Tile* south{ nullptr };
		Tile* west{ nullptr };
	};

	uint_fast16_t index{ 0 };
	Dirs dirs;

	Tile(Dirs dirs = Dirs())
		: dirs(dirs)
	{
		index = indexNumber++;
	}

	friend auto operator<<(std::ostream& os, Dirs const& dirs) -> std::ostream& {
		return os << "{"
			<< dirs.north << "↕" << dirs.south << ", "
			<< dirs.west << "↔" << dirs.east
			<< "}";
	}

	friend auto operator<<(std::ostream& os, Tile const& tile) -> std::ostream& {
		return os << "Tile " << tile.index << " " << tile.dirs;
	}
};
uint_fast16_t Tile::indexNumber = 0;




int main()
{
	using namespace std;

	SetConsoleOutputCP(CP_UTF8);

	cout << "⌛ Generating...\n";
	Tile tile = Tile();
	cout << "tile: " << tile << "\n";

	string command;
	do {
		getline(cin, command);
		cout << "> " << command << "\n";
	} while (command != "q");
}