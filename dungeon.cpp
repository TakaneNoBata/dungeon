
#include "dungeon.h"
#include <wchar.h>
#include <locale.h>
#pragma comment(lib,"libmysql.lib")
MYSQL mysql;



namespace {
	char keyMsg;
	HWND hwnd;
	std::random_device rd;
	std::mt19937 mt(rd());
	int randomInt(int exclusiveMax)
	{
		std::uniform_int_distribution<> dist(0, exclusiveMax - 1);
		return dist(mt);
	}

	//生成一个从min到max的随机数
	int randomInt(int min, int max)
	{
		std::uniform_int_distribution<> dist(0, max - min);
		return dist(mt) + min;
	}

	//按概率返回一个一个布尔值
	bool randomBool(double probability = 0.5)
	{
		std::bernoulli_distribution dist(probability);
		return dist(mt);
	}
}
struct Rect
{
	int x, y;
	int width, height;
};

struct Player
{
	int x, y;//玩家坐标
	int HP=10;
	int atk=5;
	int dfc=5;
	int score=0;
	int level=1;
};

struct MonsterXY
{
	int x, y;
	int sub_id;
};

class Dungeon
{
private:
	int width, height;
	std::vector<std::pair<int, int>> tiles;
	std::vector<Rect> rooms; // 房间用于放置楼梯和怪物
	std::vector<Rect> exits; // 4 sides of rooms or corridors
	struct Player player;

public:
	

	enum Direction
	{
		North,
		South,
		West,
		East,
		DirectionCount
	};

	enum Combat
	{
		Win,
		Lose
	};

public:
	Dungeon(int _width, int _height)
		: width(_width)
		, height(_height)
		, tiles(width* height,std::pair<char,int>(ROAD,0))
		, rooms()
		, exits()
	{
		mysql_init(&mysql);

		if (!mysql_real_connect(&mysql, "127.0.0.1", "root", "root", "dungeon", 3306, 0, 0));
		{
			std::cout << "mysql_real_connect failure" << std::endl;
		}
	}

	void generate(int maxFeatures)
	{
		// 在地图中央放置一个房间，这个房间四个侧面均是出口
		if (!makeRoom(width / 2, height / 2, static_cast<Direction>(randomInt(4), true)))
		{
			std::cout << "Unable to place the first room.\n";
			return;
		}

		// 始终保证第一个房间有一个打开的门
		for (int i = 1; i < maxFeatures; ++i)
		{
			if (!createFeature())
			{
				std::cout << "Unable to place more features (placed " << i << ").\n";
				break;
			}
		}

		if (!placeObject(StairsUp))
		{
			std::cout << "Unable to place up stairs.\n";
			return;
		}

		if (!placeObject(StairsDown))
		{
			std::cout << "Unable to place down stairs.\n";
			return;
		}

		//while (placeObject(Monster));
		while (true)
		{	
			int rand = randomInt(100);
			if (rand < 70)
			{	
				MYSQL_RES* result = nullptr;
				MYSQL_ROW sql_row;
				MYSQL_FIELD* fd;
				int		res;
				int		row_num;

				int		rare_count=0;
				int		rare_rand=0;
				int		sub_id=0;

				res = mysql_query(&mysql, "select rare from mon_tb");
				if (!res) {
					result = mysql_store_result(&mysql);
					if (result) {
						row_num = mysql_num_fields(result);
						while (sql_row = mysql_fetch_row(result)) {						
								std::string srare = sql_row[0];
								int rare = stoi(srare);
								rare_count += rare;
						}
					}
				}
				mysql_free_result(result);

				int rare_count_temp = 0;
				rare_rand = randomInt(rare_count);
				res = mysql_query(&mysql, "select rare from mon_tb");
				if (!res) {
					result = mysql_store_result(&mysql);
					if (result) {
						row_num = mysql_num_fields(result);
						while (sql_row = mysql_fetch_row(result)) {
								std::string srare = sql_row[0];
								int rare = stoi(srare);
								rare_count_temp += rare;
								if (rare_rand <= rare_count_temp) {
									break;
								}
								sub_id++;
						}
					}
				}
				mysql_free_result(result);

				if (!placeObject(std::pair < int, int>(MONSTER,sub_id))) {
					break;
				}

			}
			else
			{	
				int rand = randomInt(100);
				if (rand < 25) {
					if (!placeObject(Pakellas0))
						break;
				}
				else if (rand < 50) {
					if (!placeObject(Pakellas1))
						break;
				}
				else if (rand < 75) {
					if (!placeObject(Pakellas2))
						break;
				}
				else {
					if (!placeObject(Pakellas3))
						break;
				}
				
			}
		}

		/*for (char& tiles : tiles)
		{
			if (tiles == Road)
				tiles = '.';
			else if (tiles == Road || tiles == Road)
				tiles = ' ';
		}*/
	}

