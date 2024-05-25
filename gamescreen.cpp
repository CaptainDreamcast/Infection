#include "gamescreen.h"

#include <random>
#include <stack>

#include <prism/screeneffect.h>

#include "titlescreen.h"

static struct
{
    int mLevel = 0;
} gGameScreenData;


class GameScreen
{
public:
    MugenSpriteFile mSprites;
    MugenAnimations mAnimations;
    MugenSounds mSounds;

    GameScreen() {
        loadFiles();
        createMaze();
        createMazeAnimations();
        loadAndPlacePlayer();
        loadAndPlaceEnemies();
        loadUI();

        tryPlayMugenSound(&mSounds, 2, 0);
    }

    int roundStartTextId;
    int infectedLeftTextId;
    int timerTextId;
    int roundStartTextTime = 0;
    int infectedLeftTextTime = 0;
    int roundTimeLeft = 0;
    
    void loadUI()
    {
        std::string roundText = "ROUND " + std::to_string(gGameScreenData.mLevel + 1) + " START";
        roundStartTextId = addMugenText(roundText.c_str(), Vector3D(40, 120, 20), 1);
        setMugenTextScale(roundStartTextId, 4);
        roundStartTextTime = 120;
        infectedLeftTextId = addMugenText("UNINFECTED LEFT: 0", Vector3D(40, 220, 20), 1);
        setMugenTextScale(infectedLeftTextId, 3);
        setMugenTextVisibility(infectedLeftTextId, 0);

        timerTextId = addMugenText("TIME LEFT: 0", Vector3D(40, 240, 20), 1);
        setMugenTextScale(timerTextId, 3);
        roundTimeLeft = enemiesAndLevelToTime();
        updateTimerText(true);
    }

    int enemiesAndLevelToTime()
    {
        return 60 * (mEnemies.size() + 5);
    }

    void updateTimerText(bool forced = false)
    {
        if(isFrozen() && !forced) return;

        if(!forced)
        {
            roundTimeLeft--;
        }

        if(roundTimeLeft == 0)
        {
            changeMugenText(timerTextId, "TIME LEFT: 0");
            startGameOverText();
        }
        else
        {
            int seconds = roundTimeLeft / 60;
            int milliseconds = roundTimeLeft % 60;
            char text[100];
            sprintf(text, "TIME LEFT: %02d:%02d", seconds, milliseconds);
            changeMugenText(timerTextId, text);
        }
    }

    void updateUI()
    {
        updateRoundStartText();
        updateInfectedLeftText();
        updateWinText();
        updateGameOverText();
        updateTimerText();
    }

    void updateRoundStartText()
    {
        if(roundStartTextTime == 0) return;

        roundStartTextTime--;
        if(roundStartTextTime == 0)
        {
            setMugenTextVisibility(roundStartTextId, 0);
            streamMusicFile("game/BGM.ogg");
        }
    }

    void updateInfectedLeftText()
    {
        if(infectedLeftTextTime == 0) return;

        infectedLeftTextTime--;
        if(infectedLeftTextTime == 0)
        {
            setMugenTextVisibility(infectedLeftTextId, 0);
        }
    }

    int winTimer = 0;
    int loseTimer = 0;

    void startGameOverText()
    {
        changeMugenText(roundStartTextId, "GAME OVER, SORRY");
        stopStreamingMusicFile();
        setMugenTextVisibility(roundStartTextId, 1);
        loseTimer = 120;
    }

    void updateGameOverText()
    {
        if(loseTimer == 0) return;

        loseTimer--;
        if(loseTimer == 0)
        {
            setNewScreen(getTitleScreen());
        }
    }

    void startInfectedLeftText()
    {
        int uninfectedLeft = int(mEnemies.size());
        for(const auto& e : mEnemies)
        {
            if(e.isInfected) uninfectedLeft--;
        }

        if(uninfectedLeft)
        {
            std::string text = "UNINFECTED LEFT: " + std::to_string(uninfectedLeft);
            changeMugenText(infectedLeftTextId, text.c_str());
            setMugenTextVisibility(infectedLeftTextId, 1);
            infectedLeftTextTime = 120;
        }
        else
        {
            changeMugenText(roundStartTextId, "SUPREME VICTORY");
            stopStreamingMusicFile();
            setMugenTextVisibility(roundStartTextId, 1);
            setMugenTextVisibility(infectedLeftTextId, 0);
            winTimer = 120;
        }
    }

