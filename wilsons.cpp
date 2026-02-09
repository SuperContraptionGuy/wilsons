#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef struct
{
    int x;
    int y;
} vec2;

vec2 add(vec2 a, vec2 b)
{
    return vec2{a.x + b.x, a.y + b.y};
}

vec2 add(vec2 a, int b)
{
    return vec2{a.x + b, a.y + b};
}

vec2 scale(vec2 a, double b)
{
    return vec2{int(a.x * b), int(a.y * b)};
}

typedef struct __attribute__((packed))
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb24;

typedef struct __attribute__((packed))
{
    rgb24* pixels;
    size_t numberOfPixels;
    size_t numberOfBytes;
} rgb24_frame;

typedef struct
{
    vec2 screenSize;
    FILE* ffmpegStdIn;

    rgb24_frame frameBuffer;

} rendererProps;


void initializeRenderer(rendererProps* renderer, int x, int y, int fps, char* filename)
{
    renderer->screenSize = {x, y};
    renderer->frameBuffer.numberOfPixels = renderer->screenSize.x * renderer->screenSize.y;
    renderer->frameBuffer.numberOfBytes = sizeof(rgb24) * renderer->frameBuffer.numberOfPixels;
    renderer->frameBuffer.pixels = (rgb24*)malloc(renderer->frameBuffer.numberOfBytes);

    char ffmpegCommand[500] = {0};
    char ffmpegFormatString[] = "ffmpeg -s %ix%i -f rawvideo -pix_fmt rgb24 -r %i -i pipe:0 -pix_fmt yuv420p -profile:v high -level:v 4.1 -crf:v 20 \"%s.mp4\"";
    //char ffmpegFormatString[] = "tee log | ffmpeg -s %ix%i -f rawvideo -pix_fmt rgb24 -r %i -i pipe:0 -pix_fmt yuv420p -profile:v high -level:v 4.1 -crf:v 20 \"%s.mp4\"";
    snprintf(ffmpegCommand, 500, ffmpegFormatString, renderer->screenSize.x, renderer->screenSize.y, fps, filename);
    renderer->ffmpegStdIn = popen(ffmpegCommand, "w");

}

void destroyRenderer(rendererProps* renderer)
{
    free(renderer->frameBuffer.pixels);
    pclose(renderer->ffmpegStdIn);
}

void testRender(rendererProps* renderer)
{
    int frames;
    for(int frame = 0; frame < frames; frame++)
    { 
        for(int y = 0; y < renderer->screenSize.y; y++)
        {
            for(int x = 0; x < renderer->screenSize.x; x++)
            {
                rgb24 pixel = {(uint8_t)(x % 256), uint8_t(y % 256), uint8_t(frame % 256)};

                fwrite(&pixel, sizeof(uint8_t), 3, renderer->ffmpegStdIn);

            }
        }
    }
}

void writeFrame(rendererProps* renderer)
{
    fwrite(renderer->frameBuffer.pixels, sizeof(rgb24), renderer->screenSize.x * renderer->screenSize.y, renderer->ffmpegStdIn);
}

void setPixel(rgb24* pixel, uint8_t r, uint8_t g, uint8_t b)
{
    pixel->r = r;
    pixel->g = g;
    pixel->b = b;
}

void setPixel(rendererProps* renderer, vec2 coords, uint8_t r, uint8_t g, uint8_t b)
{
    rgb24* pixel = &renderer->frameBuffer.pixels[coords.x + coords.y * renderer->screenSize.x];
    setPixel(pixel, r, g, b);
}

void setPixel(rendererProps* renderer, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    vec2 coords = {x, y};
    setPixel(renderer, coords, r, g, b);
}

void setPixel(rendererProps* renderer, vec2 coords, rgb24 pixel)
{
    setPixel(renderer, coords, pixel.r, pixel.g, pixel.b);
}