	//清空函数，将地图初始化
	void clear()
	{
		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
				settiles(x, y, Road);
		}
	}

	//画图函数
	void print();

	void move(char keyMsg)
	{
		switch (keyMsg)
		{
		case 'w':
			if (gettiles(player.x, player.y - 1) == Road)
			{	
				settiles(player.x, player.y, Road);
				settiles(player.x, player.y-1, Player0);
				player.y--;
				keyMsg = '#';
			}
			else if (gettiles(player.x, player.y - 1) == ClosedDoor)
			{
				settiles(player.x, player.y - 1, OpenDoor);
				keyMsg = '#';
			}
			else if (gettiles(player.x, player.y - 1) == OpenDoor)
			{
				settiles(player.x, player.y, Road);
				settiles(player.x, player.y - 2, Player0);
				player.y= player.y-2;
				keyMsg = '#';
			}
			else if (gettiles(player.x, player.y - 1).first == MONSTER)
			{
				if (combat() == Win)
				{
					settiles(player.x, player.y - 1, Road);
					MessageBox(hwnd, L"你击杀了怪物", L"战斗信息", MB_OK);
					player.score += player.level;
				}
				else
				{
					MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
					restart();
				}
				keyMsg = '#';
			}
			else if (gettiles(player.x, player.y - 1) == StairsDown)
			{
				clear();
				generate(50);
				player.score += player.level * 10;
				player.level++;
				keyMsg = '#';
			}
			else if (gettiles(player.x, player.y - 1).first == CHEST)
			{
				int rand = randomInt(90);
				if (rand < 30)
				{
					player.HP += 10;
					MessageBox(hwnd, L"你打开了宝箱，获得了一瓶生命药水，HP+10.", L"宝箱信息", MB_OK);
					
				}
				else if (rand >= 30 && rand < 60)
				{
					player.atk++;
					MessageBox(hwnd, L"你打开了宝箱，获得了大宝剑，攻击力+1.", L"宝箱信息", MB_OK);
				}
				else
				{
					player.dfc++;
					MessageBox(hwnd, L"你打开了宝箱，获得了盾牌，防御力+1.", L"宝箱信息", MB_OK);
				}
				settiles(player.x, player.y - 1, Road);
			}
			break;
		case 's':
			if (gettiles(player.x, player.y + 1) == Road)
			{
				settiles(player.x, player.y, Road);
				settiles(player.x, player.y + 1, Player0);
				player.y++;
				keyMsg = '#';
			}
			else if (gettiles(player.x, player.y + 1) == ClosedDoor)
			{
				settiles(player.x, player.y + 1, OpenDoor);
				keyMsg = '#';
			}
			else if (gettiles(player.x, player.y + 1) == OpenDoor)
			{
				settiles(player.x, player.y, Road);
				settiles(player.x, player.y + 2, Player0);
				player.y+=2;
				keyMsg = '#';
			}
			else if (gettiles(player.x, player.y + 1).first == MONSTER)
			{
				if (combat() == Win)
				{
					settiles(player.x, player.y + 1, Road);
					MessageBox(hwnd, L"你击杀了怪物", L"战斗信息", MB_OK);
					player.score += player.level;
				}
				else
				{
					MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
					restart();
				}
				keyMsg = '#';
			}
			else if (gettiles(player.x, player.y + 1) == StairsDown)
			{
				clear();
				generate(50);
				player.score += player.level * 10;
				player.level++;
				keyMsg = '#';
			}
			else if (gettiles(player.x, player.y + 1).first == CHEST)
			{
				int rand = randomInt(90);
				if (rand < 30)
				{
					player.HP += 10;
					MessageBox(hwnd, L"你打开了宝箱，获得了一瓶生命药水，HP+10.", L"宝箱信息", MB_OK);

				}
				else if (rand >= 30 && rand < 60)
				{
					player.atk++;
					MessageBox(hwnd, L"你打开了宝箱，获得了大宝剑，攻击力+1.", L"宝箱信息", MB_OK);
				}
				else
				{
					player.dfc++;
					MessageBox(hwnd, L"你打开了宝箱，获得了盾牌，防御力+1.", L"宝箱信息", MB_OK);
				}
				settiles(player.x, player.y + 1, Road);
			}
			break;
		case 'd':
			if (gettiles(player.x +1 , player.y) == Road)
			{
				settiles(player.x, player.y, Road);
				settiles(player.x + 1, player.y, Player0);
				player.x++;
				keyMsg = '#';
			}
			else if (gettiles(player.x + 1, player.y) == ClosedDoor)
			{
				settiles(player.x + 1, player.y, OpenDoor);
				keyMsg = '#';
			}
			else if (gettiles(player.x + 1, player.y) == OpenDoor)
			{
				settiles(player.x, player.y, Road);
				settiles(player.x + 2, player.y, Player0);
				player.x+=2;
				keyMsg = '#';
			}
			else if (gettiles(player.x + 1, player.y).first == MONSTER)
			{
				if (combat() == Win)
				{
					settiles(player.x + 1, player.y, Road);
					MessageBox(hwnd, L"你击杀了怪物", L"战斗信息", MB_OK);
					player.score += player.level;
				}
				else
				{
					MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
					restart();
				}
				keyMsg = '#';
			}
			else if (gettiles(player.x + 1, player.y) == StairsDown)
			{
				clear();
				generate(50);
				player.score += player.level * 10;
				player.level++;
				keyMsg = '#';
			}
			else if (gettiles(player.x + 1, player.y ).first == CHEST)
			{
				int rand = randomInt(90);
				if (rand < 30)
				{
					player.HP += 10;
					MessageBox(hwnd, L"你打开了宝箱，获得了一瓶生命药水，HP+10.", L"宝箱信息", MB_OK);

				}
				else if (rand >= 30 && rand < 60)
				{
					player.atk++;
					MessageBox(hwnd, L"你打开了宝箱，获得了大宝剑，攻击力+1.", L"宝箱信息", MB_OK);
				}
				else
				{
					player.dfc++;
					MessageBox(hwnd, L"你打开了宝箱，获得了盾牌，防御力+1.", L"宝箱信息", MB_OK);
				}
				settiles(player.x + 1, player.y, Road);
			}
			break;
		case 'a':
			if (gettiles(player.x - 1, player.y) == Road)
			{
				settiles(player.x, player.y, Road);
				settiles(player.x - 1, player.y, Player0);
				player.x--;
				keyMsg = '#';
			}
			else if (gettiles(player.x - 1, player.y) == ClosedDoor)
			{
				settiles(player.x - 1, player.y, OpenDoor);
			}
			else if (gettiles(player.x - 1, player.y) == OpenDoor)
			{
				settiles(player.x, player.y, Road);
				settiles(player.x - 2, player.y, Player0);
				player.x-=2;
				keyMsg = '#';
			}
			else if (gettiles(player.x - 1, player.y).first == MONSTER)
			{
				if (combat() == Win)
				{
					settiles(player.x - 1, player.y, Road);
					MessageBox(hwnd, L"你击杀了怪物", L"战斗信息", MB_OK);
					player.score += player.level;
				}
				else
				{
					MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
					restart();
				}
				keyMsg = '#';
			}
			else if (gettiles(player.x - 1, player.y) == StairsDown)
			{
				clear();
				generate(50);
				player.score += player.level * 10;
				player.level++;
				keyMsg = '#';
			}
			else if (gettiles(player.x - 1, player.y).first == CHEST)
			{
				int rand = randomInt(90);
				if (rand < 30)
				{
					player.HP += 10;
					MessageBox(hwnd, L"你打开了宝箱，获得了一瓶生命药水，HP+10.", L"宝箱信息", MB_OK);

				}
				else if (rand >= 30 && rand < 60)
				{
					player.atk++;
					MessageBox(hwnd, L"你打开了宝箱，获得了大宝剑，攻击力+1.", L"宝箱信息", MB_OK);
				}
				else
				{
					player.dfc++;
					MessageBox(hwnd, L"你打开了宝箱，获得了盾牌，防御力+1.", L"宝箱信息", MB_OK);
				}
				settiles(player.x - 1, player.y, Road);
			}

			break;
		}
		print();
		FlushBatchDraw();
		monsterMove();
	}

