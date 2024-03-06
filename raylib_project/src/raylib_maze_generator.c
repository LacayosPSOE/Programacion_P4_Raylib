/*******************************************************************************************
*
*   raylib project template
*
*   <Game title>
*   <Game description>
*
*   This game has been created using raylib (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include <stdlib.h>

#define MAZE_WIDTH      64
#define MAZE_HEIGHT     64
#define MAZE_DRAW_SCALE  6.0f

typedef struct Point {
    int x;
    int y;
} Point;

Image GenImageMaze(int width, int height, float skipChance);

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib maze generation");
    
    SetRandomSeed(7382372);
    
    Image imMaze = GenImageMaze(MAZE_WIDTH, MAZE_HEIGHT, 0.75f);
    Texture2D texMaze = LoadTextureFromImage(imMaze);
    
    Mesh meshMaze = GenMeshCubicmap(imMaze, (Vector3){ 1.0f, 1.0f, 1.0f });
    Model mdlMaze = LoadModelFromMesh(meshMaze);
    
    Vector3 modelPosition = { 0.0f, 0.0f, 0.0f };  // Set model position

    Vector2 mazePosition = { GetScreenWidth()/2 - texMaze.width*MAZE_DRAW_SCALE/2, GetScreenHeight()/2 - texMaze.height*MAZE_DRAW_SCALE/2 };
    Point playerCell = { 1, 1 };
    
    Camera2D camera2d = { 0 };
    camera2d.target = (Vector2){ mazePosition.x + playerCell.x*MAZE_DRAW_SCALE, mazePosition.y + playerCell.y*MAZE_DRAW_SCALE };
    camera2d.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
    camera2d.rotation = 0.0f;
    camera2d.zoom = 5.0f;
    
    Camera3D cameraFP = { 0 };
    cameraFP.position = (Vector3){ 1.5f, 0.5f, 1.5f };    // Camera position
    cameraFP.target = (Vector3){ 1.5f, 0.5f, 2.0f };      // Camera looking at point
    cameraFP.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    cameraFP.fovy = 45.0f;                                // Camera field-of-view Y
    cameraFP.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    Texture2D texWalls[4] = { 0}; 
    texWalls[0] = LoadTexture("resources/maze_atlas01.png");
    texWalls[1] = LoadTexture("resources/maze_atlas02.png");
    texWalls[2] = LoadTexture("resources/maze_atlas03.png");
    texWalls[3] = LoadTexture("resources/maze_atlas04.png");
    int currentBiome = 0;
    
    // NOTE: By default each cube is mapped to one part of texture atlas
    mdlMaze.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texWalls[0];    // Set map diffuse texture
    
    int currentMode = 0;    // 0-Game2D, 1-Game3D, 2-MazeEditor

    SetTargetFPS(30);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (IsKeyPressed(KEY_Z)) currentMode = 0;
        else if (IsKeyPressed(KEY_X)) currentMode = 1;
        else if (IsKeyPressed(KEY_C)) currentMode = 2;
        
        switch (currentMode)
        {
            case 0:
            {
                Point prevPlayerCell = playerCell;
                
                if (IsKeyDown(KEY_UP)) playerCell.y -= 1;
                if (IsKeyDown(KEY_DOWN)) playerCell.y += 1;
                if (IsKeyDown(KEY_LEFT)) playerCell.x -= 1;
                if (IsKeyDown(KEY_RIGHT)) playerCell.x += 1;
                
                if (GetImageColor(imMaze, playerCell.x, playerCell.y).r == 255) playerCell = prevPlayerCell;
                
                // Update camera2d target and zoom
                camera2d.target = (Vector2){ mazePosition.x + playerCell.x*MAZE_DRAW_SCALE, mazePosition.y + playerCell.y*MAZE_DRAW_SCALE };

                camera2d.zoom += ((float)GetMouseWheelMove()*0.5f);
                if (camera2d.zoom > 6.0f) camera2d.zoom = 6.0f;
                else if (camera2d.zoom < 1.0f) camera2d.zoom = 1.0f;
            } break;
            case 1:
            {
                UpdateCamera(&cameraFP, CAMERA_FIRST_PERSON);
            } break;
            case 2:
            {
                
            }
            default: break;
        }
        
        // Change biome logic
        if (IsKeyPressed(KEY_ONE)) currentBiome = 0;
        else if (IsKeyPressed(KEY_TWO)) currentBiome = 1;
        else if (IsKeyPressed(KEY_THREE)) currentBiome = 2;
        else if (IsKeyPressed(KEY_FOUR)) currentBiome = 3;
        
        if (IsKeyPressed(KEY_ENTER)) ExportMesh(meshMaze, "my_cool_mesh.obj");
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);
            
            switch (currentMode)
            {
                case 0:
                {
                    BeginMode2D(camera2d);

                        //DrawTextureEx(texMaze, mazePosition, 0.0f, MAZE_DRAW_SCALE, WHITE);
                        for (int y = 0; y < imMaze.height; y++)
                        {
                            for (int x = 0; x < imMaze.width; x++)
                            {
                                if (GetImageColor(imMaze, x, y).r == 255)
                                {
                                    DrawTexturePro(texWalls[currentBiome], (Rectangle){ 0, texWalls[currentBiome].height/2, texWalls[currentBiome].width/2, texWalls[currentBiome].height/2 }, (Rectangle){ mazePosition.x + x*MAZE_DRAW_SCALE, mazePosition.y + y*MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE }, (Vector2){ 0,0 }, 0.0f, WHITE);
                                }
                                else
                                {
                                    DrawTexturePro(texWalls[currentBiome], (Rectangle){ 0, 0, texWalls[currentBiome].width/2, texWalls[currentBiome].height/2 }, (Rectangle){ mazePosition.x + x*MAZE_DRAW_SCALE, mazePosition.y + y*MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE }, (Vector2){ 0,0 }, 0.0f, WHITE);
                                }
                            }
                        }
                        
                        DrawRectangleLines(mazePosition.x, mazePosition.y, texMaze.width*MAZE_DRAW_SCALE, texMaze.height*MAZE_DRAW_SCALE, GREEN);
                        
                        // Draw player
                        DrawRectangle(mazePosition.x + playerCell.x*MAZE_DRAW_SCALE, mazePosition.y + playerCell.y*MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, GREEN);
                        
                    EndMode2D();
                } break;
                case 1:
                {
                    BeginMode3D(cameraFP);
                        DrawModel(mdlMaze, modelPosition, 1.0f, WHITE);   // Draw maze map
                    EndMode3D();
                } break;
                case 2:
                {
                    
                }
                default: break;
            }
            
            
            
            //DrawTexture(texWalls[currentBiome], 0, 0, WHITE);

            /*
            for (int i = 0; i < pointsCounter; i++)
            {
                DrawRectangle(GetScreenWidth()/2 - texMaze.width*MAZE_DRAW_SCALE/2 + points[i].x*MAZE_DRAW_SCALE, GetScreenHeight()/2 - texMaze.height*MAZE_DRAW_SCALE/2 + points[i].y*MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, RED);
            }
            */

            DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(texMaze);
    UnloadImage(imMaze);
    
    UnloadTexture(texWalls[currentBiome]);
    
    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

