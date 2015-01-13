#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <string>
#include <ctime>
#include <vector>

/* --- sfml remove console ---
Include the sfml-main.lib and sfml-main-d.lib 
Linker->System->SubSystem (change to windows) */

// window width/height
const int WIDTH = 250;
const int MENUWIDTH = 275;
const int HEIGHT = 550;

class Blocks : public sf::Drawable
{
private:
	struct Shape;
	struct Square;
	void draw(sf::RenderTarget& target, sf::RenderStates states) const;
	enum direct {LEFT, RIGHT, DOWN, ROTATE};
	bool shapeMove(direct dir);
	enum hitType {FLOOR, BLOCK, LWALL, RWALL, MISS};
	struct Cords;
	bool collision(direct dir);
	void removeRows();
	void updateBoard();
	void freeFall();

	struct Cords // used for current shape coordinates on the game board
	{
		Cords(int X, int Y, sf::Color col, bool piv, bool fill, int v) : x(X), y(Y), color(col), pivot(piv), filled(fill), val(v) { }
		int x, y, val;
		sf::Color color;
		bool filled;
		bool pivot;
	};

	struct Square
	{
		Square(bool fill, sf::Color col) : filled(fill), color(col) { }
		bool filled;
		sf::Color color;
	};

	std::vector<std::vector<Cords>> shapes;
	std::vector<Cords> curr;
	std::vector<std::vector<Cords>> next;
	std::vector<std::vector<Square>> board;		// TRUE = spot is taken, FALSE = spot is empty 
	std::vector<bool> clearRow;					// which rows to clear

	int boardWidth, boardHeight;

	std::vector<std::string> patterns;			// strings that hold the matrix values 0's and 1's
	int squareWidth, squareHeight;				// width and height of each block inside the shape

	sf::Clock clock;							// used for moving the shapes downward
	sf::Time speed;								// sets the speed at which the shapes "fall"
	sf::Time level[30];							// store the speed for each level
	int levelId;
	sf::Text levlTxt;

	sf::Clock clockLR;
	sf::Time speedLR;
	bool left, right;							// user moves the shape left or right

	bool falling;
	int alpha;
	sf::Clock animate;
	sf::Time animaDelay;

	int totalLines;
	sf::Text lines;
	int totalScore, currTotal;
	sf::Text score;
	int values[7];
	sf::Font font;

	int nextId;
	sf::Text worth;
	sf::Texture menuTexture;
	sf::Sprite menu;

	bool play, gameOver;
public:
	Blocks();
	void makeShape(int shape);
	void rotate(int i);
	void run(sf::Vector2i pos);
};

Blocks::Blocks()
{
	menuTexture.loadFromFile("menu.png");
	menu.setTexture(menuTexture);
	menu.setPosition(WIDTH+51, 0);

	alpha = 50;
	totalLines = totalScore = currTotal = levelId = 0;
	falling = false;
	animaDelay = sf::seconds(1);

	if(!font.loadFromFile("arial.ttf"))
	{
		std::cerr << "could not load font.";
		exit(EXIT_FAILURE);
	}

	worth.setFont(font);
	worth.setCharacterSize(18);
	worth.setColor(sf::Color(15,15,15));
	worth.setPosition(25*4+11+223, 105);

	score.setFont(font);
	score.setCharacterSize(18);
	score.setColor(sf::Color(15,15,15));
	score.setPosition(25*4+11+223, 130);
	
	lines.setFont(font);
	lines.setCharacterSize(18);
	lines.setColor(sf::Color(15,15,15));
	lines.setPosition(25*4+11+223, 155);

	levlTxt.setFont(font);
	levlTxt.setCharacterSize(18);
	levlTxt.setColor(sf::Color(15,15,15));
	levlTxt.setPosition(25*4+11+223, 180);

	squareWidth = squareHeight = 25;

	speedLR = sf::seconds(0.08);

	float tSpeed = 0.45;
	for (int i = 0; i < 30; i++)
	{
		level[i] = sf::seconds(tSpeed);
		tSpeed *= 0.95;
	}
	speed = level[levelId];

	boardHeight = 22;
	boardWidth = 10;

	for (int i = 0; i < boardHeight; i++) clearRow.push_back(false);
	
	for (int i = 0; i < boardHeight; i++)
		board.push_back(std::vector<Square>(boardWidth, Square(false, sf::Color::Black)));

	// first two characters are Width and Height, third is the pivot id in the strings bellow
	std::string tPatterns[7] = {"327100111", "327001111", "4141111", "2201111", "324110011", 
								"327010111", "324011110"};
	
	// Points
	values[0] = values[1] = 75; // L value's
	values[2] = 25; // I value
	values[3] = 50; // O value
	values[4] = 100; // z value
	values[5] = 125; // T value
	values[6] = 100; // z value

	sf::Color color[7] = {sf::Color(244,0,0), sf::Color(244,93,0), sf::Color(244,155,0), 
						  sf::Color(244,31,0), sf::Color(244,124,0), sf::Color(244,186,0),
						  sf::Color(244,62,0)};

	for (int i = 0; i < 7; i++) 
	{
		patterns.push_back(tPatterns[i]);
		int shapeW = static_cast<int>(patterns[i][0])-48;
		int shapeH = static_cast<int>(patterns[i][1])-48;
		std::vector<Cords> cord;

		for (int y = 0, ch = 3; y < shapeH; y++)
			for (int x = 0; x < shapeW; x++, ch++)
				if (static_cast<bool>(static_cast<int>(patterns[i][ch])-48))
					cord.push_back(Cords(x, y, color[i], false, true, values[i]));

		next.push_back(cord);
	}
	
	makeShape(rand() % 7);

	play = gameOver = false;
}