private:
	std::pair<int,int> gettiles(int x, int y) const
	{
		return tiles[x + y * width];
	}

	void settiles(int x, int y, std::pair<int,int> _tiles)
	{
		tiles[x + y * width] = _tiles;
	}

	bool createFeature()
	{
		for (int i = 0; i < 1000; ++i)
		{
			if (exits.empty())
				break;

			// 随机选择一个出口
			int r = randomInt(exits.size());
			int x = randomInt(exits[r].x, exits[r].x + exits[r].width - 1);
			int y = randomInt(exits[r].y, exits[r].y + exits[r].height - 1);

			// north, south, west, east
			for (int j = 0; j < DirectionCount; ++j)
			{
				if (createFeature(x, y, static_cast<Direction>(j)))
				{
					exits.erase(exits.begin() + r);
					return true;
				}
			}
		}

		return false;
	}

	//这个函数用于生成门
	bool createFeature(int x, int y, Direction dir)
	{
		static const int roomChance = 50; // 生成房间概率与生成走廊概率相加为1

		int dx = 0;
		int dy = 0;

		if (dir == North)
			dy = 1;
		else if (dir == South)
			dy = -1;
		else if (dir == West)
			dx = 1;
		else if (dir == East)
			dx = -1;

		if (gettiles(x + dx, y + dy) != Road && gettiles(x + dx, y + dy) != Road)
			return false;

		if (randomInt(100) < roomChance)
		{
			if (makeRoom(x, y, dir))
			{
				settiles(x, y, ClosedDoor);

				return true;
			}
		}

		else
		{
			if (makeCorridor(x, y, dir))
			{
				if (gettiles(x + dx, y + dy) == Road)
					settiles(x, y, ClosedDoor);
				else // 防止在两个走廊之间生成门
					settiles(x, y, Road);

				return true;
			}
		}

		return false;
	}

	bool makeRoom(int x, int y, Direction dir, bool firstRoom = false)
	{
		static const int minRoomSize = 3;
		static const int maxRoomSize = 6;

		//随机生成一个3*3到6*6大小的矩形房间
		Rect room;
		room.width = randomInt(minRoomSize, maxRoomSize);
		room.height = randomInt(minRoomSize, maxRoomSize);

		if (dir == North)
		{
			room.x = x - room.width / 2;
			room.y = y - room.height;
		}

		else if (dir == South)
		{
			room.x = x - room.width / 2;
			room.y = y + 1;
		}

		else if (dir == West)
		{
			room.x = x - room.width;
			room.y = y - room.height / 2;
		}

		else if (dir == East)
		{
			room.x = x + 1;
			room.y = y - room.height / 2;
		}

		if (placeRect(room, Road))
		{
			rooms.emplace_back(room);

			if (dir != South || firstRoom) // north side
				exits.emplace_back(Rect{ room.x, room.y - 1, room.width, 1 });
			if (dir != North || firstRoom) // south side
				exits.emplace_back(Rect{ room.x, room.y + room.height, room.width, 1 });
			if (dir != East || firstRoom) // west side
				exits.emplace_back(Rect{ room.x - 1, room.y, 1, room.height });
			if (dir != West || firstRoom) // east side
				exits.emplace_back(Rect{ room.x + room.width, room.y, 1, room.height });

			return true;
		}

		return false;
	}

	bool makeCorridor(int x, int y, Direction dir)
	{	
		//生成一条3到6长的走廊
		static const int minCorridorLength = 3;
		static const int maxCorridorLength = 6;

		Rect corridor;
		corridor.x = x;
		corridor.y = y;

		if (randomBool()) // 水平走廊
		{
			corridor.width = randomInt(minCorridorLength, maxCorridorLength);
			corridor.height = 1;

			if (dir == North)
			{
				corridor.y = y - 1;

				if (randomBool()) // west
					corridor.x = x - corridor.width + 1;
			}

			else if (dir == South)
			{
				corridor.y = y + 1;

				if (randomBool()) // west
					corridor.x = x - corridor.width + 1;
			}

			else if (dir == West)
				corridor.x = x - corridor.width;

			else if (dir == East)
				corridor.x = x + 1;
		}

		else // 垂直走廊
		{
			corridor.width = 1;
			corridor.height = randomInt(minCorridorLength, maxCorridorLength);

			if (dir == North)
				corridor.y = y - corridor.height;

			else if (dir == South)
				corridor.y = y + 1;

			else if (dir == West)
			{
				corridor.x = x - 1;

				if (randomBool()) // north
					corridor.y = y - corridor.height + 1;
			}

			else if (dir == East)
			{
				corridor.x = x + 1;

				if (randomBool()) // north
					corridor.y = y - corridor.height + 1;
			}
		}

		if (placeRect(corridor, Road))
		{
			if (dir != South && corridor.width != 1) // north side
				exits.emplace_back(Rect{ corridor.x, corridor.y - 1, corridor.width, 1 });
			if (dir != North && corridor.width != 1) // south side
				exits.emplace_back(Rect{ corridor.x, corridor.y + corridor.height, corridor.width, 1 });
			if (dir != East && corridor.height != 1) // west side
				exits.emplace_back(Rect{ corridor.x - 1, corridor.y, 1, corridor.height });
			if (dir != West && corridor.height != 1) // east side
				exits.emplace_back(Rect{ corridor.x + corridor.width, corridor.y, 1, corridor.height });

			return true;
		}

		return false;
	}


	bool placeRect(const Rect& rect, std::pair<char,int> tiles)
	{
		if (rect.x < 1 || rect.y < 1 || rect.x + rect.width > width - 1 || rect.y + rect.height > height - 1)
			return false;

		for (int y = rect.y; y < rect.y + rect.height; ++y)
			for (int x = rect.x; x < rect.x + rect.width; ++x)
			{
				if (gettiles(x, y).first != ROAD)
					return false; // the area already used
			}

		for (int y = rect.y - 1; y < rect.y + rect.height + 1; ++y)
			for (int x = rect.x - 1; x < rect.x + rect.width + 1; ++x)
			{
				if (x == rect.x - 1 || y == rect.y - 1 || x == rect.x + rect.width || y == rect.y + rect.height)
					settiles(x, y, Wall);
				else
					settiles(x, y, tiles);
			}

		return true;
	}

	//这个函数用来在一个房间内生成宝箱、怪物或者玩家
	bool placeObject(std::pair<char,int>tiles)
	{
		if (rooms.empty())
			return false;

		int r = randomInt(rooms.size()); // 随机选择一个房间
		int x = randomInt(rooms[r].x + 1, rooms[r].x + rooms[r].width - 2);//随机选择横坐标
		int y = randomInt(rooms[r].y + 1, rooms[r].y + rooms[r].height - 2);// 随机选择纵坐标

		//该坐标下地形是空的
		if (gettiles(x, y) == Road)
		{
			settiles(x, y, tiles);

			if (tiles.first == UNPASSABLE && tiles.second == 6)//StairsUp
			{
				if (gettiles(x - 1, y) == Road)
				{
					settiles(x - 1, y, std::pair<int,int>(PLAYER,0));
					player.x = x - 1;
					player.y = y;
					std::cout << "test";
				}
				else if (gettiles(x + 1, y) == Road)
				{
					settiles(x + 1, y, std::pair<char, int>(PLAYER, 0));
					player.x = x + 1;
					player.y = y;
					std::cout << "test";
				}
				else if (gettiles(x, y - 1) == Road)
				{
					settiles(x, y - 1, std::pair<char, int>(PLAYER, 0));
					player.x = x ;
					player.y = y - 1 ;
					std::cout << "test";
				}
				else
				{
					settiles(x, y + 1, std::pair<char, int>(PLAYER, 0));
					player.x = x;
					player.y = y + 1;
					std::cout << "test";
				}
			}
			// place one object in one room (optional)
			//从向量中删除这个房间
			rooms.erase(rooms.begin() + r);

			return true;
		}

		return false;
	}

	void monsterMove()
	{	
		struct MonsterXY mxy[50];
		for (int i = 0; i < 50; i++)
		{
			mxy[i].x = -1;
			mxy[i].y = -1;
			mxy[i].sub_id = -1;
		}
		int monsterNum = 0;
		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				if (gettiles(x, y).first == MONSTER)
				{
					mxy[monsterNum].x = x;
					mxy[monsterNum].y = y;
					mxy[monsterNum].sub_id = gettiles(x,y).second;
					monsterNum++;
				}
			}
		}
		for (int i = 0; i < monsterNum ; i++)
		{
			int randNum = randomInt(100) / 25;
			switch (randNum)
			{
			case 0://East
				if (gettiles(mxy[i].x + 1, mxy[i].y) == Player0)
				{
					if (combat() == Win)
					{
						settiles(mxy[i].x, mxy[i].y, Road);
						MessageBox(hwnd, L"怪物主动攻击你，你击杀了怪物", L"战斗信息", MB_OK);
						player.score += player.level;

					}
					else
					{
						MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
						restart();
					}
				}
				else if (gettiles(mxy[i].x + 1, mxy[i].y) == Road)
				{
					settiles(mxy[i].x, mxy[i].y, Road);
					settiles(mxy[i].x + 1, mxy[i].y, std::pair<char,int>(MONSTER,mxy[i].sub_id));
				}
				else if (gettiles(mxy[i].x + 1, mxy[i].y) == OpenDoor)
				{
					if (gettiles(mxy[i].x + 2, mxy[i].y) == Road)
					{
						settiles(mxy[i].x, mxy[i].y, Road);
						settiles(mxy[i].x + 2, mxy[i].y, std::pair<char, int>(MONSTER, mxy[i].sub_id));

					}
					else if (gettiles(mxy[i].x + 2, mxy[i].y) == Player0)
					{
						if (combat() == Win)
						{
							settiles(mxy[i].x, mxy[i].y, Road);
							MessageBox(hwnd, L"怪物主动攻击你，你击杀了怪物", L"战斗信息", MB_OK);
							player.score += player.level;
						}
						else
						{
							MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
							restart();
						}
					}


				}

				break;
			case 1://West

				if (gettiles(mxy[i].x - 1, mxy[i].y).first == PLAYER)
				{
					if (combat() == Win)
					{
						settiles(mxy[i].x, mxy[i].y, Road);
						MessageBox(hwnd, L"怪物主动攻击你，你击杀了怪物", L"战斗信息", MB_OK);
						player.score += player.level;

					}
					else
					{
						MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
						restart();
					}
				}
				else if (gettiles(mxy[i].x - 1, mxy[i].y).first== ROAD)
				{
					settiles(mxy[i].x, mxy[i].y, Road);
					settiles(mxy[i].x - 1, mxy[i].y, std::pair<char, int>(MONSTER, mxy[i].sub_id));
				}
				else if (gettiles(mxy[i].x - 1, mxy[i].y) == OpenDoor)
				{
					if (gettiles(mxy[i].x - 2, mxy[i].y).first == ROAD)
					{
						settiles(mxy[i].x, mxy[i].y, Road);
						settiles(mxy[i].x - 2, mxy[i].y, std::pair<char, int>(MONSTER, mxy[i].sub_id));

					}
					else if (gettiles(mxy[i].x - 2, mxy[i].y).first == PLAYER)
					{
						if (combat() == Win)
						{
							settiles(mxy[i].x, mxy[i].y, Road);
							MessageBox(hwnd, L"怪物主动攻击你，你击杀了怪物", L"战斗信息", MB_OK);
							player.score += player.level;
						}
						else
						{
							MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
							restart();
						}
					}


				}
				break;
			case 2://South

				if (gettiles(mxy[i].x, mxy[i].y + 1).first == PLAYER)
				{
					if (combat() == Win)
					{
						settiles(mxy[i].x, mxy[i].y, Road);
						MessageBox(hwnd, L"怪物主动攻击你，你击杀了怪物", L"战斗信息", MB_OK);
						player.score += player.level;

					}
					else
					{
						MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
						restart();
					}
				}
				else if (gettiles(mxy[i].x, mxy[i].y + 1).first == ROAD)
				{
					settiles(mxy[i].x, mxy[i].y, Road);
					settiles(mxy[i].x, mxy[i].y + 1, std::pair<char, int>(MONSTER, mxy[i].sub_id));
				}
				else if (gettiles(mxy[i].x, mxy[i].y + 1) == OpenDoor)
				{
					if (gettiles(mxy[i].x, mxy[i].y + 2).first == ROAD)
					{
						settiles(mxy[i].x, mxy[i].y, Road);
						settiles(mxy[i].x, mxy[i].y + 2, std::pair<char, int>(MONSTER, mxy[i].sub_id));

					}
					else if (gettiles(mxy[i].x, mxy[i].y + 2).first == PLAYER)
					{
						if (combat() == Win)
						{
							settiles(mxy[i].x, mxy[i].y, Road);
							MessageBox(hwnd, L"怪物主动攻击你，你击杀了怪物", L"战斗信息", MB_OK);
							player.score += player.level;
						}
						else
						{
							MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
							restart();
						}
					}


				}
				break;
			case 3:

				if (gettiles(mxy[i].x, mxy[i].y - 1).first == PLAYER)
				{
					if (combat() == Win)
					{
						settiles(mxy[i].x, mxy[i].y, Road);
						MessageBox(hwnd, L"怪物主动攻击你，你击杀了怪物", L"战斗信息", MB_OK);
						player.score += player.level;

					}
					else
					{
						MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
						restart();
					}
				}
				else if (gettiles(mxy[i].x, mxy[i].y - 1).first == ROAD)
				{
					settiles(mxy[i].x, mxy[i].y, Road);
					settiles(mxy[i].x, mxy[i].y - 1, std::pair<char, int>(MONSTER, mxy[i].sub_id));
				}
				else if (gettiles(mxy[i].x, mxy[i].y - 1) == OpenDoor)
				{
					if (gettiles(mxy[i].x, mxy[i].y - 2).first == ROAD)
					{
						settiles(mxy[i].x, mxy[i].y, Road);
						settiles(mxy[i].x, mxy[i].y - 2, std::pair<char, int>(MONSTER, mxy[i].sub_id));

					}
					else if (gettiles(mxy[i].x, mxy[i].y - 2).first == PLAYER)
					{
						if (combat() == Win)
						{
							settiles(mxy[i].x, mxy[i].y, Road);
							MessageBox(hwnd, L"怪物主动攻击你，你击杀了怪物", L"战斗信息", MB_OK);
							player.score += player.level;
						}
						else
						{
							MessageBox(hwnd, L"你死了，将重新开始游戏", L"战斗信息", MB_OK);
							restart();
						}
					}


				}
				break;
			}
		}
	}

	unsigned combat()
	{	
		Monster monster;
		int damageToPlayer = monster.atk - player.dfc + player.level;
		if (damageToPlayer < 0)
			damageToPlayer = 0;
		int damageToMonster = player.atk - monster.dfc - player.level;
		if (damageToMonster < 0)
			damageToMonster = 0;

		monster.combatHP = monster.HP + player.level*2;

		while (player.HP > 0)
		{
			//玩家总是先攻击
			monster.combatHP -= damageToMonster;
			if (monster.combatHP <= 0)
				return Win;
			player.HP -= damageToPlayer;
		}

		return Lose;


	}

	void restart()
	{
		clear();
		player.HP = 10;
		player.atk = 5;
		player.dfc = 5;
		player.score = 0;
		player.level = 1;
		generate(50);
	}


};