Image GenImageMaze(int width, int height, float skipChance)
{
    Image imMaze = GenImageColor(width, height, BLACK);
    
    //Point points[width*height] = { 0 };
    Point *points = (Point *)malloc(MAZE_WIDTH*MAZE_HEIGHT*sizeof(Point));
    int pointsCounter = 0;

    for (int y = 0; y < imMaze.height; y++)
    {
        for (int x = 0; x < imMaze.width; x++)
        {
            if ((x == 0) || (x == (imMaze.width - 1)) ||
                (y == 0) || (y == (imMaze.height - 1)))
            {
                ImageDrawPixel(&imMaze, x, y, WHITE); 
            }
            else
            {
                if ((x%3 == 0) && (y%3 == 0))
                {
                    float chance = (float)GetRandomValue(0, 100)/100.0f;
                    
                    if (chance >= skipChance) 
                    {
                        ImageDrawPixel(&imMaze, x, y, WHITE);
                        
                        points[pointsCounter].x = x;
                        points[pointsCounter].y = y;
                        pointsCounter++;
                    }
                }
            }
        }
    }
    
    Point directions[4] = {
        { 0, -1 },
        { 0, 1 },
        { -1, 0 },
        { 1, 0 }
    };
    
    int *pointsIndex = LoadRandomSequence(pointsCounter, 0, pointsCounter - 1);
    
    for (int i = 0; i < pointsCounter; i++)
    {
        Point currentPoint = points[pointsIndex[i]];
        Point currentDir = directions[GetRandomValue(0, 3)];
        currentPoint.x += currentDir.x;
        currentPoint.y += currentDir.y;
        
        while (GetImageColor(imMaze, currentPoint.x, currentPoint.y).r != 255)
        {
            ImageDrawPixel(&imMaze, currentPoint.x, currentPoint.y, WHITE);
            
            currentPoint.x += currentDir.x;
            currentPoint.y += currentDir.y;
        }
    }

    UnloadRandomSequence(pointsIndex);

    return imMaze;
}