// this function will set a shapes paramaters it's matrix, the shape will be pushed onto the 'shape' vector
void Blocks::makeShape(int shape)
{
	int shapeW = static_cast<int>(patterns[shape][0])-48;
	int shapeH = static_cast<int>(patterns[shape][1])-48;

	sf::Color color[7] = {sf::Color(244,0,0), sf::Color(244,93,0), sf::Color(244,155,0), 
						  sf::Color(244,31,0), sf::Color(244,124,0), sf::Color(244,186,0),
						  sf::Color(244,62,0)};

	int spawnX = rand() % boardWidth;
	while (shapeW+spawnX > boardWidth) spawnX--;

	curr.clear();

	for (int y = 0, ch = 3; y < shapeH; y++)
	{
		for (int x = 0, sx = spawnX; x < shapeW; x++, sx++, ch++)
		{
			if (static_cast<bool>(static_cast<int>(patterns[shape][ch])-48))
			{
				// check for pivot
				if (static_cast<int>(patterns[shape][2])-48 != ch)
					curr.push_back(Cords(sx, y, color[shape], false, true, values[shape]));
				else
					curr.push_back(Cords(sx, y, color[shape], true, true, values[shape]));
			}
		}
	}

	nextId = rand() % 7;
}

bool Blocks::collision(direct dir)
{
	bool hit = false;

	for (std::size_t c = 0; c < curr.size(); c++)
	{
		if (dir == LEFT && curr[c].x-1 < 0) hit = true; // LEFT WALL
		if (dir == RIGHT && curr[c].x+1 >= boardWidth) hit = true; // RIGHT WALL
		if (dir == DOWN && curr[c].y+1 >= boardHeight) hit = true; // FLOOR

		if (dir == ROTATE) // ROTATING ON BLOCKS
		{
			if (curr[c].x < 0 || curr[c].x >= boardWidth || curr[c].y >= boardHeight || curr[c].y < 0)
				hit = true;
			else if (board[curr[c].y][curr[c].x].filled) hit = true;
		}

		if (dir == LEFT && curr[c].x-1 >= 0) // LEFT BLOCK
			if(board[curr[c].y][curr[c].x-1].filled) hit = true;
		if (dir == RIGHT && curr[c].x+1 < boardWidth) // RIGHT BLOCK
			if(board[curr[c].y][curr[c].x+1].filled) hit = true;
		if (dir == DOWN && curr[c].y+1 < boardHeight) // FALL ON BLOCK
			if(board[curr[c].y+1][curr[c].x].filled) hit = true;
	}

	if (hit) return true;
	else return false;
}