    int isFrozen()
    {
        return winTimer || roundStartTextTime || loseTimer;
    }

    void updateWinText()
    {
        if(winTimer == 0) return;

        winTimer--;
        if(winTimer == 0)
        {
            gGameScreenData.mLevel++;
            setNewScreen(getGameScreen());
        }
    }

    struct Enemy
    {
        int entityID;
        Vector2DI tilePosition = Vector2DI(0, 0);
        Vector2DI movementDirection = Vector2DI(0, 0);
        Vector2DI movementDelta = Vector2DI(0, 0);
        int isMoving = false;
        bool isInfected = false;
    };
    std::vector<Enemy> mEnemies;
    
    int enemyCountFromFreeTileCount()
    {
        int freeTileCount = 0;
        for (int y = 0; y < mazeSizeY; y++)
        {
            for (int x = 0; x < mazeSizeX; x++)
            {
                if (maze[y][x] == 0)
                {
                    freeTileCount++;
                }
            }
        }

        return freeTileCount / 10;
    }

    void loadAndPlaceEnemies()
    {
        int enemyCount = enemyCountFromFreeTileCount();
        for(size_t i = 0; i < enemyCount; i++)
        {
            Enemy e;
            
            while (true)
            {
                e.tilePosition.x = rand() % mazeSizeX;
                e.tilePosition.y = rand() % mazeSizeY;

                bool isEnemyOnTile = false;
                for (const auto& enemy : mEnemies)
                {
                    if (enemy.tilePosition == e.tilePosition)
                    {
                        isEnemyOnTile = true;
                        break;
                    }
                }
                if(maze[e.tilePosition.y][e.tilePosition.x] == 0 && !isEnemyOnTile) break;
            }

            e.entityID = addBlitzEntity((tileToScreen(e.tilePosition) + playerPositionOffset).xyz(2));
            addBlitzMugenAnimationComponent(e.entityID, &mSprites, &mAnimations, 11);
            mEnemies.push_back(e);
        }
    }

    int mazeSizeX = 20;
    int mazeSizeY = 15;
    std::vector<std::vector<int>> maze;

    void createMaze()
    {
        // Initialize the maze with walls
        maze.resize(mazeSizeY, std::vector<int>(mazeSizeX, 1));

        // Start generating the maze from a random position
        std::stack<std::pair<int, int>> stack;
        int startX = rand() % mazeSizeX;
        int startY = rand() % mazeSizeY;
        stack.push({startX, startY});
        maze[startY][startX] = 0;

        std::random_device rd;
        std::mt19937 g(rd());
        while (!stack.empty())
        {
            int x = stack.top().first;
            int y = stack.top().second;
            stack.pop();

            // Shuffle the directions
            std::vector<std::pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            std::shuffle(directions.begin(), directions.end(), g);

            for (const auto& dir : directions)
            {
                int nx = x + dir.first * 2;
                int ny = y + dir.second * 2;

                if (nx >= 0 && nx < mazeSizeX && ny >= 0 && ny < mazeSizeY && maze[ny][nx] == 1)
                {
                    // Carve a path
                    maze[y + dir.second][x + dir.first] = 0;
                    maze[ny][nx] = 0;

                    stack.push({nx, ny});
                    break;
                }
            }
        }

        // Ensure all paths are connected
        for (int y = 1; y < mazeSizeY - 1; y += 2)
        {
            for (int x = 1; x < mazeSizeX - 1; x += 2)
            {
                if (maze[y][x] == 1)
                {
                    // Find a neighboring path
                    std::vector<std::pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
                    std::shuffle(directions.begin(), directions.end(), g);

                    for (const auto& dir : directions)
                    {
                        int nx = x + dir.first;
                        int ny = y + dir.second;

                        if (maze[ny][nx] == 0)
                        {
                            // Carve a path to the neighboring path
                            maze[y][x] = 0;
                            break;
                        }
                    }
                }
            }
        }

        // Ensure at least 70% of the maze is path
        int pathCount = 0;
        for (const auto& row : maze)
        {
            for (int cell : row)
            {
                if (cell == 0)
                    pathCount++;
            }
        }

        while (pathCount < mazeSizeX * mazeSizeY * 0.7)
        {
            int x = rand() % mazeSizeX;
            int y = rand() % mazeSizeY;

            if (maze[y][x] == 1)
            {
                //maze[y][x] = 0;
                pathCount++;
            }
        }
    }

