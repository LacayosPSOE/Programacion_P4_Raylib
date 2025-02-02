/*******************************************************************************************
 *
 *   raylib maze generator
 *
 *   Procedural maze generator using Maze Grid Algorithm
 *
 *   This game has been created using raylib (www.raylib.com)
 *   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
 *
 *   Copyright (c) 2024 Ramon Santamaria (@raysan5)
 *
 ********************************************************************************************/

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h" // Required for immediate-mode UI elements

#include <stdlib.h> // Required for: malloc(), free()

#define MAZE_WIDTH 64
#define MAZE_HEIGHT 64
#define MAZE_DRAW_SCALE 10.0f

#define MAX_MAZE_ITEMS 16

// Declare new data type: Point
typedef struct Point
{
    int x;
    int y;
} Point;

typedef struct Timer
{
    double currentTime;
    double lifeTime;
} Timer;

// Generate procedural maze image, using grid-based algorithm
// NOTE: Functions defined as static are internal to the module
static Image GenImageMazeEx(int width, int height, int spacingRows, int spacingCols, float skipChance);

// Get shorter path between two points, implements pathfinding algorithm: A*
static Point *LoadPathAStar(Image map, Point start, Point end, int *pointCount);

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "raylib maze generator");

    // Current application mode
    int currentMode = 2; // 0-Game2D, 1-Game3D, 2-Editor

    // Random seed defines the random numbers generation,
    // always the same if using the same seed
    SetRandomSeed(67216);

    // Generate maze image using the grid-based generator
    // DONE: [1p] Improve function to support extra configuration parameters
    Image imMaze = GenImageMazeEx(MAZE_WIDTH, MAZE_HEIGHT, 3, 3, 0.75f);

    // Load a texture to be drawn on screen from our image data
    // WARNING: If imMaze pixel data is modified, texMaze needs to be re-loaded
    Texture texMaze = LoadTextureFromImage(imMaze);

    // Generate 3D mesh from image and load a 3D model from mesh
    Mesh meshMaze = GenMeshCubicmap(imMaze, (Vector3){1.0f, 1.0f, 1.0f});
    Model mdlMaze = LoadModelFromMesh(meshMaze);
    Vector2 mazePosition = {GetScreenWidth() / 2 - texMaze.width * MAZE_DRAW_SCALE / 2, GetScreenHeight() / 2 - texMaze.height * MAZE_DRAW_SCALE / 2};
    Vector3 mdlPosition = {0.0f, 0.0f, 0.0f}; // Set model position

    // Start and end cell positions (user defined)
    Point startCell = {1, 1};
    Point endCell = {imMaze.width - 2, imMaze.height - 2};

    // Player current position on image-coordinates
    // WARNING: It could require conversion to world coordinates!
    Point playerCell = startCell;

    // Camera 2D for 2d gameplay mode
    // DONE: Initialize camera parameters as required
    Camera2D camera2d = {0};
    camera2d.target = (Vector2){mazePosition.x + playerCell.x * MAZE_DRAW_SCALE, mazePosition.y + playerCell.y * MAZE_DRAW_SCALE};
    camera2d.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    camera2d.rotation = 0.0f;
    camera2d.zoom = 5.0f;

    // Camera 3D for first-person gameplay mode
    // DONE: Initialize camera parameters as required
    // NOTE: In a first-person mode, camera.position is actually the player position
    // REMEMBER: We are using a different coordinates space than 2d mode
    Camera3D cameraFP = {0};
    cameraFP.position = (Vector3){1.5f, 0.5f, 1.5f};
    cameraFP.target = (Vector3){1.5f, 0.5f, 2.0f};
    cameraFP.up = (Vector3){0.0f, 1.0f, 0.0f};
    cameraFP.fovy = 45.0f;
    cameraFP.projection = CAMERA_PERSPECTIVE;

    // Mouse selected cell for maze editing
    Point selectedCell = {0};

    // Maze items position and state
    Point mazeItems[MAX_MAZE_ITEMS] = {0};
    bool mazeItemPicked[MAX_MAZE_ITEMS] = {0};

    // Define textures to be used as our "biomes"
    // DONE: Load additional textures for different biomes
    Texture texBiomes[4] = {0};
    texBiomes[0] = LoadTexture("resources/maze_atlas01.png");
    texBiomes[1] = LoadTexture("resources/maze_atlas02.png");
    texBiomes[2] = LoadTexture("resources/maze_atlas03.png");
    texBiomes[3] = LoadTexture("resources/maze_atlas04.png");
    int currentBiome = 0;

    mdlMaze.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texBiomes[0];

    // DONE: Define all variables required for game UI elements (sprites, fonts...)

    // DONE: Define all variables required for UI editor (raygui)
    Rectangle rowSpaceRec = (Rectangle){GetScreenWidth() - 150, 40, 120, 20};
    int spacingRows = 3;
    Rectangle colSpaceRec = (Rectangle){GetScreenWidth() - 150, 60, 120, 20};
    int spacingCols = 3;
    Rectangle seedRec = (Rectangle){GetScreenWidth() - 150, 80, 120, 20};
    int seed = 67218;
    Rectangle skipRec = (Rectangle){GetScreenWidth() - 150, 100, 120, 20};
    int skipChance = 75;
    Rectangle buttonRec = (Rectangle){GetScreenWidth() - 150, 120, 120, 20};
    bool editRowSpace = false;
    bool editColSpace = false;
    bool editSeed = false;
    bool editSkipChance = false;
    bool updateMap = false;

    // Check if A* calc is needed
    bool isAStarCalculated = false;
    int aStarPointCount = 0;
    Point *pathAStar = NULL;

    SetTargetFPS(60);      // Set our game to run at 60 frames-per-second
    bool exitGame = false; // Game exit handler
    double gameScore = 0;  // Game Score

    // DONE: Set game timer
    Timer timer;
    timer.currentTime = 0;
    timer.lifeTime = 120;
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Check if game exits
        exitGame = exitGame || timer.currentTime >= timer.lifeTime;
        if (exitGame)
            break;

        // Select current mode as desired
        if (IsKeyPressed(KEY_Z))
            currentMode = 0; // Game 2D mode
        else if (IsKeyPressed(KEY_X))
            currentMode = 1; // Game 3D mode
        else if (IsKeyPressed(KEY_C))
            currentMode = 2; // Editor mode

        switch (currentMode)
        {
        case 0: // Game 2D mode
        {
            // DONE: [2p] Player 2D movement from predefined start point (A) to end point (B)
            // Implement maze 2D player movement logic (cursors || WASD)
            // Use imMaze pixel information to check collisions
            // Detect if current playerCell == endCell to finish game
            Point playerCellPre = playerCell;

            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
                playerCell.y -= 1;
            if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
                playerCell.y += 1;
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
                playerCell.x -= 1;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
                playerCell.x += 1;

            if (GetImageColor(imMaze, playerCell.x, playerCell.y).r == 255)
                playerCell = playerCellPre;

            if ((playerCell.x == endCell.x) && (playerCell.y == endCell.y))
                exitGame = true;

            // DONE: [2p] Camera 2D system following player movement around the map
            // Update Camera2D parameters as required to follow player and zoom control
            camera2d.target = (Vector2){mazePosition.x + playerCell.x * MAZE_DRAW_SCALE, mazePosition.y + playerCell.y * MAZE_DRAW_SCALE};

            camera2d.zoom += ((float)GetMouseWheelMove() * 0.5f);
            if (camera2d.zoom > 6.0f)
                camera2d.zoom = 6.0f;
            else if (camera2d.zoom < 1.0f)
                camera2d.zoom = 1.0f;

            // Sync 3D camera position
            cameraFP.position.x = playerCell.x + mdlPosition.x - 0.5f;
            cameraFP.position.z = playerCell.y + mdlPosition.y - 0.5f;

            // DONE: Maze items pickup logic
            for (int i = 0; i < MAX_MAZE_ITEMS; i++)
            {
                if ((playerCell.x == mazeItems[i].x) && (playerCell.y == mazeItems[i].y) && !mazeItemPicked[i])
                {
                    gameScore++;
                    mazeItemPicked[i] = true;
                }
            }

            // Increase Timer
            timer.currentTime += GetFrameTime();
        }
        break;
        case 1: // Game 3D mode
        {
            // DONE: [1p] Camera 3D system and �3D maze mode�
            // Implement maze 3d first-person mode -> TIP: UpdateCamera()
            // Use the imMaze map to implement collision detection, similar to 2D
            Vector3 camOldPos = cameraFP.position;
            UpdateCamera(&cameraFP, CAMERA_FIRST_PERSON);

            // Position update
            Vector2 playerPos = {cameraFP.position.x, cameraFP.position.z};
            playerCell.x = (int)(playerPos.x - mdlPosition.x + 0.5f);
            playerCell.y = (int)(playerPos.y - mdlPosition.z + 0.5f);

            // Wall collision handler
            if (GetImageColor(imMaze, playerCell.x, playerCell.y).r == 255)
                cameraFP.position = camOldPos;

            // DONE: Maze items pickup logic
            for (int i = 0; i < MAX_MAZE_ITEMS; i++)
            {
                if ((playerCell.x == mazeItems[i].x) && (playerCell.y == mazeItems[i].y) && !mazeItemPicked[i])
                {
                    mazeItemPicked[i] = true;
                    gameScore++;
                }
            }

            // Increase Timer
            timer.currentTime += GetFrameTime();
        }
        break;
        case 2: // Editor mode
        {
            // DONE: [2p] Visual �map editor mode�. Edit image pixels with mouse.
            // Implement logic to selecte image cell from mouse position -> TIP: GetMousePosition()
            // NOTE: Mouse position is returned in screen coordinates and it has to
            // transformed into image coordinates
            // Once the cell is selected, if mouse button pressed add/remove image pixels
            // WARNING: Remember that when imMaze changes, texMaze and mdlMaze must be also updated!
            Vector2 mousePos = GetMousePosition();
            selectedCell.x = (mousePos.x - (GetScreenWidth() / 2 - texMaze.width * MAZE_DRAW_SCALE / 2)) / MAZE_DRAW_SCALE;
            selectedCell.y = (mousePos.y - (GetScreenHeight() / 2 - texMaze.height * MAZE_DRAW_SCALE / 2)) / MAZE_DRAW_SCALE;

            if (selectedCell.x >= 0 && selectedCell.x < MAZE_WIDTH && selectedCell.y >= 0 && selectedCell.y < MAZE_HEIGHT)
            {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    ImageDrawPixel(&imMaze, selectedCell.x, selectedCell.y, WHITE);
                    updateMap = true;
                }
                else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
                {
                    ImageDrawPixel(&imMaze, selectedCell.x, selectedCell.y, BLACK);
                    // Check if an item is deleted
                    for (int i = 0; i < MAX_MAZE_ITEMS; i++)
                    {
                        if (selectedCell.x == mazeItems[i].x && selectedCell.y == mazeItems[i].y)
                            mazeItems[i] = (Point){NULL};
                    }
                    updateMap = true;
                }
            }

            // DONE: [2p] Collectible map items: player score
            // Using same mechanism than map editor, implement an items editor, registering
            // points in the map where items should be added for player pickup -> TIP: mazeItems[]
            if (selectedCell.x >= 0 && selectedCell.x < MAZE_WIDTH && selectedCell.y >= 0 && selectedCell.y < MAZE_HEIGHT)
            {
                if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON))
                {
                    int pos = 0;
                    bool found = false;
                    while ((pos < MAX_MAZE_ITEMS) && !found)
                    {
                        if (mazeItems[pos].x == NULL)
                            found = true;
                        else
                            pos++;
                    }
                    if (found)
                    {
                        ImageDrawPixel(&imMaze, selectedCell.x, selectedCell.y, BLUE);
                        mazeItems[pos] = (Point){selectedCell.x, selectedCell.y};
                        mazeItemPicked[pos] = false;
                        updateMap = true;
                    }
                }
            }

            // Reload texture and model
            if (updateMap)
            {
                UnloadTexture(texMaze);
                texMaze = LoadTextureFromImage(imMaze);
                UnloadMesh(meshMaze);
                meshMaze = GenMeshCubicmap(imMaze, (Vector3){1.0f, 1.0f, 1.0f});
                isAStarCalculated = false;
                updateMap = false;
            }
        }
        break;
        default:
            break;
        }

        // DONE: [1p] Multiple maze biomes supported
        // Implement changing between the different textures to be used as biomes
        // NOTE: For the 3d model, the current selected texture must be applied to the model material
        if (IsKeyPressed(KEY_ONE))
            currentBiome = 0;
        else if (IsKeyPressed(KEY_TWO))
            currentBiome = 1;
        else if (IsKeyPressed(KEY_THREE))
            currentBiome = 2;
        else if (IsKeyPressed(KEY_FOUR))
            currentBiome = 3;

        // DONE: EXTRA: Calculate shorter path between startCell (or playerCell) to endCell (A* algorithm)
        // NOTE: Calculation can be costly, only do it if startCell/playerCell or endCell change
        if (!isAStarCalculated)
        {
            pathAStar = LoadPathAStar(imMaze, startCell, endCell, &aStarPointCount);
            isAStarCalculated = true;
        }

        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        switch (currentMode)
        {
        case 0: // Game 2D mode
        {
            // Draw maze using camera2d (for automatic positioning and scale)
            BeginMode2D(camera2d);

            // DONE: Draw maze walls and floor using current texture biome
            for (int y = 0; y < imMaze.height; y++)
            {
                for (int x = 0; x < imMaze.width; x++)
                {
                    if (GetImageColor(imMaze, x, y).r == 255)
                    {
                        DrawTexturePro(texBiomes[currentBiome], (Rectangle){0, texBiomes[currentBiome].height / 2, texBiomes[currentBiome].width / 2, texBiomes[currentBiome].height / 2}, (Rectangle){mazePosition.x + x * MAZE_DRAW_SCALE, mazePosition.y + y * MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE}, (Vector2){0, 0}, 0.0f, WHITE);
                    }
                    else
                    {
                        DrawTexturePro(texBiomes[currentBiome], (Rectangle){0, 0, texBiomes[currentBiome].width / 2, texBiomes[currentBiome].height / 2}, (Rectangle){mazePosition.x + x * MAZE_DRAW_SCALE, mazePosition.y + y * MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE}, (Vector2){0, 0}, 0.0f, WHITE);
                    }
                }
            }

            // End cell drawn in red in order to see finish position
            DrawRectangle(mazePosition.x + endCell.x * MAZE_DRAW_SCALE, mazePosition.y + endCell.y * MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, RED);

            // DONE: Draw maze items 2d (using sprite texture?)
            for (int i = 0; i < MAX_MAZE_ITEMS; i++)
            {
                if (mazeItems[i].x != NULL)
                    DrawRectangle(mazePosition.x + mazeItems[i].x * MAZE_DRAW_SCALE, mazePosition.y + mazeItems[i].y * MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, BLUE);
            }

            // DONE: EXTRA: Draw pathfinding result, shorter path from start to end
            for (int i = 1; i < aStarPointCount; i++)
            {
                DrawRectangle(mazePosition.x + pathAStar[i].x * MAZE_DRAW_SCALE, mazePosition.y + pathAStar[i].y * MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, PURPLE);
            }

            // DONE: Draw player rectangle or sprite at player position
            DrawRectangle(mazePosition.x + playerCell.x * MAZE_DRAW_SCALE, mazePosition.y + playerCell.y * MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, GREEN);

            EndMode2D();

            // DONE: Draw game UI (score, time...) using custom sprites/fonts
            // NOTE: Game UI does not receive the camera2d transformations,
            // it is drawn in screen space coordinates directly
            // Draw Time Left
            char *currentTime = TextFormat("Time Left: %.0f", timer.lifeTime - timer.currentTime);
            DrawText(currentTime, 10, 30, 20, BLACK);

            // Draw Current Points
            char *currentScore = TextFormat("Score: %.0f", gameScore);
            DrawText(currentScore, 10, 50, 20, BLACK);
        }
        break;
        case 1: // Game 3D mode
        {
            // Draw maze using cameraFP (for first-person camera)
            BeginMode3D(cameraFP);

            // DONE: Draw maze generated 3d model
            mdlMaze.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texBiomes[currentBiome];
            DrawModel(mdlMaze, mdlPosition, 1.0f, WHITE);

            // DONE: Maze items 3d draw (using 3d shape/model?) on required positions
            for (int i = 0; i < MAX_MAZE_ITEMS; i++)
            {
                if (mazeItems[i].x != NULL)
                {
                    DrawCube((Vector3){mdlPosition.x + mazeItems[i].x, mdlPosition.y + 0.5f, mdlPosition.z + mazeItems[i].y}, 0.5f, 0.5f, 0.5f, BLUE);
                }
            }

            EndMode3D();

            // DONE: Draw game UI (score, time...) using custom sprites/fonts
            // NOTE: Game UI does not receive the camera2d transformations,
            // it is drawn in screen space coordinates directly
            // Draw Time Left
            char *currentTime = TextFormat("Time Left: %.0f", timer.lifeTime - timer.currentTime);
            DrawText(currentTime, 10, 30, 20, BLACK);

            // Draw Current Points
            char *currentScore = TextFormat("Score: %.0f", gameScore);
            DrawText(currentScore, 10, 50, 20, BLACK);
        }
        break;
        case 2: // Editor mode
        {
            // Draw generated maze texture, scaled and centered on screen
            DrawTextureEx(texMaze, (Vector2){GetScreenWidth() / 2 - texMaze.width * MAZE_DRAW_SCALE / 2, GetScreenHeight() / 2 - texMaze.height * MAZE_DRAW_SCALE / 2}, 0.0f, MAZE_DRAW_SCALE, WHITE);

            // Draw lines rectangle over texture, scaled and centered on screen
            DrawRectangleLines(GetScreenWidth() / 2 - texMaze.width * MAZE_DRAW_SCALE / 2, GetScreenHeight() / 2 - texMaze.height * MAZE_DRAW_SCALE / 2, MAZE_WIDTH * MAZE_DRAW_SCALE, MAZE_HEIGHT * MAZE_DRAW_SCALE, RED);

            // Draw Mouse Position if in bounds
            if (selectedCell.x >= 0 && selectedCell.x < MAZE_WIDTH && selectedCell.y >= 0 && selectedCell.y < MAZE_HEIGHT)
                DrawRectangleLines(mazePosition.x + selectedCell.x * MAZE_DRAW_SCALE, mazePosition.y + selectedCell.y * MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, BLUE);

            // DONE: Draw player using a rectangle, consider maze screen coordinates!
            DrawRectangle(mazePosition.x + playerCell.x * MAZE_DRAW_SCALE, mazePosition.y + playerCell.y * MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, GREEN);

            // End cell drawn in red in order to see finish position
            DrawRectangle(mazePosition.x + endCell.x * MAZE_DRAW_SCALE, mazePosition.y + endCell.y * MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, RED);

            // DONE: Draw editor UI required elements -> TIP: raygui immediate mode UI
            // NOTE: In immediate-mode UI, logic and drawing is defined together
            // REFERENCE: https://github.com/raysan5/raygui
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                editRowSpace = CheckCollisionPointRec(GetMousePosition(), rowSpaceRec);
                editColSpace = CheckCollisionPointRec(GetMousePosition(), colSpaceRec);
                editSeed = CheckCollisionPointRec(GetMousePosition(), seedRec);
                editSkipChance = CheckCollisionPointRec(GetMousePosition(), skipRec);
            }
            GuiSpinner(rowSpaceRec, "Row Spacing", &spacingRows, 1, 8, editRowSpace);
            GuiSpinner(colSpaceRec, "Column Spacing", &spacingCols, 1, 8, editColSpace);
            GuiValueBox(seedRec, "Seed", &seed, 0, 99999, editSeed);
            GuiValueBox(skipRec, "Skip Chance", &skipChance, 0, 100, editSkipChance);
            if (GuiButton(buttonRec, "Generate"))
            {
                SetRandomSeed(seed);
                imMaze = GenImageMazeEx(MAZE_WIDTH, MAZE_HEIGHT, spacingRows, spacingCols, (float)skipChance / 100);
                updateMap = true;
            }
        }
        break;
        default:
            break;
        }

        DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(texMaze); // Unload maze texture from VRAM (GPU)
    UnloadImage(imMaze);    // Unload maze image from RAM (CPU)

    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