void Blocks::removeRows()
{
	int sum = 0, multiply = 0;
	// check for rows to delete if full
	for (int y = 0; y < boardHeight; y++)
	{
		bool clear = true;
		for (int x = 0; x < boardWidth; x++)
			if (!board[y][x].filled) clear = false;
		if (clear) 
		{
			totalLines++;
			multiply++;
			clearRow[y] = true;
			falling = true;
			animate.restart();
			for (int s = 0; s < shapes.size(); s++)
			{
				for (int c = 0; c < shapes[s].size(); c++)
				{
					if (shapes[s][c].y == y) 
					{
						sum += shapes[s][c].val;
						shapes[s].erase(shapes[s].begin() + c);
						c--; 
					}
					else if (shapes[s][c].y < y) shapes[s][c].y++;	
				}
				if (shapes[s].size() == 0)
				{
					shapes.erase(shapes.begin() + s);
					s--;
				}
			}
		}
	}

	// multiply the square values by how many lines were removed at the same time
	if (multiply) 
	{
		currTotal += sum;
		totalScore += (sum*multiply);
	}

	// increase speed after every 5 lines cleared
	if (currTotal >  1000)
	{
		levelId += (currTotal/1000);
		currTotal = 0;
		if (levelId < 30) speed = level[levelId];
		else levelId = 29;
	}
}
void Blocks::updateBoard()
{
	for (int y = 0; y < boardHeight; y++)
		for (int x = 0; x < boardWidth; x++) // clear entire board
			board[y][x].filled = false;

	for (int s = 0; s < shapes.size(); s++)
	{
		for (int c = 0; c < shapes[s].size(); c++)
		{
			board[shapes[s][c].y][shapes[s][c].x].filled = true;
			board[shapes[s][c].y][shapes[s][c].x].color = shapes[s][c].color;
		}
	}
}

void Blocks::run(sf::Vector2i pos)
{
	if (pos.x >= 301 && pos.x <= 412 && pos.y >= 221 && pos.y <= 263)
		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !gameOver)
			play = true;

	if (!falling && play)
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) left = true;
		else left = false;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) right = true;
		else right = false;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) speed = sf::seconds(0.03);
		else speed = level[levelId];
	
		// set to a bool value so the times are equal for left and right
		bool move = clockLR.getElapsedTime() > speedLR;
		if (left && !right && move) 
		{
			shapeMove(LEFT);
			left = false;
			clockLR.restart();
		}
		if (right && !left && move) 
		{
			shapeMove(RIGHT);
			right = false;
			clockLR.restart();
		}
		left = right = false;

		if (clock.getElapsedTime() > speed)
		{
			if (!shapeMove(DOWN)) // block hit
			{
				for (std::size_t c = 0; c < curr.size(); c++)
				{
					board[curr[c].y][curr[c].x].filled = true;
					board[curr[c].y][curr[c].x].color = curr[c].color;
					if (curr[c].y == 0) // if blocks hit the "top" of the board, gameover
					{
						gameOver = true;
						play = false;
					}
				}
				shapes.push_back(curr);
				removeRows();
				if (!falling) updateBoard();

				makeShape(nextId);
			}
			clock.restart();
		}

		if (!falling) freeFall();

		score.setString("score " + std::to_string(totalScore));
		worth.setString("square value " + std::to_string(curr[0].val));
		lines.setString("lines " + std::to_string(totalLines));
		levlTxt.setString("level " + std::to_string(levelId+1));
	}
	else
	{
		if (play)
		{
			alpha+=15;
			if (alpha > 185)
				alpha = 50;
			if (animate.getElapsedTime() > animaDelay)
			{
				falling = false;
				alpha = 50;
				for (int y = 0; y < clearRow.size(); y++)
					clearRow[y] = false;
				updateBoard();
				freeFall();
			}
		}
	}
}

void Blocks::freeFall()
{
	bool moving = true;
	while (moving)
	{
		moving = false;
		for (int s = 0; s < shapes.size(); s++)
		{
			// set shapes position to false on the board to prevent itself from colliding with itself
			for (int c = 0; c < shapes[s].size(); c++)
				board[shapes[s][c].y][shapes[s][c].x].filled = false; 
		
			bool hit = false;
			for (int c = 0; c < shapes[s].size(); c++)
			{
				if (shapes[s][c].y+1 >= boardHeight) hit = true; // FLOOR
			
				if (shapes[s][c].y+1 < boardHeight) // FALL ON BLOCK
					if(board[shapes[s][c].y+1][shapes[s][c].x].filled) hit = true;
			}
			if (!hit)
			{
				moving = true;
				for (int c = 0; c < shapes[s].size(); c++)
					shapes[s][c].y++;
			}
			for (int c = 0; c < shapes[s].size(); c++)
			{
				board[shapes[s][c].y][shapes[s][c].x].filled = true;
				board[shapes[s][c].y][shapes[s][c].x].color = shapes[s][c].color;
			}
		}
	}
	// only remove new rows if all pieces have fallen to their lowest position
	if (!moving) removeRows();
}

bool Blocks::shapeMove(direct dir)
{
	if (!collision(dir))
	{
		for (std::size_t c = 0; c < curr.size(); c++)
		{
			if (dir == DOWN) curr[c].y += 1;
			if (dir == LEFT) curr[c].x -= 1;
			if (dir == RIGHT) curr[c].x += 1;
		}
		return true;
	}
	return false;
}

