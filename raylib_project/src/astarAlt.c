// Calculate heuristic value for the start point
    int startHvalue = (abs(start.x - end.x) + abs(start.y - end.y));

    // Create start node
    PathNode startNode = {start, 0, startHvalue, NULL, false};

    // Initialize open list with start node
    // Allocate memory for open list
    // int maxNodes = map.width * map.height; // A heuristic to ensure enough space
    // PathNode *openList = (PathNode *)malloc(maxNodes * sizeof(PathNode));
    // if (openList == NULL) {
    //     *pointCount = 0;
    //     return NULL; // Memory allocation failed
    // }
    PathNode *openList = (PathNode *)malloc(map.width * map.height * sizeof(PathNode));
    int openListSize = 1;

    // Continue until open list is empty
    while (openListSize > 0)
    {
        // Find node with lowest f = g + h value in the open list
        int lowestFValue = INT_MAX;
        PathNode *currentNode = NULL;
        PathNode *current = openList;
        for (int i = 0; i < openListSize; i++)
        {
            int fValue = current->gvalue + current->hvalue;
            if (fValue < lowestFValue)
            {
                lowestFValue = fValue;
                currentNode = current;
            }
            current++;
        }

        // Remove current node from open list
        currentNode->closed = true;
        openListSize--;

        // Check if current node is the goal
        if (currentNode->p.x == end.x && currentNode->p.y == end.y)
        {
            // Reconstruct path
            Point *path = NULL;
            int length = 0;
            PathNode *temp = currentNode;
            // Count path length
            while (temp)
            {
                length++;
                temp = temp->parent;
            }
            // Allocate memory for path
            path = (Point *)malloc(length * sizeof(Point));

            // Fill path array
            temp = currentNode;
            for (int i = length - 1; i >= 0; i--)
            {
                path[i] = temp->p;
                temp = temp->parent;
            }
            pathCounter = length;
            break;
        }

        // Generate neighbors of current node
        Point neighbors[] = {
            {currentNode->p.x - 1, currentNode->p.y},
            {currentNode->p.x + 1, currentNode->p.y},
            {currentNode->p.x, currentNode->p.y - 1},
            {currentNode->p.x, currentNode->p.y + 1}};

        // Process each neighbor
        for (int i = 0; i < 4; i++)
        {
            Point neighbor = neighbors[i];

            // Skip invalid neighbors or closed nodes
            bool isValid = (neighbor.x >= 0) && (neighbor.y >= 0) && (neighbor.x < map.width) && (neighbor.y < map.height) && (GetImageColor(map, neighbor.x, neighbor.y).r != 255);
            if (!isValid)
                continue;

            // Calculate tentative g value
            int tentativeGValue = currentNode->gvalue + 1;

            // Check if neighbor is already in open list
            bool inOpenList = false;
            for (int j = 0; j < openListSize; j++)
            {
                if (openList[j].p.x == neighbor.x && openList[j].p.y == neighbor.y)
                {
                    inOpenList = true;
                    break;
                }
            }

            // If neighbor is not in open list or has a lower g value, update it
            if (!inOpenList || tentativeGValue < currentNode->gvalue)
            {
                PathNode *neighborNode = NULL;
                if (!inOpenList)
                {
                    // Add neighbor to open list
                    openListSize++;
                    neighborNode = &openList[openListSize - 1];
                    neighborNode->p = neighbor;
                    neighborNode->parent = currentNode;
                    neighborNode->hvalue = (abs(neighbor.x - end.x) + abs(neighbor.y - end.y));
                }
                else
                {
                    // Find neighbor in open list
                    for (int j = 0; j < openListSize; j++)
                    {
                        if (openList[j].p.x == neighbor.x && openList[j].p.y == neighbor.y)
                        {
                            neighborNode = &openList[j];
                            break;
                        }
                    }
                }

                // Update neighbor node values
                neighborNode->gvalue = tentativeGValue;
                neighborNode->parent = currentNode;
            }
        }
    }