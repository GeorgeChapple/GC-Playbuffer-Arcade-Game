#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"
#include "Functions.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
int DISPLAY_WIDTH = 650;
int DISPLAY_HEIGHT = 800;
int DISPLAY_SCALE = 1;

int** boardArray; //Stores the current level.
int enemyDirectionArray[5] = { 0, 0, 0, 0, 0 }; //Stores the directions that the enemies are moving in.
bool generate = true; //Determines if the game should generate a new stage.

struct GameState
{
	int score = 0;
	int stage = 1;
	int lives = 3;
	int enemiesRemaining;
};

struct PlayerState
{
	int direction = 0;
	int nearestX = 0;
	int nearestY = 0;
};

struct AttackState
{
	int direction = 0;
	int nearestX = 0;
	int nearestY = 0;
	bool active = false;
};

enum GameObjectType
{
	TYPE_PLAYER,
	TYPE_ATTACK,
	TYPE_ENEMY,
};

GameState gameState;
PlayerState playerState;
AttackState attackState;

void Restart();        //Restarts game after losing 3 lives.
void PlayerControls(); //Manages player movement and attacking.
void PlayerSprite();   //Manages drawing the player sprite to screen.
void PlayerAttack();   //Manages the player's laser ball attack.
void PlaceEnemies();   //Places enemy game objects at board spawn points.
void UpdateEnemies();  //Manages the enemies, their collisions, movements and lose conditions.

// The entry point for a PlayBuffer program
void MainGameEntry(PLAY_IGNORE_COMMAND_LINE)
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	//Play::CentreAllSpriteOrigins();
	Play::CreateGameObject(TYPE_PLAYER, Point2D(0, 0), 7, "Drill");	      //Creates the player.
	Play::CreateGameObject(TYPE_ATTACK, Point2D(0, 0), 7, "DrillAttack"); //Creates the attack.
}

// Called by the PlayBuffer once every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	Play::ClearDrawingBuffer(Play::cWhite); 
	Play::DrawSprite(0, Point2D(0, 0), 0); //Background
	//Run the game if player has lives.
	if (gameState.lives != 0)
	{
		//Display generate sprite if generating level.
		if (generate)
		{
			Play::DrawSprite(6, Point2D(DISPLAY_WIDTH / 2 - 128, DISPLAY_HEIGHT / 2 - 128), 0);
		}
		//Else run game...
		else
		{
			//If player moves into a dirt tile, make tunnel and give points.
			if (boardArray[playerState.nearestY][playerState.nearestX] != 0)
			{
				boardArray[playerState.nearestY][playerState.nearestX] = 0;
				gameState.score += 10;
			}
			DrawBoard(boardArray); //Draw board to screen.
			PlayerControls();      //Manage player controls/movement.
			PlayerSprite();        //Draw player to screen.
			//If player presses attack button.
			if (attackState.active)
			{
				PlayerAttack(); //Attack!
			}
			//Else keep attack laser ball at play pos.
			else
			{
				Play::GetGameObjectByType(TYPE_ATTACK).pos = Play::GetGameObjectByType(TYPE_PLAYER).pos;
			}
			UpdateEnemies(); //Manage enemies...
			Play::DrawFontText("64px", "SCORE : " + std::to_string(gameState.score),{ 32 * 14, 17 * 32 }, Play::RIGHT); //Draws score number to screen.
			Play::DrawFontText("64px", "LIVES : " + std::to_string(gameState.lives),{ 32 * 14, 19 * 32 }, Play::RIGHT); //Draws lives number to screen.
			Play::DrawFontText("64px", "STAGE : " + std::to_string(gameState.stage),{ 32 * 14, 21 * 32}, Play::RIGHT);  //Draws stage number to screen.
			Play::DrawFontText("64px", "ARROW KEYS TO MOVE AND SPACE TO FIRE",{ 32 * 18, 23 * 32 }, Play::RIGHT);       //Draws instructions to screen.
		}
		//If game needs a new level...
		if (generate)
		{
			//Makes sure that generate sprite draws in front.
			Play::DrawSprite(6, Point2D(DISPLAY_WIDTH / 2 - 128, DISPLAY_HEIGHT / 2 - 128), 0);
		}
		Play::PresentDrawingBuffer(); //Present drawn screen.
		//If game needs a new level...
		if (generate)
		{
			//Reset player position.
			Play::GetGameObjectByType(TYPE_PLAYER).pos = Point2D(7 * 32 + 2, 9 * 32 + 2);
			//Generate a new level.
			boardArray = GetBoard();
			//Reset need for a new level.
			generate = false;
			//Put enemies at spawn points generated.
			PlaceEnemies();
			//Reset enemies remaining stat.
			gameState.enemiesRemaining = 5;
		}
	}
	//Else if game lost...
	else
	{
		//Prompt player to restart.
		Play::DrawDebugText({ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, "Game Over! Press SPACE to play again!");
		//Draw text to screen.
		Play::PresentDrawingBuffer();
		//Check for restart.
		Restart();
	}
	//Quit game.
	return Play::KeyDown(VK_ESCAPE);
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();
	return PLAY_OK;
}