// Generate procedural maze image, using grid-based algorithm
// NOTE: Black=Walkable cell, White=Wall/Block cell
static Image GenImageMazeEx(int width, int height, int spacingRows, int spacingCols, float skipChance)
{
    // Generate image of plain color (BLACK)
    Image imMaze = GenImageColor(width, height, BLACK);

    // Allocate an array of point used for maze generation
    // NOTE: Dynamic array allocation, memory allocated in HEAP (MAX: Available RAM)
    Point *mazePoints = (Point *)malloc(MAZE_WIDTH * MAZE_HEIGHT * sizeof(Point));
    int mazePointsCounter = 0;

    // Start traversing image data, line by line, to paint our maze
    for (int y = 0; y < imMaze.height; y++)
    {
        for (int x = 0; x < imMaze.width; x++)
        {
            // Check image borders (1 px)
            if ((x == 0) || (x == (imMaze.width - 1)) || (y == 0) || (y == (imMaze.height - 1)))
            {
                ImageDrawPixel(&imMaze, x, y, WHITE); // Image border pixels set to WHITE
            }
            else
            {
                // Check pixel module to set maze corridors width and height
                if ((x % spacingCols == 0) && (y % spacingRows == 0))
                {
                    // Get change to define a point for further processing
                    float chance = (float)GetRandomValue(0, 100) / 100.0f;

                    if (chance >= skipChance)
                    {
                        // Set point as wall...
                        ImageDrawPixel(&imMaze, x, y, WHITE);

                        // ...save point for further processing
                        mazePoints[mazePointsCounter] = (Point){x, y};
                        mazePointsCounter++;
                    }
                }
            }
        }
    }

    // Define an array of 4 directions for convenience
    Point directions[4] = {
        {0, -1}, // Up
        {0, 1},  // Down
        {-1, 0}, // Left
        {1, 0},  // Right
    };

    // Load a random sequence of points, to be used as indices, so,
    // we can access mazePoints[] randomly indexed, instead of following the order we gor them
    int *pointIndices = LoadRandomSequence(mazePointsCounter, 0, mazePointsCounter - 1);

    // Process every random maze point, moving in one random direction,
    // until we collision with another wall (WHITE pixel)
    for (int i = 0; i < mazePointsCounter; i++)
    {
        Point currentPoint = mazePoints[pointIndices[i]];
        Point currentDir = directions[GetRandomValue(0, 3)];
        currentPoint.x += currentDir.x;
        currentPoint.y += currentDir.y;

        // Keep incrementing wall in selected direction until a WHITE pixel is found
        // NOTE: We only check against the color.r component
        while (GetImageColor(imMaze, currentPoint.x, currentPoint.y).r != 255)
        {
            ImageDrawPixel(&imMaze, currentPoint.x, currentPoint.y, WHITE);

            currentPoint.x += currentDir.x;
            currentPoint.y += currentDir.y;
        }
    }

    UnloadRandomSequence(pointIndices);

    return imMaze;
}