    void createMazeAnimations()
    {
        for (int y = 0; y < mazeSizeY; y++)
        {
            for (int x = 0; x < mazeSizeX; x++)
            {
                if (maze[y][x] == 1)
                {
                    auto id = addMugenAnimation(getMugenAnimation(&mAnimations, 1), &mSprites, Vector3D(x * 16, y * 16, 1));
                }
            }
        }
    }

    Vector2D tileToScreen(const Vector2DI& tilePosition)
    {
        return Vector2D(tilePosition.x * 16, tilePosition.y * 16);
    }

    int playerBlitzEntity;
    Vector2DI playerPositionOffset = Vector2DI(7, 14);
    Vector2DI playerTilePosition = Vector2DI(0, 0);
    void loadAndPlacePlayer()
    {
        playerTilePosition.x = 0;
        playerTilePosition.y = 0;
        while (maze[playerTilePosition.y][playerTilePosition.x] == 1)
        {
            playerTilePosition.x++;
            if (playerTilePosition.x == mazeSizeX)
            {
                playerTilePosition.x = 0;
                playerTilePosition.y++;
            }
        }

        playerBlitzEntity = addBlitzEntity((tileToScreen(playerTilePosition) + playerPositionOffset).xyz(2));
        addBlitzMugenAnimationComponent(playerBlitzEntity, &mSprites, &mAnimations, 10);
    }

    void loadFiles() {
        mSprites = loadMugenSpriteFileWithoutPalette("game/GAME.sff");
        mAnimations = loadMugenAnimationFile("game/GAME.air");
        mSounds = loadMugenSoundFile("game/GAME.snd");
    }

    void update() {
        updatePlayerMovement();
        updateEnemiesMovement();
        updateInfectionEnemies();
        updateUI();
    }

    void updateEnemiesMovement()
    {
        if(isFrozen()) return;

        for(auto& e : mEnemies)
        {
            updateEnemyMovement(e);
        }
    }

    void updateEnemyMovement(Enemy& e)
    {
        updateEnemyMovementStart(e);
        updateEnemyMovementOngoing(e);
    }

    int directionToIndex(const Vector2DI& direction)
    {
        if(direction == Vector2DI(-1, 0)) return 0;
        if(direction == Vector2DI(1, 0)) return 1;
        if(direction == Vector2DI(0, -1)) return 2;
        if(direction == Vector2DI(0, 1)) return 3;
        return 0;
    }

    void updateEnemyMovementStart(Enemy& e)
    {
        if(e.isMoving) return;

        int direction = directionToIndex(e.movementDirection);
        for(int i = 0; i < 100; i++)
        {
            if(direction == 0 && e.tilePosition.x > 0 && !maze[e.tilePosition.y][e.tilePosition.x - 1])
            {
                e.movementDirection = Vector2DI(-1, 0);
                e.isMoving = 1;
                e.movementDelta = Vector2DI(0, 0);
                break;
            }
            
            if(direction == 1 && e.tilePosition.x < mazeSizeX - 1 && !maze[e.tilePosition.y][e.tilePosition.x + 1])
            {
                e.movementDirection = Vector2DI(1, 0);
                e.isMoving = 1;
                e.movementDelta = Vector2DI(0, 0);
                break;
            }
            
            if(direction == 2 && e.tilePosition.y > 0 && !maze[e.tilePosition.y - 1][e.tilePosition.x])
            {
                e.movementDirection = Vector2DI(0, -1);
                e.isMoving = 1;
                e.movementDelta = Vector2DI(0, 0);
                break;
            }
            
            if(direction == 3 && e.tilePosition.y < mazeSizeY - 1 && !maze[e.tilePosition.y + 1][e.tilePosition.x])
            {
                e.movementDirection = Vector2DI(0, 1);
                e.isMoving = 1;
                e.movementDelta = Vector2DI(0, 0);
                break;
            }

            direction = rand() % 4;
        }
    }