void Restart()
{
	//If space key pressed, reset game.
	if (Play::KeyPressed(VK_SPACE))
	{
		gameState.lives = 3;
		gameState.stage = 1;
		gameState.score = 0;
		generate = true;
	}
}

void PlayerControls()
{
	GameObject& obj_player = Play::GetGameObjectByType(TYPE_PLAYER);
	//Store player screen coords.
	int x = obj_player.pos.x;
	int y = obj_player.pos.y;
	//Calculate the nearest tile the player is too.
	int tempInt = (x) / 32;
	double tempDouble = (x) / 32;
	if (tempInt + 0.5 > tempDouble)
	{
		playerState.nearestX = tempInt - 1;
	}
	else
	{
		playerState.nearestX = tempInt;
	}
	tempInt = (y) / 32;
	tempDouble = (y) / 32;
	if (tempInt + 0.5 > tempDouble)
	{
		playerState.nearestY = tempInt - 1;
	}
	else
	{
		playerState.nearestY = tempInt;
	}
	//Arrow controls for movement.
	//These move the player in the chosen direction.
	//Since the game operates on a grid, we check to see if the player is properly aligned with the grid.
	//If not, we move them in the opposing axis in the direction that gets them moving in the desired direction fastest.
	//This also changes and stores the last direction the player was facing for when they attack.
	if (Play::KeyDown(VK_UP) && y > 32)
	{
		if (x % 32 == 0)
		{
			obj_player.velocity = { 0, -2 };
			playerState.direction = 1;
		}
		else if (x % 32 > 16)
		{
			obj_player.velocity = { 2, 0 };
			playerState.direction = 2;
		}
		else
		{
			obj_player.velocity = { -2, 0 };
			playerState.direction = 0;
		}
	}
	else if (Play::KeyDown(VK_DOWN) && y < 512)
	{
		if (x % 32 == 0)
		{
			obj_player.velocity = { 0, 2 };
			playerState.direction = 3;
		}
		else if (x % 32 > 16)
		{
			obj_player.velocity = { 2, 0 };
			playerState.direction = 2;
		}
		else
		{
			obj_player.velocity = { -2, 0 };
			playerState.direction = 0;
		}
	}
	else if (Play::KeyDown(VK_LEFT) && x > 32)
	{
		if (y % 32 == 0)
		{
			obj_player.velocity = { -2, 0 };
			playerState.direction = 0;
		}
		else if (y % 32 > 16)
		{
			obj_player.velocity = { 0, 2 };
			playerState.direction = 3;
		}
		else
		{
			obj_player.velocity = { 0, -2 };
			playerState.direction = 1;
		}
	}
	else if (Play::KeyDown(VK_RIGHT) && x < 448)
	{
		if (y % 32 == 0)
		{
			obj_player.velocity = { 2, 0 };
			playerState.direction = 2;
		}
		else if (y % 32 > 16)
		{
			obj_player.velocity = { 0, 2 };
			playerState.direction = 3;
		}
		else
		{
			obj_player.velocity = { 0, -2 };
			playerState.direction = 1;
		}
	}
	//If no keys pressed, don't move.
	else
	{
		obj_player.velocity = { 0,0 };
		obj_player.acceleration = { 0,0 };
	}
	//If space pressed, attack!
	if (Play::KeyPressed(VK_SPACE) && !attackState.active)
	{
		attackState.direction = playerState.direction;
		attackState.active = true;
	}
	//Update player object.
	Play::UpdateGameObject(obj_player);
}

void PlayerSprite()
{
	GameObject& obj_player = Play::GetGameObjectByType(TYPE_PLAYER);
	//Draws the player sprite in the direction they are facing.
	Play::DrawSprite(2, obj_player.pos, playerState.direction);
}

void PlayerAttack()
{
	GameObject& obj_attack = Play::GetGameObjectByType(TYPE_ATTACK);
	//Stores laser ball attack position.
	int x = obj_attack.pos.x;
	int y = obj_attack.pos.y;
	//Calculates nearest position on board.
	int tempInt = (x) / 32;
	double tempDouble = (x) / 32;
	if (tempInt + 0.5 > tempDouble)
	{
		attackState.nearestX = tempInt - 1;
	}
	else
	{
		attackState.nearestX = tempInt;
	}
	tempInt = (y) / 32;
	tempDouble = (y) / 32;
	if (tempInt + 0.5 > tempDouble)
	{
		attackState.nearestY = tempInt - 1;
	}
	else
	{
		attackState.nearestY = tempInt;
	}
	//Moves attack in given direction.
	if (attackState.direction == 0)
	{
		obj_attack.velocity = { -4, 0 };
	}
	else if (attackState.direction == 1)
	{
		obj_attack.velocity = { 0, -4 };
	}
	else if (attackState.direction == 2)
	{
		obj_attack.velocity = { 4, 0 };
	}
	else if (attackState.direction == 3)
	{
		obj_attack.velocity = { 0, 4 };
	}
	//If out of bounds or hit a wall, end attack.
	if (x < 32 || x > 448 || y < 32 || y > 512)
	{
		attackState.active = false;
	}
	else if (boardArray[attackState.nearestY][attackState.nearestX] != 0)
	{
		attackState.active = false;
	}
	//Update objcet and draw sprite.
	Play::UpdateGameObject(obj_attack);
	Play::DrawSprite(3, obj_attack.pos, 0);
}