void testFrameRender(rendererProps* renderer)
{

    //printf("starting test render \n");
    int frames = 256;
    for(int frame = 0; frame < frames; frame++)
    { 
        for(int x = 0; x < renderer->screenSize.x; x++)
        {
            for(int y = 0; y < renderer->screenSize.y; y++)
            {
                setPixel(renderer, x, y, (uint8_t)(x % 256), uint8_t(y % 256), uint8_t(frame % 256));
            }
        }

        writeFrame(renderer);
    }

}

void drawFill(rendererProps* renderer, rgb24 color)
{
    for(int y = 0; y < renderer->screenSize.x; y++)
    {
        for(int x = 0; x < renderer->screenSize.x; x++)
        {
            setPixel(renderer, vec2{x, y}, color);
        }
    }
}

void restrictBounds(rendererProps* renderer, vec2* vector)
{
    if(vector->x < 0)
        vector->x = 0;
    if(vector->x > renderer->screenSize.x)
        vector->x = renderer->screenSize.x;
    if(vector->y < 0)
        vector->y = 0;
    if(vector->y > renderer->screenSize.y)
        vector->y = renderer->screenSize.y;
}

int checkBounds(rendererProps* renderer, vec2 vector)
{
    if(vector.x < 0)
        return 0;
    if(vector.x > renderer->screenSize.x)
        return 0;
    if(vector.y < 0)
        return 0;
    if(vector.y > renderer->screenSize.y)
        return 0;

    return 1;
}

void drawBox(rendererProps* renderer, vec2 pos, vec2 size, rgb24 color)
{
    if(size.x < 0)
        size.x = 0;
    if(size.y < 0)
        size.y = 0;
    vec2 start = {pos.x, pos.y};
    restrictBounds(renderer, &start);
    vec2 end = {size.x + pos.x, size.y + pos.y};
    restrictBounds(renderer, &end);

    for(int y = start.y; y < end.y; y++)
    {
        for(int x = start.x; x < end.x; x++)
        {
            setPixel(renderer, vec2{x, y}, color);
        }
    }
}

void drawCircle(rendererProps* renderer, vec2 pos, int r, rgb24 color)
{
    if(r < 0)
        r = 0;

    // draw it in horizontal slices
    vec2 start = {0, pos.y - r};
    restrictBounds(renderer, &start);
    vec2 end = {0, pos.y + r};
    restrictBounds(renderer, &end);

    for(int y = start.y; y < end.y; y++)
    {
        int a = y - pos.y;
        int b = sqrt(pow(r, 2) - pow(a, 2));

        start.x = pos.x - b;
        restrictBounds(renderer, &start);
        end.x = pos.x + b;
        restrictBounds(renderer, &end);

        for(int x = start.x; x < end.x; x++)
        {
            setPixel(renderer, vec2{x, y}, color);
        }
    }
}

void drawLine(rendererProps* renderer, vec2 pos1, vec2 pos2, rgb24 color)
{
    int dx = pos2.x - pos1.x;
    int dy = pos2.y - pos1.y;
    float x = pos1.x;
    float y = pos1.y;
    if(abs(dx) >= abs(dy) && dx != 0)
    {
        // using x coord
        float m = (float)dy / (float)dx;
        for(; (int)x != (int)pos2.x; x += dx / abs(dx))
        {
            y = m * (x - pos1.x) + pos1.y;
            vec2 coords = vec2{(int)x, (int)round(y)};
            if(checkBounds(renderer, coords))
                setPixel(renderer, coords, color);
        }
    } else if(dy != 0)
    {
        // using y coord
        float m = (float)dx / (float)dy;
        for(; (int)y != (int)pos2.y; y += dy / abs(dy))
        {
            //y = m * (x - pos1.x) + pos1.y;
            x = m * (y - pos1.y) + pos1.x;
            vec2 coords = vec2{(int)round(x), (int)y};
            if(checkBounds(renderer, coords))
                setPixel(renderer, coords, color);
        }
    } else
    {
        // both dx and dy are zero, just do a point
        setPixel(renderer, pos1, color);
    }
}

