#include "Functions.h"
#include "Play.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>

//Functions source file for board functions.

//Loads a board for a new level.
//These functions return override ints, so that I can return dynamic arrays.
int** GetBoard()
{
	return GenerateBoard(LoadBoard());
}

//Loads a board template from file.
int** LoadBoard()
{
	//Sets up an array for the board.
	int** array = new int*[16];
	for (int i = 0; i < 16; i++)
	{
		array[i] = new int[14];
	}
	//Open the default file to read from.
	std::fstream file;
	file.open("default.txt", std::ios::in);
	if (file.is_open())
	{
		int j = 0;
		std::string line;
		//Read file line by line.
		while (std::getline(file, line))
		{
			for (int k = 0; k < 14; k++)
			{
				//For each character in line, store each character as an element in an arrray. Then move onto next line when finished.
				array[j][k] = std::stoi(std::to_string(line[k])) - 48;
			}
			j++;
		}
		//Close file.
		file.close();
	}
	//Return the new array to be used as a level.
	return array;
}

//Randomly generates enemy spawn points on the given board.
int** GenerateBoard(int** array)
{
	int x;
	int y;
	int enemies = 5;
	bool generateEnemySpawn;
	//Spawns 5 enemies.
	while (enemies > 0)
	{
		generateEnemySpawn = true;
		//Generate X and Y coordinates, using system time as a seed.
		srand(time(0));
		x = rand() % 14;
		srand(time(0));
		y = rand() % 16;
		//If statement to stop enemies from generating above ground.
		if (y < 2)
		{
			generateEnemySpawn = false;
		}
		else
		{
			//Check if their are any existing tunnels surrounding the spawn point for the enemy.
			//We want to generate a 3 long tunnel for enemies to spawn in, so we need to check the 3x3 area around each of the three squares in the potential tunnel.
			for (int i = x; i < x + 3; i++)
			{
				for (int j = y - 1; j < y + 2; j++)
				{
					for (int k = i - 1; k < i + 2; k++)
					{
						//If out of bounds or tunnel exists,do not generate enemy spawn.
						if (j < 0 || j > 15 || k < 0 || k > 14)
						{
							generateEnemySpawn = false;
						}
						else 
						{
							if (array[j][k] != 1)
							{
								generateEnemySpawn = false;
							}
						}
					}
				}
			}
		}
		//If valid location, spawn an enemy.
		if (generateEnemySpawn)
		{
			array[y][x] = 7;
			array[y][x+1] = 0;
			array[y][x+2] = 0;
			enemies--;
		}
		//Process repeats until 5 enemies have been generated.
	}
	return array;
}

//Draws the board to the screen.
void DrawBoard(int** array)
{
	int spriteID;
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 14; j++)
		{
			spriteID = array[i][j];
			if (i == 0)
			{
				spriteID = 5;
			}
			else if (spriteID > 5)
			{
				spriteID = 0;
			}
			else if (spriteID != 0)
			{
				if (i >= 5 && i < 9)
				{
					spriteID += 1;
				}
				else if (i >= 9 && i < 13)
				{
					spriteID += 2;
				}
				else if (i >= 13)
				{
					spriteID += 3;
				}
			}
			
			Play::DrawSprite(1, Point2D(j*32+32, i*32+32), spriteID);
		}
	}
}