//int main()
//{
//	
//	
//
//	hwnd = initgraph(1600, 768);//50*24
//
//	/*loadimage(&imageFloor, _T("floor.png"));
//	loadimage(&imageCorridor, _T("corridor.png"));
//	loadimage(&imageWall, _T("wall.png"));
//	loadimage(&imageClosedDoor,_T("closed_door.png"));
//	loadimage(&imageOpenDoor, _T("open_door.png"));
//	loadimage(&imageUpstair, _T("stairs_up.png"));
//	loadimage(&imageDownstair, _T("stairs_down.png"));
//	loadimage(&imagePlayer, _T("player.png"));
//	loadimage(&imageMonster, _T("tiles/monster/alligator.png"));
//	loadimage(&imageChest, _T("chest.png"));
//	loadimage(&imageCloud, _T("cloud.png"));*/
//
//	loadImage();
//	BeginBatchDraw();
//	Dungeon d(45, 24);
//	d.generate(50);
//	d.print();
//	FlushBatchDraw();
//	while (true)
//	{
//		cleardevice();
//		BeginBatchDraw();
//		if (_kbhit())
//		{
//			keyMsg = _getch();
//			d.move(keyMsg);
//			
//		}
//		d.print();
//		FlushBatchDraw();
//
//	}
//	
//	std::cout << "Press Enter to quit... ";
//	std::cin.get();
//	EndBatchDraw();
//
//	return 0;
//}

void loadImage() {
	
	MYSQL mysql;
	MYSQL_RES* result = nullptr;
	MYSQL_ROW sql_row;
	MYSQL_FIELD* fd;
	int res;
	mysql_init(&mysql);
	if (mysql_real_connect(&mysql, "127.0.0.1", "root", "root", "dungeon", 3306, NULL, 0)) {
		res = mysql_query(&mysql, "select path from unpassable_tb");
		if (!res) {
			int i = 0;
			result = mysql_store_result(&mysql);
			while (sql_row = mysql_fetch_row(result)) {
				std::string str = sql_row[0];
				std::wstring wstr;
				StringToWstring_CRT(str, wstr);
				LPCTSTR path = wstr.c_str();
				loadimage(&imageMap[std::pair<int, int>(UNPASSABLE, i)], path);
				i++;
			}
			mysql_free_result(result);
		}

		res = mysql_query(&mysql, "select path from mon_tb");
		if (!res) {
			int i = 0;
			result = mysql_store_result(&mysql);
			while (sql_row = mysql_fetch_row(result)) {
				std::string str = sql_row[0];
				std::wstring wstr;
				StringToWstring_CRT(str, wstr);
				LPCTSTR path = wstr.c_str();
				loadimage(&imageMap[std::pair<int, int>(MONSTER, i)], path);
				i++;
			}
			mysql_free_result(result);
		}

		res = mysql_query(&mysql, "select path from chest_tb");
		if (!res) {
			int i = 0;
			result = mysql_store_result(&mysql);
			while (sql_row = mysql_fetch_row(result)) {
				std::string str = sql_row[0];
				std::wstring wstr;
				StringToWstring_CRT(str, wstr);
				LPCTSTR path = wstr.c_str();
				loadimage(&imageMap[std::pair<int, int>(CHEST, i)], path);
				i++;
			}
			mysql_free_result(result);
		}
	}
	mysql_close(&mysql);

	return;
}

//画图函数