void testDraws(rendererProps* renderer)
{
    int frames = 100;
    for(int frame = 0; frame < frames; frame++)
    {
        drawFill(renderer, rgb24{0, 0, 0});

        drawCircle(renderer, vec2{50 + (int)(50. * cos(frame * 2. * M_PI / 120.)), 50 + (int)(50. * sin(frame * 2. * M_PI / 120.))}, 50, rgb24{0, 0, 255});
        drawBox(renderer, (vec2){frame - 25, 5}, (vec2){50, frame+50}, rgb24{255, 255, 255});
        drawBox(renderer, (vec2){50, 75 - frame}, (vec2){50, 50}, rgb24{255, 255, 255});
        drawCircle(renderer, vec2{50 + (int)(10. * cos(frame * 2. * M_PI / 30.)), 50 + (int)(10. * sin(frame * 2. * M_PI / 30.))}, 25, rgb24{255, 0, 0});
        drawLine(renderer, vec2{50 + (int)(30. * cos(frame * 2. * M_PI / 60.)), 50 + (int)(30. * sin(frame * 2. * M_PI / 60.))}, vec2{50 + (int)(-100. * cos(frame * 2. * M_PI / 60.)), 50 + (int)(-100. * sin(frame * 2. * M_PI / 60.))}, rgb24{0, 255, 0});
        writeFrame(renderer);
    }
}

// utilities


// maze related stuff
//
//

typedef enum
{
    TREE,
    UNUSED,
    RANDOMWALK
} list_affiliation_t;

typedef struct linked_node
{
    // tree links
    linked_node* parent;
    linked_node* children[4];   // max number of children for a node
    size_t numberOfChildren;

    // random walk and unlinked doubly linked lists. 
    linked_node* next;
    linked_node* previous;

    list_affiliation_t listAffiliation; // determines which graph it's associated with

    vec2 position;
} linked_node;

typedef struct
{
    linked_node* root;      // root of acyclic graph (tree)
    list_affiliation_t listName;
} linked_acyclic_graph;

typedef struct
{
    linked_node* beginning;
    linked_node* end;

    size_t length;
    list_affiliation_t listName;

} doubly_linked_list;

void pushNodeBack(doubly_linked_list* list, linked_node* node)
{
    // check if list is empty
    if(list->end == NULL)
    {
        list->beginning = node;
        list->end = node;
        node->next = NULL;
        node->previous = NULL;
    } else
    {
        // add it to the end of the list
        node->next = NULL;
        node->previous = list->end;
        list->end->next = node;
        list->end = node;
    }

    list->length++;
    node->listAffiliation = list->listName;
}

void pushNodeFront(doubly_linked_list*list, linked_node* node)
{
    if(list->beginning == NULL)
    {
        list->beginning = node;
        list->end = node;
        node->next = NULL;
        node->previous = NULL;
    } else
    {
        node->previous = NULL;
        node->next = list->beginning;
        list->beginning->previous = node;
        list->beginning = node;
    }

    list->length++;
    node->listAffiliation = list->listName;
}

linked_node* popNodeBack(doubly_linked_list* list)
{
    // check if list is emty
    if(list->end == NULL)
        return NULL;
    linked_node* node = list->end;
    list->end = list->end->previous;    // handles case where there was only one node, since previous would be null in that case
    list->end->next = NULL;
    if(list->end == NULL)
        list->beginning = NULL; // set beginning if the list is now empty

    list->length--;
    return node;
}

linked_node* popNodeFront(doubly_linked_list* list)
{
    // if list is empty
    if(list->beginning == NULL)
        return NULL;
    linked_node* node = list->beginning;
    list->beginning = list->beginning->next;
    list->beginning->previous = NULL;
    if(list->beginning == NULL)
        list->end = NULL;

    list->length--;

    return node;
}

void addNodeBefore(doubly_linked_list* list, linked_node* next, linked_node* node)
{
    // is next the first item? should work in either case
    node->previous = next->previous;
    node->next = next;
    next->previous->next = node;
    next->previous = node;

    list->length++;
    node->listAffiliation = list->listName;
}