    int isTileInfected(const Vector2DI& tilePosition)
    {
        if (playerTilePosition == tilePosition) return 1;
        if(isPlayerInMovement && playerTilePosition + playerDirection == tilePosition) return 1;

        for(const auto& e : mEnemies)
        {
            if(!e.isInfected) continue;
            if(e.tilePosition == tilePosition) return 1;
            if(e.isMoving && e.tilePosition + e.movementDirection == tilePosition) return 1;
        }

        return maze[tilePosition.y][tilePosition.x] == 1;
    }

    void updateInfectionEnemies()
    {
        for(auto& e : mEnemies)
        {
            updateInfectionSingleEnemy(e);
        }
    }

    void updateInfectionSingleEnemy(Enemy& e)
    {
        if(e.isInfected) return;

        bool isInfected = isTileInfected(e.tilePosition);
        if(e.isMoving) {
            isInfected = isInfected || isTileInfected(e.tilePosition + e.movementDirection);
        }
        
        if(isInfected) {
            e.isInfected = true;
            tryPlayMugenSound(&mSounds, 3, 0);
            startInfectedLeftText();
            changeBlitzMugenAnimation(e.entityID, 12);
        }
    }

    void updateEnemyMovementOngoing(Enemy& e)
    {
        if(!e.isMoving) return;

        e.movementDelta = e.movementDelta + e.movementDirection + e.movementDirection;

        setBlitzEntityPosition(e.entityID, (tileToScreen(e.tilePosition) + playerPositionOffset + e.movementDelta).xyz(2));

        if (std::abs(e.movementDelta.x) == 16 || std::abs(e.movementDelta.y) == 16)
        {
            e.isMoving = 0;
            e.tilePosition = e.tilePosition + e.movementDirection;
        }
    }

    int isPlayerInMovement = 0;
    Vector2DI playerDirection = Vector2DI(0, 0);
    Vector2DI playerMovementDelta = Vector2DI(0, 0);

    void updatePlayerMovement()
    {
        if(isFrozen()) return;
        updatePlayerMovementStart();
        updatePlayerMovementOngoing();
    }

    void updatePlayerMovementStart()
    {
        if(isPlayerInMovement) return;

        if(hasPressedLeft() && playerTilePosition.x > 0 && !maze[playerTilePosition.y][playerTilePosition.x - 1])
        {
            playerDirection = Vector2DI(-1, 0);
            isPlayerInMovement = 1;
            playerMovementDelta = Vector2DI(0, 0);
        }
        else if(hasPressedRight() && playerTilePosition.x < mazeSizeX - 1 && !maze[playerTilePosition.y][playerTilePosition.x + 1])
        {
            playerDirection = Vector2DI(1, 0);
            isPlayerInMovement = 1;
            playerMovementDelta = Vector2DI(0, 0);
        }
        else if(hasPressedUp() && playerTilePosition.y > 0 && !maze[playerTilePosition.y - 1][playerTilePosition.x])
        {
            playerDirection = Vector2DI(0, -1);
            isPlayerInMovement = 1;
            playerMovementDelta = Vector2DI(0, 0);
        }
        else if(hasPressedDown() && playerTilePosition.y < mazeSizeY - 1 && !maze[playerTilePosition.y + 1][playerTilePosition.x])
        {
            playerDirection = Vector2DI(0, 1);
            isPlayerInMovement = 1;
            playerMovementDelta = Vector2DI(0, 0);
        }
    }

    void updatePlayerMovementOngoing()
    {
        if(!isPlayerInMovement) return;

        playerMovementDelta = playerMovementDelta + playerDirection + playerDirection;

        setBlitzEntityPosition(playerBlitzEntity, (tileToScreen(playerTilePosition) + playerPositionOffset + playerMovementDelta).xyz(2));

        if (std::abs(playerMovementDelta.x) == 16 || std::abs(playerMovementDelta.y) == 16)
        {
            isPlayerInMovement = 0;
            playerTilePosition = playerTilePosition + playerDirection;
        }
    }
};

EXPORT_SCREEN_CLASS(GameScreen);

void resetGame()
{
    gGameScreenData.mLevel = 0;
}