inline void Dungeon::print()
{
	//	
	//	for (int y = _player.y-1; y < _player.y+2; ++y)//九宫格 
	//	{
	//		for (int x = _player.x-1; x < _player.x+2; ++x)
	//		{
	//			if (gettiles(x, y) == Chest)
	//				putimage(x * 32, y * 32, &imageChest);
	//			else if (gettiles(x, y) == Road)
	//				putimage(x * 32, y * 32, &imageFloor);
	//			else if (gettiles(x, y) == Road)
	//				putimage(x * 32, y * 32, &imageCorridor);
	//			else if (gettiles(x, y) == Wall)
	//				putimage(x * 32, y * 32, &imageWall);
	//			else if (gettiles(x, y) == ClosedDoor)
	//				putimage(x * 32, y * 32, &imageClosedDoor);
	//			else if (gettiles(x, y) == OpenDoor)
	//				putimage(x * 32, y * 32, &imageOpenDoor);
	//			else if (gettiles(x, y) == UpStairs)
	//				putimage(x * 32, y * 32, &imageUpstair);
	//			else if (gettiles(x, y) == StairsDown)
	//				putimage(x * 32, y * 32, &imageDownstair);
	//			else if (gettiles(x, y) == Player)
	//				putimage(x * 32, y * 32, &imagePlayer);
	//			else if (gettiles(x, y) == Monster)
	//				putimage(x * 32, y * 32, &imageMonster);
	//		}

	//		
	//	}
	//	if (gettiles(_player.x - 1, _player.y) != Wall && gettiles(_player.x - 1, _player.y) != ClosedDoor)//West
	//	{
	//		if (gettiles(_player.x-2, _player.y) == Road)
	//			;
	//		else if (gettiles(_player.x - 2, _player.y) == Road)
	//			putimage((_player.x - 2) * 32, _player.y * 32, &imageFloor);
	//		else if (gettiles(_player.x - 2, _player.y) == Road)
	//			putimage((_player.x - 2) * 32, _player.y * 32, &imageCorridor);
	//		else if (gettiles(_player.x - 2, _player.y) == Wall)
	//			putimage((_player.x - 2) * 32, _player.y * 32, &imageWall);
	//		else if (gettiles(_player.x - 2, _player.y) == ClosedDoor)
	//			putimage((_player.x - 2) * 32, _player.y * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x - 2, _player.y) == OpenDoor)
	//			putimage((_player.x - 2) * 32, _player.y * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x - 2, _player.y) == UpStairs)
	//			putimage((_player.x - 2) * 32, _player.y * 32, &imageUpstair);
	//		else if (gettiles(_player.x - 2, _player.y) == StairsDown)
	//			putimage((_player.x - 2) * 32, _player.y * 32, &imageDownstair);
	//		else if (gettiles(_player.x - 2, _player.y) == Player)
	//			putimage((_player.x - 2) * 32, _player.y * 32, &imagePlayer);
	//		else if (gettiles(_player.x - 2, _player.y) == Monster)
	//			putimage((_player.x - 2) * 32, _player.y * 32, &imageMonster);
	//		else if (gettiles(_player.x - 2, _player.y) == Chest)
	//			putimage((_player.x - 2) * 32, _player.y * 32, &imageChest);

	//		if (gettiles(_player. x - 2, _player.y) != Wall && gettiles(_player. x - 2, _player.y) != ClosedDoor)
	//		{
	//			if (gettiles(_player.x - 3, _player.y) == Road)
	//				;
	//			else if (gettiles(_player.x - 3, _player.y) == Road)
	//				putimage((_player.x - 3) * 32, _player.y * 32, &imageFloor);
	//			else if (gettiles(_player.x - 3, _player.y) == Road)
	//				putimage((_player.x - 3) * 32, _player.y * 32, &imageCorridor);
	//			else if (gettiles(_player.x - 3, _player.y) == Wall)
	//				putimage((_player.x - 3) * 32, _player.y * 32, &imageWall);
	//			else if (gettiles(_player.x - 3, _player.y) == ClosedDoor)
	//				putimage((_player.x - 3) * 32, _player.y * 32, &imageClosedDoor);
	//			else if (gettiles(_player.x - 3, _player.y) == OpenDoor)
	//				putimage((_player.x - 3) * 32, _player.y * 32, &imageOpenDoor);
	//			else if (gettiles(_player.x - 3, _player.y) == UpStairs)
	//				putimage((_player.x - 3) * 32, _player.y * 32, &imageUpstair);
	//			else if (gettiles(_player.x - 3, _player.y) == StairsDown)
	//				putimage((_player.x - 3) * 32, _player.y * 32, &imageDownstair);
	//			else if (gettiles(_player.x - 3, _player.y) == Player)
	//				putimage((_player.x - 3) * 32, _player.y * 32, &imagePlayer);
	//			else if (gettiles(_player.x - 3, _player.y) == Monster)
	//				putimage((_player.x - 3) * 32, _player.y * 32, &imageMonster);
	//			else if (gettiles(_player.x - 3, _player.y) == Chest)
	//				putimage((_player.x - 3) * 32, _player.y * 32, &imageChest);
	//		}
	//	}

	//	if (gettiles(_player.x - 1, _player.y - 1) != Wall && gettiles(_player.x - 1, _player.y - 1) != ClosedDoor)//Northwest
	//	{
	//		if (gettiles(_player.x - 2, _player.y - 1) == Road)
	//			;
	//		else if (gettiles(_player.x - 2, _player.y - 1) == Road)
	//			putimage((_player.x - 2) * 32, (_player.y - 1) * 32, &imageFloor);
	//		else if (gettiles(_player.x - 2, _player.y - 1) == Road)
	//			putimage((_player.x - 2) * 32, (_player.y - 1) * 32, &imageCorridor);
	//		else if (gettiles(_player.x - 2, _player.y - 1) == Wall)
	//			putimage((_player.x - 2) * 32, (_player.y - 1) * 32, &imageWall);
	//		else if (gettiles(_player.x - 2, _player.y - 1) == ClosedDoor)
	//			putimage((_player.x - 2) * 32, (_player.y - 1) * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x - 2, _player.y - 1) == OpenDoor)
	//			putimage((_player.x - 2) * 32, (_player.y - 1) * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x - 2, _player.y - 1) == UpStairs)
	//			putimage((_player.x - 2) * 32, (_player.y - 1) * 32, &imageUpstair);
	//		else if (gettiles(_player.x - 2, _player.y - 1) == StairsDown)
	//			putimage((_player.x - 2) * 32, (_player.y - 1) * 32, &imageDownstair);
	//		else if (gettiles(_player.x - 2, _player.y - 1) == Player)
	//			putimage((_player.x - 2) * 32, (_player.y - 1) * 32, &imagePlayer);
	//		else if (gettiles(_player.x - 2, _player.y - 1) == Monster)
	//			putimage((_player.x - 2) * 32, (_player.y - 1) * 32, &imageMonster);
	//		else if (gettiles(_player.x - 2, _player.y - 1) == Chest)
	//			putimage((_player.x - 2) * 32, (_player.y - 1) * 32, &imageChest);

	//		if (gettiles(_player.x - 1, _player.y - 2) == Road)
	//			;
	//		else if (gettiles(_player.x - 1, _player.y - 2) == Road)
	//			putimage((_player.x - 1) * 32, (_player.y - 2) * 32, &imageFloor);
	//		else if (gettiles(_player.x - 1, _player.y - 2) == Road)
	//			putimage((_player.x - 1) * 32, (_player.y - 2) * 32, &imageCorridor);
	//		else if (gettiles(_player.x - 1, _player.y - 2) == Wall)
	//			putimage((_player.x - 1) * 32, (_player.y - 2) * 32, &imageWall);
	//		else if (gettiles(_player.x - 1, _player.y - 2) == ClosedDoor)
	//			putimage((_player.x - 1) * 32, (_player.y - 2) * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x - 1, _player.y - 2) == OpenDoor)
	//			putimage((_player.x - 1) * 32, (_player.y - 2) * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x - 1, _player.y - 2) == UpStairs)
	//			putimage((_player.x - 1) * 32, (_player.y - 2) * 32, &imageUpstair);
	//		else if (gettiles(_player.x - 1, _player.y - 2) == StairsDown)
	//			putimage((_player.x - 1) * 32, (_player.y - 2) * 32, &imageDownstair);
	//		else if (gettiles(_player.x - 1, _player.y - 2) == Player)
	//			putimage((_player.x - 1) * 32, (_player.y - 2) * 32, &imagePlayer);
	//		else if (gettiles(_player.x - 1, _player.y - 2) == Monster)
	//			putimage((_player.x - 1) * 32, (_player.y - 2) * 32, &imageMonster);
	//		else if (gettiles(_player.x - 1, _player.y - 2) == Chest)
	//			putimage((_player.x - 1) * 32, (_player.y - 2) * 32, &imageChest);
	//	}
	//	if (gettiles(_player.x, _player.y - 1) != Wall && gettiles(_player.x, _player.y - 1) != ClosedDoor)//North
	//	{
	//		if (gettiles(_player.x, _player.y - 2) == Road)
	//			;
	//		else if (gettiles(_player.x, _player.y - 2) == Road)
	//			putimage(_player.x * 32, (_player.y - 2) * 32, &imageFloor);
	//		else if (gettiles(_player.x, _player.y - 2) == Road)
	//			putimage(_player.x * 32, (_player.y - 2) * 32, &imageCorridor);
	//		else if (gettiles(_player.x, _player.y - 2) == Wall)
	//			putimage(_player.x * 32, (_player.y - 2) * 32, &imageWall);
	//		else if (gettiles(_player.x, _player.y - 2) == ClosedDoor)
	//			putimage(_player.x * 32, (_player.y - 2) * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x, _player.y - 2) == OpenDoor)
	//			putimage(_player.x * 32, (_player.y - 2) * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x, _player.y - 2) == UpStairs)
	//			putimage(_player.x * 32, (_player.y - 2) * 32, &imageUpstair);
	//		else if (gettiles(_player.x, _player.y - 2) == StairsDown)
	//			putimage(_player.x * 32, (_player.y - 2) * 32, &imageDownstair);
	//		else if (gettiles(_player.x, _player.y - 2) == Player)
	//			putimage(_player.x * 32, (_player.y - 2) * 32, &imagePlayer);
	//		else if (gettiles(_player.x, _player.y - 2) == Monster)
	//			putimage(_player.x * 32, (_player.y - 2) * 32, &imageMonster);
	//		else if (gettiles(_player.x, _player.y - 2) == Chest)
	//			putimage(_player.x * 32, (_player.y - 2) * 32, &imageChest);

	//		if (gettiles(_player.x, _player.y - 2) != Wall && gettiles(_player.x, _player.y - 2) != ClosedDoor)
	//		{
	//			if (gettiles(_player.x, _player.y - 3) == Road)
	//				;
	//			else if (gettiles(_player.x, _player.y - 3) == Road)
	//				putimage(_player.x * 32, (_player.y - 3)* 32, &imageFloor);
	//			else if (gettiles(_player.x, _player.y - 3) == Road)
	//				putimage(_player.x * 32, (_player.y - 3) * 32, &imageCorridor);
	//			else if (gettiles(_player.x, _player.y - 3) == Wall)
	//				putimage(_player.x * 32, (_player.y - 3) * 32, &imageWall);
	//			else if (gettiles(_player.x, _player.y - 3) == ClosedDoor)
	//				putimage(_player.x * 32, (_player.y - 3) * 32, &imageClosedDoor);
	//			else if (gettiles(_player.x, _player.y - 3) == OpenDoor)
	//				putimage(_player.x * 32, (_player.y - 3) * 32, &imageOpenDoor);
	//			else if (gettiles(_player.x, _player.y - 3) == UpStairs)
	//				putimage(_player.x * 32, (_player.y - 3) * 32, &imageUpstair);
	//			else if (gettiles(_player.x, _player.y - 3) == StairsDown)
	//				putimage(_player.x * 32, (_player.y - 3) * 32, &imageDownstair);
	//			else if (gettiles(_player.x, _player.y - 3) == Player)
	//				putimage(_player.x * 32, (_player.y - 3) * 32, &imagePlayer);
	//			else if (gettiles(_player.x, _player.y - 3) == Monster)
	//				putimage(_player.x * 32, (_player.y - 3) * 32, &imageMonster);
	//			else if (gettiles(_player.x, _player.y - 3) == Chest)
	//				putimage(_player.x * 32, (_player.y - 3) * 32, &imageChest);
	//		}
	//	}
	//	if (gettiles(_player.x + 1, _player.y - 1) != Wall && gettiles(_player.x + 1, _player.y - 1) != ClosedDoor)//Northeast
	//	{
	//		if (gettiles(_player.x + 2, _player.y - 1) == Road)
	//			;
	//		else if (gettiles(_player.x + 2, _player.y - 1) == Road)
	//			putimage((_player.x + 2) * 32, (_player.y - 1) * 32, &imageFloor);
	//		else if (gettiles(_player.x + 2, _player.y - 1) == Road)
	//			putimage((_player.x + 2) * 32, (_player.y - 1) * 32, &imageCorridor);
	//		else if (gettiles(_player.x + 2, _player.y - 1) == Wall)
	//			putimage((_player.x + 2) * 32, (_player.y - 1) * 32, &imageWall);
	//		else if (gettiles(_player.x + 2, _player.y - 1) == ClosedDoor)
	//			putimage((_player.x + 2) * 32, (_player.y - 1) * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x + 2, _player.y - 1) == OpenDoor)
	//			putimage((_player.x + 2) * 32, (_player.y - 1) * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x + 2, _player.y - 1) == UpStairs)
	//			putimage((_player.x + 2) * 32, (_player.y - 1) * 32, &imageUpstair);
	//		else if (gettiles(_player.x + 2, _player.y - 1) == StairsDown)
	//			putimage((_player.x + 2) * 32, (_player.y - 1) * 32, &imageDownstair);
	//		else if (gettiles(_player.x + 2, _player.y - 1) == Player)
	//			putimage((_player.x + 2) * 32, (_player.y - 1) * 32, &imagePlayer);
	//		else if (gettiles(_player.x + 2, _player.y - 1) == Monster)
	//			putimage((_player.x + 2) * 32, (_player.y - 1) * 32, &imageMonster);
	//		else if (gettiles(_player.x + 2, _player.y - 1) == Chest)
	//			putimage((_player.x + 2) * 32, (_player.y - 1) * 32, &imageChest);

	//		if (gettiles(_player.x + 1, _player.y - 2) == Road)
	//			;
	//		else if (gettiles(_player.x + 1, _player.y - 2) == Road)
	//			putimage((_player.x + 1) * 32, (_player.y - 2) * 32, &imageFloor);
	//		else if (gettiles(_player.x + 1, _player.y - 2) == Road)
	//			putimage((_player.x + 1) * 32, (_player.y - 2) * 32, &imageCorridor);
	//		else if (gettiles(_player.x + 1, _player.y - 2) == Wall)
	//			putimage((_player.x + 1) * 32, (_player.y - 2) * 32, &imageWall);
	//		else if (gettiles(_player.x + 1, _player.y - 2) == ClosedDoor)
	//			putimage((_player.x + 1) * 32, (_player.y - 2) * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x + 1, _player.y - 2) == OpenDoor)
	//			putimage((_player.x + 1) * 32, (_player.y - 2) * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x + 1, _player.y - 2) == UpStairs)
	//			putimage((_player.x + 1) * 32, (_player.y - 2) * 32, &imageUpstair);
	//		else if (gettiles(_player.x + 1, _player.y - 2) == StairsDown)
	//			putimage((_player.x + 1) * 32, (_player.y - 2) * 32, &imageDownstair);
	//		else if (gettiles(_player.x + 1, _player.y - 2) == Player)
	//			putimage((_player.x + 1) * 32, (_player.y - 2) * 32, &imagePlayer);
	//		else if (gettiles(_player.x + 1, _player.y - 2) == Monster)
	//			putimage((_player.x + 1) * 32, (_player.y - 2) * 32, &imageMonster);
	//		else if (gettiles(_player.x + 1, _player.y - 2) == Chest)
	//			putimage((_player.x + 1) * 32, (_player.y - 2) * 32, &imageChest);
	//	}
	//	if (gettiles(_player.x + 1, _player.y) != Wall && gettiles(_player.x + 1, _player.y) != ClosedDoor)//East
	//	{
	//		if (gettiles(_player.x + 2, _player.y) == Road)
	//			;
	//		else if (gettiles(_player.x + 2, _player.y) == Road)
	//			putimage((_player.x + 2) * 32, _player.y * 32, &imageFloor);
	//		else if (gettiles(_player.x + 2, _player.y) == Road)
	//			putimage((_player.x + 2) * 32, _player.y * 32, &imageCorridor);
	//		else if (gettiles(_player.x + 2, _player.y) == Wall)
	//			putimage((_player.x + 2) * 32, _player.y * 32, &imageWall);
	//		else if (gettiles(_player.x + 2, _player.y) == ClosedDoor)
	//			putimage((_player.x + 2) * 32, _player.y * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x + 2, _player.y) == OpenDoor)
	//			putimage((_player.x + 2) * 32, _player.y * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x + 2, _player.y) == UpStairs)
	//			putimage((_player.x + 2) * 32, _player.y * 32, &imageUpstair);
	//		else if (gettiles(_player.x + 2, _player.y) == StairsDown)
	//			putimage((_player.x + 2) * 32, _player.y * 32, &imageDownstair);
	//		else if (gettiles(_player.x + 2, _player.y) == Player)
	//			putimage((_player.x + 2) * 32, _player.y * 32, &imagePlayer);
	//		else if (gettiles(_player.x + 2, _player.y) == Monster)
	//			putimage((_player.x + 2) * 32, _player.y * 32, &imageMonster);
	//		else if (gettiles(_player.x + 2, _player.y) == Chest)
	//			putimage((_player.x + 2) * 32, _player.y * 32, &imageChest);

	//		if (gettiles(_player.x + 2, _player.y) != Wall && gettiles(_player.x + 2, _player.y) != ClosedDoor)
	//		{
	//			if (gettiles(_player.x + 3, _player.y) == Road)
	//				;
	//			else if (gettiles(_player.x + 3, _player.y) == Road)
	//				putimage((_player.x + 3) * 32, _player.y * 32, &imageFloor);
	//			else if (gettiles(_player.x + 3, _player.y) == Road)
	//				putimage((_player.x + 3) * 32, _player.y * 32, &imageCorridor);
	//			else if (gettiles(_player.x + 3, _player.y) == Wall)
	//				putimage((_player.x + 3) * 32, _player.y * 32, &imageWall);
	//			else if (gettiles(_player.x + 3, _player.y) == ClosedDoor)
	//				putimage((_player.x + 3) * 32, _player.y * 32, &imageClosedDoor);
	//			else if (gettiles(_player.x + 3, _player.y) == OpenDoor)
	//				putimage((_player.x + 3) * 32, _player.y * 32, &imageOpenDoor);
	//			else if (gettiles(_player.x + 3, _player.y) == UpStairs)
	//				putimage((_player.x + 3) * 32, _player.y * 32, &imageUpstair);
	//			else if (gettiles(_player.x + 3, _player.y) == StairsDown)
	//				putimage((_player.x + 3) * 32, _player.y * 32, &imageDownstair);
	//			else if (gettiles(_player.x + 3, _player.y) == Player)
	//				putimage((_player.x + 3) * 32, _player.y * 32, &imagePlayer);
	//			else if (gettiles(_player.x + 3, _player.y) == Monster)
	//				putimage((_player.x + 3) * 32, _player.y * 32, &imageMonster);
	//			else if (gettiles(_player.x + 3, _player.y) == Chest)
	//				putimage((_player.x + 3) * 32, _player.y * 32, &imageChest);
	//		}
	//	}
	//	if (gettiles(_player.x + 1, _player.y + 1) != Wall && gettiles(_player.x + 1, _player.y + 1) != ClosedDoor)//Southeast
	//	{
	//		if (gettiles(_player.x + 2, _player.y + 1) == Road)
	//			;
	//		else if (gettiles(_player.x + 2, _player.y + 1) == Road)
	//			putimage((_player.x + 2) * 32, (_player.y + 1) * 32, &imageFloor);
	//		else if (gettiles(_player.x + 2, _player.y + 1) == Road)
	//			putimage((_player.x + 2) * 32, (_player.y + 1) * 32, &imageCorridor);
	//		else if (gettiles(_player.x + 2, _player.y + 1) == Wall)
	//			putimage((_player.x + 2) * 32, (_player.y + 1) * 32, &imageWall);
	//		else if (gettiles(_player.x + 2, _player.y + 1) == ClosedDoor)
	//			putimage((_player.x + 2) * 32, (_player.y + 1) * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x + 2, _player.y + 1) == OpenDoor)
	//			putimage((_player.x + 2) * 32, (_player.y + 1) * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x + 2, _player.y + 1) == UpStairs)
	//			putimage((_player.x + 2) * 32, (_player.y + 1) * 32, &imageUpstair);
	//		else if (gettiles(_player.x + 2, _player.y + 1) == StairsDown)
	//			putimage((_player.x + 2) * 32, (_player.y + 1) * 32, &imageDownstair);
	//		else if (gettiles(_player.x + 2, _player.y + 1) == Player)
	//			putimage((_player.x + 2) * 32, (_player.y + 1) * 32, &imagePlayer);
	//		else if (gettiles(_player.x + 2, _player.y + 1) == Monster)
	//			putimage((_player.x + 2) * 32, (_player.y + 1) * 32, &imageMonster);
	//		else if (gettiles(_player.x + 2, _player.y + 1) == Chest)
	//			putimage((_player.x + 2) * 32, (_player.y + 1) * 32, &imageChest);

	//		if (gettiles(_player.x + 1, _player.y + 2) == Road)
	//			;
	//		else if (gettiles(_player.x + 1, _player.y + 2) == Road)
	//			putimage((_player.x + 1) * 32, (_player.y + 2) * 32, &imageFloor);
	//		else if (gettiles(_player.x + 1, _player.y + 2) == Road)
	//			putimage((_player.x + 1) * 32, (_player.y + 2) * 32, &imageCorridor);
	//		else if (gettiles(_player.x + 1, _player.y + 2) == Wall)
	//			putimage((_player.x + 1) * 32, (_player.y + 2) * 32, &imageWall);
	//		else if (gettiles(_player.x + 1, _player.y + 2) == ClosedDoor)
	//			putimage((_player.x + 1) * 32, (_player.y + 2) * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x + 1, _player.y + 2) == OpenDoor)
	//			putimage((_player.x + 1) * 32, (_player.y + 2) * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x + 1, _player.y + 2) == UpStairs)
	//			putimage((_player.x + 1) * 32, (_player.y + 2) * 32, &imageUpstair);
	//		else if (gettiles(_player.x + 1, _player.y + 2) == StairsDown)
	//			putimage((_player.x + 1) * 32, (_player.y + 2) * 32, &imageDownstair);
	//		else if (gettiles(_player.x + 1, _player.y + 2) == Player)
	//			putimage((_player.x + 1) * 32, (_player.y + 2) * 32, &imagePlayer);
	//		else if (gettiles(_player.x + 1, _player.y + 2) == Monster)
	//			putimage((_player.x + 1) * 32, (_player.y + 2) * 32, &imageMonster);
	//		else if (gettiles(_player.x + 1, _player.y + 2) == Chest)
	//			putimage((_player.x + 1) * 32, (_player.y + 2) * 32, &imageChest);
	//	}
	//	if (gettiles(_player.x, _player.y + 1) != Wall && gettiles(_player.x, _player.y + 1) != ClosedDoor)//South
	//	{
	//		if (gettiles(_player.x, _player.y + 2) == Road)
	//			;
	//		else if (gettiles(_player.x, _player.y + 2) == Road)
	//			putimage(_player.x * 32, (_player.y + 2) * 32, &imageFloor);
	//		else if (gettiles(_player.x, _player.y + 2) == Road)
	//			putimage(_player.x * 32, (_player.y + 2) * 32, &imageCorridor);
	//		else if (gettiles(_player.x, _player.y + 2) == Wall)
	//			putimage(_player.x * 32, (_player.y + 2) * 32, &imageWall);
	//		else if (gettiles(_player.x, _player.y + 2) == ClosedDoor)
	//			putimage(_player.x * 32, (_player.y + 2) * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x, _player.y + 2) == OpenDoor)
	//			putimage(_player.x * 32, (_player.y + 2) * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x, _player.y + 2) == UpStairs)
	//			putimage(_player.x * 32, (_player.y + 2) * 32, &imageUpstair);
	//		else if (gettiles(_player.x, _player.y + 2) == StairsDown)
	//			putimage(_player.x * 32, (_player.y + 2) * 32, &imageDownstair);
	//		else if (gettiles(_player.x, _player.y + 2) == Player)
	//			putimage(_player.x * 32, (_player.y + 2) * 32, &imagePlayer);
	//		else if (gettiles(_player.x, _player.y + 2) == Monster)
	//			putimage(_player.x * 32, (_player.y + 2) * 32, &imageMonster);
	//		else if (gettiles(_player.x, _player.y + 2) == Chest)
	//			putimage(_player.x * 32, (_player.y + 2) * 32, &imageChest);

	//		if (gettiles(_player.x, _player.y + 2) != Wall && gettiles(_player.x, _player.y + 2) != ClosedDoor)
	//		{
	//			if (gettiles(_player.x, _player.y + 3) == Road)
	//				;
	//			else if (gettiles(_player.x, _player.y + 3) == Road)
	//				putimage(_player.x * 32, (_player.y + 3) * 32, &imageFloor);
	//			else if (gettiles(_player.x, _player.y + 3) == Road)
	//				putimage(_player.x * 32, (_player.y + 3) * 32, &imageCorridor);
	//			else if (gettiles(_player.x, _player.y + 3) == Wall)
	//				putimage(_player.x * 32, (_player.y + 3) * 32, &imageWall);
	//			else if (gettiles(_player.x, _player.y + 3) == ClosedDoor)
	//				putimage(_player.x * 32, (_player.y + 3) * 32, &imageClosedDoor);
	//			else if (gettiles(_player.x, _player.y + 3) == OpenDoor)
	//				putimage(_player.x * 32, (_player.y + 3) * 32, &imageOpenDoor);
	//			else if (gettiles(_player.x, _player.y + 3) == UpStairs)
	//				putimage(_player.x * 32, (_player.y + 3) * 32, &imageUpstair);
	//			else if (gettiles(_player.x, _player.y + 3) == StairsDown)
	//				putimage(_player.x * 32, (_player.y + 3) * 32, &imageDownstair);
	//			else if (gettiles(_player.x, _player.y + 3) == Player)
	//				putimage(_player.x * 32, (_player.y + 3) * 32, &imagePlayer);
	//			else if (gettiles(_player.x, _player.y + 3) == Monster)
	//				putimage(_player.x * 32, (_player.y + 3) * 32, &imageMonster);
	//			else if (gettiles(_player.x, _player.y + 3) == Chest)
	//				putimage(_player.x * 32, (_player.y + 3) * 32, &imageChest);
	//		}
	//	}
	//	if (gettiles(_player.x - 1, _player.y + 1) != Wall && gettiles(_player.x - 1, _player.y + 1) != ClosedDoor)//Southwest
	//	{
	//		if (gettiles(_player.x - 2, _player.y + 1) == Road)
	//			;
	//		else if (gettiles(_player.x - 2, _player.y + 1) == Road)
	//			putimage((_player.x - 2) * 32, (_player.y + 1) * 32, &imageFloor);
	//		else if (gettiles(_player.x - 2, _player.y + 1) == Road)
	//			putimage((_player.x - 2) * 32, (_player.y + 1) * 32, &imageCorridor);
	//		else if (gettiles(_player.x - 2, _player.y + 1) == Wall)
	//			putimage((_player.x - 2) * 32, (_player.y + 1) * 32, &imageWall);
	//		else if (gettiles(_player.x - 2, _player.y + 1) == ClosedDoor)
	//			putimage((_player.x - 2) * 32, (_player.y + 1) * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x - 2, _player.y + 1) == OpenDoor)
	//			putimage((_player.x - 2) * 32, (_player.y + 1) * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x - 2, _player.y + 1) == UpStairs)
	//			putimage((_player.x - 2) * 32, (_player.y + 1) * 32, &imageUpstair);
	//		else if (gettiles(_player.x - 2, _player.y + 1) == StairsDown)
	//			putimage((_player.x - 2) * 32, (_player.y + 1) * 32, &imageDownstair);
	//		else if (gettiles(_player.x - 2, _player.y + 1) == Player)
	//			putimage((_player.x - 2) * 32, (_player.y + 1) * 32, &imagePlayer);
	//		else if (gettiles(_player.x - 2, _player.y + 1) == Monster)
	//			putimage((_player.x - 2) * 32, (_player.y + 1) * 32, &imageMonster);
	//		else if (gettiles(_player.x - 2, _player.y + 1) == Chest)
	//			putimage((_player.x - 2) * 32, (_player.y + 1) * 32, &imageChest);

	//		if (gettiles(_player.x - 1, _player.y + 2) == Road)
	//			;
	//		else if (gettiles(_player.x - 1, _player.y + 2) == Road)
	//			putimage((_player.x - 1) * 32, (_player.y + 2) * 32, &imageFloor);
	//		else if (gettiles(_player.x - 1, _player.y + 2) == Road)
	//			putimage((_player.x - 1) * 32, (_player.y + 2) * 32, &imageCorridor);
	//		else if (gettiles(_player.x - 1, _player.y + 2) == Wall)
	//			putimage((_player.x - 1) * 32, (_player.y + 2) * 32, &imageWall);
	//		else if (gettiles(_player.x - 1, _player.y + 2) == ClosedDoor)
	//			putimage((_player.x - 1) * 32, (_player.y + 2) * 32, &imageClosedDoor);
	//		else if (gettiles(_player.x - 1, _player.y + 2) == OpenDoor)
	//			putimage((_player.x - 1) * 32, (_player.y + 2) * 32, &imageOpenDoor);
	//		else if (gettiles(_player.x - 1, _player.y + 2) == UpStairs)
	//			putimage((_player.x - 1) * 32, (_player.y + 2) * 32, &imageUpstair);
	//		else if (gettiles(_player.x - 1, _player.y + 2) == StairsDown)
	//			putimage((_player.x - 1) * 32, (_player.y + 2) * 32, &imageDownstair);
	//		else if (gettiles(_player.x - 1, _player.y + 2) == Player)
	//			putimage((_player.x - 1) * 32, (_player.y + 2) * 32, &imagePlayer);
	//		else if (gettiles(_player.x - 1, _player.y + 2) == Monster)
	//			putimage((_player.x - 1) * 32, (_player.y + 2) * 32, &imageMonster);
	//		else if (gettiles(_player.x - 1, _player.y + 2) == Chest)
	//			putimage((_player.x - 1) * 32, (_player.y + 2) * 32, &imageChest);
	//	}

	//演示用

	for (int y = 0; y < height; ++y)//九宫格 
	{
		for (int x = 0; x < width; ++x)
		{
			//unpassable
			/*if (gettiles(x, y) == ClosedDoor)
				putimage(x * 32, y * 32, &imageChest);
			else if (gettiles(x, y) == Road)
				putimage(x * 32, y * 32, &imageFloor);
			else if (gettiles(x, y) == Road)
				putimage(x * 32, y * 32, &imageCorridor);
			else if (gettiles(x, y) == Wall)
				putimage(x * 32, y * 32, &imageWall);
			else if (gettiles(x, y) == ClosedDoor)
				putimage(x * 32, y * 32, &imageClosedDoor);
			else if (gettiles(x, y) == OpenDoor)
				putimage(x * 32, y * 32, &imageOpenDoor);
			else if (gettiles(x, y) == UpStairs)
				putimage(x * 32, y * 32, &imageUpstair);
			else if (gettiles(x, y) == StairsDown)
				putimage(x * 32, y * 32, &imageDownstair);
			else if (gettiles(x, y) == Player)
				putimage(x * 32, y * 32, &imagePlayer);
			else if (gettiles(x, y) == Monster)
				putimage(x * 32, y * 32, &imageMonster);*/

			std::pair<int, int> tile = gettiles(x, y);
			putimage(x * 32, y * 32, &imageMap[tile]);
		}


	}

	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 25;
	wcscpy_s(f.lfFaceName, L"微软雅黑 Light");
	f.lfQuality = ANTIALIASED_QUALITY;
	settextstyle(&f);
	settextcolor(WHITE);
	outtextxy(46 * 32, 1 * 32, L"HP:");
	wchar_t HP[50];
	swprintf_s(HP, L"%d\0", player.HP);
	outtextxy(46 * 32, 2 * 32, HP);

	outtextxy(46 * 32, 3 * 32, L"攻击:");
	wchar_t atk[50];
	swprintf_s(atk, L"%d\0", player.atk);
	outtextxy(46 * 32, 4 * 32, atk);

	outtextxy(46 * 32, 5 * 32, L"防御:");
	wchar_t dfc[50];
	swprintf_s(dfc, L"%d\0", player.dfc);
	outtextxy(46 * 32, 6 * 32, dfc);

	outtextxy(46 * 32, 7 * 32, L"分数:");
	wchar_t score[50];
	swprintf_s(score, L"%d\0", player.score);
	outtextxy(46 * 32, 8 * 32, score);

	outtextxy(46 * 32, 9 * 32, L"层数:");
	wchar_t level[50];
	swprintf_s(level, L"%d\0", player.level);
	outtextxy(46 * 32, 10 * 32, level);





}

void StringToWstring_CRT(const std::string& str, std::wstring& wstr)
{
	std::string curLocale = setlocale(LC_ALL, NULL); //curLocale = "C"
	setlocale(LC_ALL, "chs");
	int nLen = str.size() + 1;
	mbstowcs((wchar_t*)wstr.c_str(), str.c_str(), nLen);
	setlocale(LC_ALL, curLocale.c_str());

	return;
}
void WstringToString_CRT(const std::wstring& wstr, std::string& str)
{
	std::string curLocale = setlocale(LC_ALL, NULL); //curLocale = "C"
	setlocale(LC_ALL, "chs");
	int nLen = wstr.size() + 1;
	wcstombs((char*)str.c_str(), wstr.c_str(), nLen);
	setlocale(LC_ALL, curLocale.c_str());

	return;
}