// DONE: EXTRA: [10p] Get shorter path between two points, implements pathfinding algorithm: A*
// NOTE: The functions returns an array of points and the pointCount
static Point *LoadPathAStar(Image map, Point start, Point end, int *pointCount)
{
    Point *path = (Point *)malloc(map.height * map.width * sizeof(Point *));
    int pathCounter = 0;

    // PathNode struct definition
    // NOTE: This is a possible useful struct but it's not a requirement
    typedef struct PathNode PathNode;
    struct PathNode
    {
        Point p;
        int gvalue;
        int hvalue;
        PathNode *parent;
    };

    // DONE: Implement A* algorithm logic
    // NOTE: This function must be self-contained!
    // Initialize the start and end nodes
    PathNode startNode = {start, 0, 0, NULL};
    PathNode endNode = {end, 0, 0, NULL};

    int frontierSize = 0;
    PathNode *frontier = (PathNode *)malloc(map.height * map.width * sizeof(PathNode));
    frontier[frontierSize] = startNode;
    frontierSize++;
    int reachedSize = 0;
    PathNode *reached = (PathNode *)malloc(map.height * map.width * sizeof(PathNode));
    reached[reachedSize] = startNode;
    reachedSize++;

    // Get all nodes in
    while (frontierSize > 0)
    {
        // Unqueue first frontier item to the current node
        PathNode currentNode = frontier[0];
        for (int i = 0; i < frontierSize - 1; i++)
        {
            frontier[i] = frontier[i + 1];
        }
        frontierSize--;

        // Get neighbors of the current node
        Point neighbors[4] = {
            {currentNode.p.x, currentNode.p.y - 1},
            {currentNode.p.x - 1, currentNode.p.y},
            {currentNode.p.x, currentNode.p.y + 1},
            {currentNode.p.x + 1, currentNode.p.y},
        };

        // Set all 4 neighbors as sons of the current node and save them to both Node Paths
        for (int i = 0; i < 4; i++)
        {
            // Gvalue is distance from startNode and Hvalue is distance to endNode
            int heuristic = abs(neighbors[i].x - endNode.p.x) + abs(neighbors[i].y - endNode.p.y);
            PathNode neighbor = {neighbors[i], currentNode.gvalue + 1, heuristic, NULL};

            bool isValid = (neighbor.p.x >= 0) && (neighbor.p.y >= 0) && (neighbor.p.x < map.width) && (neighbor.p.y < map.height) && (GetImageColor(map, neighbor.p.x, neighbor.p.y).r == 0);
            if (isValid)
            {
                int isInReached = -1;
                for (int j = 0; j < reachedSize; j++)
                {
                    if ((reached[j].p.x == neighbor.p.x) && (reached[j].p.y == neighbor.p.y))
                    {
                        isInReached = j;
                        break;
                    }
                }

                // If node was not reached before or is closer to the start node in the new path save it
                if ((isInReached == -1) || (neighbor.gvalue < reached[isInReached].gvalue))
                {
                    reached[reachedSize] = neighbor;
                    // Search for the node's parent on the reached array to save it in the array
                    for (int j = 0; j < reachedSize; j++)
                    {
                        if ((reached[j].p.x == currentNode.p.x) && (reached[j].p.y == currentNode.p.y))
                        {
                            reached[reachedSize].parent = &reached[j];
                            break;
                        }
                    }
                    reachedSize++;
                    // save the node on the frontier queue too
                    frontier[frontierSize] = neighbor;
                    frontierSize++;
                }
            }
        }

        // Reorder frontier array by gValue + hValue descending (gvalue + hvalue = priority on queue)
        for (int i = 0; i < frontierSize; i++)
        {
            int maxGHIndex = i;
            for (int j = 0; j < frontierSize; j++)
            {
                if ((frontier[maxGHIndex].gvalue + frontier[maxGHIndex].hvalue) < (frontier[j].gvalue + frontier[j].hvalue))
                {
                    maxGHIndex = j;
                }
            }
            PathNode aux = frontier[i];
            frontier[i] = frontier[maxGHIndex];
            frontier[maxGHIndex] = aux;
        }
    }

    // Save the optimal path to "path" array backtracking from the endNode
    // The pathCounter < reachedSize in the while loop exists to prevent the program from a memory leak
    Point current = endNode.p;
    while ((current.x != startNode.p.x || current.y != startNode.p.y) && pathCounter < reachedSize)
    {
        for (int i = 0; i < reachedSize; i++)
        {
            if ((current.x == reached[i].p.x) && (current.y == reached[i].p.y))
            {
                path[pathCounter] = current;
                pathCounter++;
                current = reached[i].parent->p;
                break;
            }
        }
    }

    // Free up reached and fromtier arrays memory as they are no longer needed
    free(reached);
    free(frontier);

    *pointCount = pathCounter; // Return number of path points
    return path;               // Return path array (dynamically allocated)
}