void addNodeAfter(doubly_linked_list* list, linked_node* previous, linked_node* node)
{
    node->previous = previous;
    node->next = previous->next;
    previous->next->previous = node;
    previous->next = node;

    list->length++;
    node->listAffiliation = list->listName;
}

void removeNode(doubly_linked_list* list, linked_node* node)
{
    if(!node)
    {
        printf("null pointer at remove node... grr...\n");
        return;
    }
    if(!node->next)
    {
        // it's the last item
        popNodeBack(list);
        return;
    } if(!node->previous)
    {
        // it's the first item
        popNodeFront(list);
        return;
    } if(node->next && node->previous)
    {
        // if it has both neighbors
        node->previous->next = node->next;
        node->next->previous = node->previous;
        list->length--;
        return;
    }
}

linked_node* getNthAfter(linked_node* node, size_t n)
{
    //linked_node* nextNode = node;
    //for(size_t i = 0; (i < n) && ((nextNode = nextNode->next) != NULL); i++);
    //for(size_t i = 0; i <= n; i++)
    //size_t i = 0;
    //while((n--) && (node != NULL))
    while(n-- && (node != NULL))
    {
        node = node->next;
    }
    // n is underflow after loop if it started at 0, fyi

    return node;    // returns null if there aren't n nodes
}

void testLinkedLists()
{
    doubly_linked_list list = {0};
    list.listName = UNUSED;

    // test pushNodeBack
    //  empty?
    linked_node node = {0};
    pushNodeBack(&list, &node);
    if(     !(list.beginning == &node &&
                    list.end == &node &&
                 list.length == 1 &&
        node.listAffiliation == list.listName &&
               node.previous == NULL &&
                   node.next == NULL ))
        printf("pushNodeBack() failed it's test for an empty list. for shame...\n");
    // not empty?
    linked_node node2 = {0};
    pushNodeBack(&list, &node2);
    if(!( list.beginning        ==  &node           &&
          list.end              ==  &node2          &&
          list.length           ==  2               &&
          node.previous         ==  NULL            &&
          node.next             ==  &node2          &&
          node.listAffiliation  ==  list.listName   &&
          node2.previous        ==  &node           &&
          node2.next            ==  NULL            &&
          node2.listAffiliation ==  list.listName   ))
        printf("pushNodeBack() failed it's test for not empty list.\n");


    //  only?
    //  First?
    //  Last?
    //  somewhere in the middle
}

typedef struct
{
    linked_node* nodes;     // node pool array
    linked_node** *map;     // node pointers in 2d array for coordinate addressing
    linked_acyclic_graph mazeIncluded;
    doubly_linked_list randomWalk;
    doubly_linked_list unused;

    vec2 size;
    size_t numberOfNodes;

    drand48_data PRNG;
} maze_t;

void initializeMaze(maze_t* maze, size_t x, size_t y)
{
    maze->size.x = x;
    maze->size.y = y;
    maze->numberOfNodes = x * y;

    maze->unused.listName = UNUSED;
    maze->randomWalk.listName = RANDOMWALK;
    maze->mazeIncluded.listName = TREE;

    // generate the pool of nodes
    maze->nodes = (linked_node*)malloc(maze->numberOfNodes * sizeof(linked_node));

    // set up the map
    maze->map = (linked_node***)malloc(x*sizeof(linked_node**));
    int n = 0;
    for(int i = 0; i < x; i++)
    {
        maze->map[i] = (linked_node**)malloc(y*sizeof(linked_node*));
        for(int j = 0; j < y; j++, n++)
        {
            linked_node* node = &maze->nodes[n];
            // clear the data for each node
            *node = {0};
            node->listAffiliation = UNUSED;
            node->position.x = i;
            node->position.y = j;
            maze->map[i][j] = node;
            // add all nodes to the unused list
            pushNodeBack(&maze->unused, node);
        }
    }

    // setup prng
    srand(1);
    srand48_r(rand(), &maze->PRNG);

    // choose a random cell to be initially included in the maze
    long int nodeIndex;
    lrand48_r(&maze->PRNG, &nodeIndex);
    nodeIndex = nodeIndex % maze->unused.length;
    linked_node* node = getNthAfter(maze->unused.beginning, nodeIndex);

    removeNode(&maze->unused, node);
    pushNodeFront(&maze->unused, node);
}