void PlaceEnemies()
{
	//Search for spawn points on board and place enemies there.
	for (int i = 0; i < 5; i++)
	{
		for (int i = 0; i < 16; i++)
		{
			for (int j = 0; j < 14; j++)
			{
				if (boardArray[i][j] == 7)
				{
					//Creates new enemy object.
					Play::CreateGameObject(TYPE_ENEMY, Point2D(j * 32 + 32, i * 32 + 32), 7, "Thing");
					boardArray[i][j] = 0;
				}
			}
		}
	}
}

void UpdateEnemies()
{
	//Store enemies in a vector.
	std::vector<int> vEnemies = Play::CollectGameObjectIDsByType(TYPE_ENEMY);
	int count = 0;
	//For each enemy...
	for (int id : vEnemies)
	{
		GameObject& obj_enemy = Play::GetGameObject(id);
		//Store object position.
		int x = obj_enemy.pos.x;
		int y = obj_enemy.pos.y;
		int tempInt = (x) / 32;
		double tempDouble = (x) / 32;
		int nearestX;
		int nearestY;
		//Grab direction.
		int direction = enemyDirectionArray[count];
		//Check if colliding with player.
		if (Play::IsColliding(obj_enemy, Play::GetGameObjectByType(TYPE_PLAYER)))
		{
			//Destroy all enemies.
			Play::DestroyGameObjectsByType(TYPE_ENEMY);
			//Stop any attacks.
			attackState.active = false;
			//Reset player position.
			playerState.nearestX = 0;
			playerState.nearestY = 0;
			//Reset board.
			boardArray = 0;
			//Remove a life.
			gameState.lives--;
			//Tell game to generate a new stage if player still has lives.
			if (gameState.lives > 0)
			{
				generate = true;
			}
			break;
		}
		//If attack hits enemy.
		if (Play::IsColliding(obj_enemy, Play::GetGameObjectByType(TYPE_ATTACK)) && attackState.active == true)
		{
			//End attack.
			attackState.active = false;
			//Give points.
			gameState.score += 500;
			//Destroy enemy object.
			Play::DestroyGameObject(id);
			//Tick down enemies remaining.
			gameState.enemiesRemaining--;
			//If no enemies left...
			if (gameState.enemiesRemaining < 1)
			{
				//Reset player pos.
				playerState.nearestX = 0;
				playerState.nearestY = 0;
				//Generate new stage.
				generate = true;
				gameState.stage++;
			}
		}
		//Calculate nearest position on board.
		if (tempInt + 0.5 > tempDouble)
		{
			nearestX = tempInt - 1;
		}
		else
		{
			nearestX = tempInt;
		}
		tempInt = (y) / 32;
		tempDouble = (y) / 32;
		if (tempInt + 0.5 > tempDouble)
		{
			nearestY = tempInt - 1;
		}
		else
		{
			nearestY = tempInt;
		}
		//Check if there is no path in current direction, if not, change to next direction.
		if (direction == 0)
		{
			if (nearestX - 1 >= 0)
			{
				if (boardArray[nearestY][nearestX - 1] == 1 || (obj_enemy.pos.x) < 33)
				{
					direction = 1;
				}
			}
		}
		if (direction == 1)
		{
			if (nearestY - 1 >= 0)
			{
				if (boardArray[nearestY - 1][nearestX] == 1 || (obj_enemy.pos.y) < 33)
				{
					direction = 2;
				}
			}
		}
		if (direction == 2)
		{
			if (nearestX + 1 < 14)
			{
				if (boardArray[nearestY][nearestX + 1] == 1 || (obj_enemy.pos.x) > 447)
				{
					direction = 3;
				}
			}
		}
		if (direction == 3)
		{
			if (nearestY + 1 < 16)
			{
				if (boardArray[nearestY + 1][nearestX] == 1 || (obj_enemy.pos.y) > 511)
				{
					direction = 0;
				}
			}
		}
		//If no direction, give direction.
		if (direction == -1)
		{
			direction = 2;
		}
		//Move enemy in given direction.
		if (direction == 0)
		{
			obj_enemy.pos.x--;
		}
		else if (direction == 1)
		{
			obj_enemy.pos.y--;
		}
		else if (direction == 2)
		{
			obj_enemy.pos.x++;
		}
		else if (direction == 3)
		{
			obj_enemy.pos.y++;
		}
		//Store enemy direction for next use.
		enemyDirectionArray[count] = direction;
		//Update and draw enemy.
		Play::UpdateGameObject(obj_enemy);
		Play::DrawSprite(7, obj_enemy.pos, 0);
		count++;
	}
}