void Blocks::rotate(int r)
{
	// rotation matrices: Left / Right
	int R[2][2][2] = {{{0,-1},{1,0}}, {{0,1},{-1,0}}};

	std::vector<Cords> reset = curr;

	int px = -1, py = -1; // pivot cords
	for (int i = 0; i < curr.size(); i++)
	{
		if (curr[i].pivot)
		{
			px = curr[i].x;
			py = curr[i].y;
		}
	}

	if (px > -1 && py > -1)
	{
		// center the pivot as the orgin
		for (int i = 0; i < curr.size(); i++)
		{
			curr[i].x -= px;
			curr[i].y -= py;
		}

		for (int i = 0; i < curr.size(); i++)
		{
			int sum[2][1];
			int C[2][1] = {{curr[i].x}, {curr[i].y}};
			for (int j = 0; j < 2; j++)
			{
				int total = 0;
				for (int k = 0; k < 2; k++)
					total += R[r][j][k]*C[k][0];
				sum[j][0] = total;
			}
			curr[i].x = sum[0][0] + px;
			curr[i].y = sum[1][0] + py;
		}

		// rotate failure 
		if (collision(ROTATE)) curr = reset;
	}
}

void Blocks::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	sf::RectangleShape square;
	square.setSize(sf::Vector2f(squareWidth-1, squareHeight-1));
	square.setOutlineThickness(1);
	square.setOutlineColor(sf::Color::Black);
	
	for (int y = 0; y < boardHeight; y++)
	{
		for (int x = 0; x < boardWidth; x++)
		{
			if (board[y][x].filled)
			{
				square.setFillColor(board[y][x].color);
				square.setPosition(x*squareWidth+1+25, y*squareHeight+1+25);
				target.draw(square);
			}
		}
	}
	for (std::size_t c = 0; c < curr.size(); c++)
	{
		square.setFillColor(curr[c].color);
		square.setPosition(curr[c].x*squareWidth+1+25, curr[c].y*squareHeight+1+25);
		target.draw(square);
	}

	target.draw(menu);
	target.draw(worth);
	target.draw(score);
	target.draw(lines);
	target.draw(levlTxt);

	for (std::size_t n = 0; n < next[nextId].size(); n++)
	{
		square.setFillColor(next[nextId][n].color);
		square.setOutlineColor(sf::Color(15,15,15));
		square.setPosition(next[nextId][n].x*squareWidth+363, next[nextId][n].y*squareHeight+447);
		target.draw(square);
	}

	for (int y = 0; y < boardHeight+2; y++)
	{
		for (int x = 0; x < boardWidth+2; x++)
		{
			if (x == 0 || x == boardWidth+2-1 || y == 0 || y == boardHeight+2-1)
			{
				square.setOutlineColor(sf::Color(75,75,75));
				square.setFillColor(sf::Color(105,105,105));
				square.setPosition(x*squareWidth+1, y*squareHeight+1);
				target.draw(square);
			}
		}
	}

	if (animate.getElapsedTime() < animaDelay && falling)
	{
		for (int y = 0; y < clearRow.size(); y++)
		{
			if (clearRow[y])
			{
				square.setOutlineThickness(0);
				square.setFillColor(sf::Color(255,251,166,alpha));
				square.setSize(sf::Vector2f(249, 24));
				square.setPosition(26, y*25+26);
				target.draw(square);
			}
		}
	}
}

int main()
{
	srand(static_cast<unsigned int>(time(0)));

	sf::RenderWindow app(sf::VideoMode(WIDTH+MENUWIDTH,HEIGHT+1+50), "Blocks", sf::Style::Close);
	app.setVerticalSyncEnabled(true);

	Blocks * blocks = new Blocks;

	while (app.isOpen())
	{
		sf::Event event;
		
		sf::Vector2i mPos = sf::Mouse::getPosition(app);
		while (app.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				app.close();
			if (event.type == sf::Event::TextEntered)
			{
				if (event.text.unicode < 128 && static_cast<char>(event.text.unicode) == 'q')
					blocks->rotate(0);
				else if (event.text.unicode < 128 && static_cast<char>(event.text.unicode) == 'e')
					blocks->rotate(1);
			}
			if (event.type == sf::Event::MouseButtonReleased)
			{
				if (mPos.x >= 414 && mPos.x <= 525 && mPos.y >= 221 && mPos.y <= 263)
				{
					// reset button pressed
					delete blocks;
					blocks = new Blocks;
				}
				
			}
		}

		blocks->run(mPos);

		app.clear(sf::Color(0,0,0));
		app.draw(*blocks);
		app.display();
	}

	delete blocks;
	return 0;
}