void destroyMaze(maze_t* maze)
{
    // take down the pool
    free(maze->nodes);

    // take down the map
    for(int i = 0; i < maze->size.x; i++)
    {
        free(maze->map[i]);
    }
    free(maze->map);
}

typedef struct
{
    int cellSize;
    int cellGridOffset;
    int cellGridSize;
} maze_renderer;

void renderLinkedList(rendererProps* renderer, maze_renderer* mazeRenderer, doubly_linked_list* linkedList, rgb24 cellColor, rgb24 lineColor)
{
    // check for empty list
    // render all the unused nodes, linked list
    linked_node* node = linkedList->beginning;
    if(!node)
        return;
    do
    {
        // draw this node
        vec2 pos = {node->position.x * mazeRenderer->cellGridSize + mazeRenderer->cellGridOffset, node->position.y * mazeRenderer->cellGridSize + mazeRenderer->cellGridOffset};
        drawBox(renderer, pos, vec2{mazeRenderer->cellSize, mazeRenderer->cellSize}, cellColor);
    } while((node = node->next) != NULL);

    // render unused linked list relations
    node = linkedList->beginning;
    do
    {
        if(node->previous)
        {
            //drawLine(
            // draw lines from previous to this
            drawLine(renderer,
                    add(scale(node->position, mazeRenderer->cellGridSize), mazeRenderer->cellGridOffset + mazeRenderer->cellSize / 2),
                    add(scale(node->previous->position, mazeRenderer->cellGridSize), mazeRenderer->cellGridOffset + mazeRenderer->cellSize / 2),
                    lineColor);
        }
    } while((node = node->next) != NULL);
}

void renderMaze(rendererProps* renderer, maze_t* maze)
{
    maze_renderer mazeRenderer = {10, 2, 0};
    mazeRenderer.cellGridSize = mazeRenderer.cellSize + mazeRenderer.cellGridOffset * 2;

    renderLinkedList(renderer, &mazeRenderer, &maze->unused, rgb24{50, 50, 50}, rgb24{50, 0, 0});
    renderLinkedList(renderer, &mazeRenderer, &maze->randomWalk, rgb24{30, 30, 200}, rgb24{230, 230, 230});

    // need boxes
    // need lines of thickness with end caps
}

void testMazeOperations(rendererProps* renderer)
{
    maze_t maze = {0};
    initializeMaze(&maze, 4, 5);

    printf("%i\n", (int)maze.unused.length);
    drawFill(renderer, rgb24{0, 0, 0});
    renderMaze(renderer, &maze);
    writeFrame(renderer);

    linked_node* node = popNodeFront(&maze.unused);

    printf("%i\n", (int)maze.unused.length);
    drawFill(renderer, rgb24{0, 0, 0});
    renderMaze(renderer, &maze);
    writeFrame(renderer);

    pushNodeBack(&maze.unused, node);

    printf("%i\n", (int)maze.unused.length);
    drawFill(renderer, rgb24{0, 0, 0});
    renderMaze(renderer, &maze);
    writeFrame(renderer);

    for(int i = 0; i < 60; i++)
    {
        writeFrame(renderer);
    }
}

void randomStep(maze_t* maze)
{

}

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        printf("include a file name in quotes to write to\n");
        return 1;
    }

    rendererProps renderer;
    initializeRenderer(&renderer, 100, 100, 60, argv[1]);

    //testRender(&renderer);
    //testFrameRender(&renderer);
    //testDraws(&renderer);
    testMazeOperations(&renderer);

    destroyRenderer(&renderer);

    return 0;
}
