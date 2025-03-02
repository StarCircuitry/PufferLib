#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include <stdio.h>

// for the purposes of these actions, I'm assuming pointy-top hex orientation
// could separate direction from action choice in future
const unsigned char NOOP = 0;
const unsigned char MOVE_NORTHEAST = 1;
const unsigned char MOVE_EAST = 2;
const unsigned char MOVE_SOUTHEAST = 3;
const unsigned char MOVE_SOUTHWEST = 4;
const unsigned char MOVE_WEST = 5;
const unsigned char MOVE_NORTHWEST = 6;

const unsigned char CONSUME_INPLACE = 7;
const unsigned char CONSUME_NORTHEAST = 8;
const unsigned char CONSUME_EAST = 9;
const unsigned char CONSUME_SOUTHEAST = 10;
const unsigned char CONSUME_SOUTHWEST = 11;
const unsigned char CONSUME_WEST = 12;
const unsigned char CONSUME_NORTHWEST = 13;

// entities, placing values well outside what will be used in the environment
const unsigned char EMPTY = 20;
const unsigned char AGENT = 21;

// elements (to be expanded)
const unsigned char WATER = 3;
const unsigned char EARTH = 4;
const unsigned char WOOD = 5;
const unsigned char NULLELEM = 255;  // doesn't really matter, but well outside element range



typedef struct HexCoord HexCoord;
struct HexCoord {
    int q;
    int r;
    int index;
};

// Map helpers for {HexCoord -> unsigned char} maps
// Todo: comparison and operator functions for hexcoord

// get the index of a key (and therefore value)
int get_hexmap_index(HexCoord* keys, HexCoord key, int size) {
    for (int i = 0; i < size; i++) {
        if (key.q == keys[i].q && key.r == keys[i].r) {
            return i;
        }
    }
    return -1;  // Key not found
}

// get the value of a key
unsigned char hexmap_get(HexCoord* keys, HexCoord key, unsigned char* values, int size) {
    int index = get_hexmap_index(keys, key, size);

    if (index == -1) {
        return 255; // same as NULLELEM, doesn't matter as long as it's not confused with something else
    } else {
        return values[index];
    }
}


typedef struct Hexmap Hexmap;
struct Hexmap {
    unsigned char* observations;
    int* actions;
    float* rewards;
    unsigned char* terminals;
    int radius; // for these hexagonal maps, size = (2*radius + 1) + 2*SUM(2*radius - n) over n = 0, 1,..., radius-1. 
    int size;   // {radius = 1; size = 7}, {radius = 2; size = 19}, {radius = 3; size = 37},... TODO: auto-set
    int tick;
    int q;
    int r;
    int thirst;
    int hunger;
    HexCoord target_coord;
    HexCoord* terrain_coordinates;
    unsigned char* terrain_elements;
};

void allocate(Hexmap* env) {
    env->observations = (unsigned char*)calloc(env->size, sizeof(unsigned char));
    env->actions = (int*)calloc(1, sizeof(int));
    env->rewards = (float*)calloc(1, sizeof(float));
    env->terminals = (unsigned char*)calloc(1, sizeof(unsigned char));
    env->terrain_coordinates = (HexCoord*)calloc(env->size, sizeof(HexCoord));
    env->terrain_elements = (unsigned char*)calloc(env->size, sizeof(unsigned char));
}

void free_allocated(Hexmap* env) {
    free(env->observations);
    free(env->actions);
    free(env->rewards);
    free(env->terminals);
    free(env->terrain_coordinates);
    free(env->terrain_elements);
}

// currently generates hexagonal maps only
void generate_terrain(HexCoord* terrain_coordinates, unsigned char* terrain_elements,
    unsigned char* observations, int radius, int size, Hexmap* env) {

    int r1;
    int r2;
    int index = 0;
    unsigned char terrain_elem;

    for (int q = -radius; q <= radius; q++) {

        if (-radius > -q-radius) {
            r1 = -radius;
        } else {
            r1 = -q-radius;
        }

        if (radius < -q+radius) {
            r2 = radius;
        } else {
            r2 = -q+radius;
        }

        for (int r = r1; r <= r2; r++) {

            if (index < size) {
                terrain_elem = (rand() % (WOOD - WATER + 1)) + WATER;
                env->target_coord.q = q;
                env->target_coord.r = r;

                env->terrain_coordinates[index].q = env->target_coord.q;
                env->terrain_coordinates[index].r = env->target_coord.r;
                env->terrain_coordinates[index].index = index;
                
                env->terrain_elements[index] = terrain_elem;
                env->observations[index] = terrain_elem;
                
                index += 1;
            }
        }
    }
}


void c_reset(Hexmap* env) {
    memset(env->observations, 0, env->size*sizeof(unsigned char));
    
    generate_terrain(env->terrain_coordinates, env->terrain_elements, env->observations, env->radius, env->size, env);

    env->q = 0;
    env->r = 0;
    env->hunger = 10;
    env->thirst = 10;

    // TODO: encapsulate as a setter
    env->target_coord.q = env->q;
    env->target_coord.r = env->r;
    env->target_coord.index = get_hexmap_index(env->terrain_coordinates, env->target_coord, env->size);

    if (env->target_coord.index >= 0 && env->target_coord.index < env->size){
        env->observations[env->target_coord.index] = AGENT;
    }
    env->tick = 0;
}


void c_step(Hexmap* env) {
    int action = env->actions[0];
    env->terminals[0] = 0;
    env->rewards[0] = 0;
    env->thirst -= 1;
    env->hunger -= 1;

    env->target_coord.q = env->q;
    env->target_coord.r = env->r;
    env->target_coord.index = get_hexmap_index(env->terrain_coordinates, env->target_coord, env->size);

    if (env->target_coord.index >= 0 && env->target_coord.index < env->size) {
        env->observations[env->target_coord.index] = env->terrain_elements[env->target_coord.index];
    }

    if (action == MOVE_NORTHEAST) {
        env->q += 1;
        env->r -= 1;
    } else if (action == MOVE_EAST) {
        env->q += 1;
    } else if (action == MOVE_SOUTHEAST) {
        env->r += 1;
    } else if (action == MOVE_SOUTHWEST) {
        env->q -= 1;
        env->r += 1;
    } else if (action == MOVE_WEST) {
        env->q -= 1;
    } else if (action == MOVE_NORTHWEST) {
        env->r -= 1;
    }
    else if (action == CONSUME_NORTHEAST) {
        env->target_coord.q += 1;
        env->target_coord.r -= 1;
    } else if (action == CONSUME_EAST) {
        env->target_coord.q += 1;
    } else if (action == CONSUME_SOUTHEAST) {
        env->target_coord.r += 1;
    } else if (action == CONSUME_SOUTHWEST) {
        env->target_coord.q -= 1;
        env->target_coord.r += 1;
    } else if (action == CONSUME_WEST) {
        env->target_coord.q -= 1;
    } else if (action == CONSUME_NORTHWEST) {
        env->target_coord.r -= 1;
    }

    if (action >= CONSUME_INPLACE && action <= CONSUME_NORTHWEST) {
        env->target_coord.index = get_hexmap_index(env->terrain_coordinates,
            env->target_coord, env->size);

        if (env->target_coord.index >= 0 && env->target_coord.index < env->size) {
            unsigned char elem = env->terrain_elements[env->target_coord.index];

            env->terrain_elements[env->target_coord.index] = NULLELEM;
            env->observations[env->target_coord.index] = NULLELEM;

            switch (elem) {
                case WATER:
                    env->thirst += 5;
                    env->rewards[0] += 0.1;
                    break;
                case EARTH:
                    env->hunger -= 2;
                    env->thirst -= 2;
                    break;
                case WOOD:
                    env->hunger += 5;
                    env->rewards[0] += 0.1;
                    break;
            }
        }
    }

    env->target_coord.q = env->q;
    env->target_coord.r = env->r;
    env->target_coord.index = get_hexmap_index(env->terrain_coordinates, env->target_coord, env->size);

    if (env->hunger <= 0 
            || env->thirst <= 0
            || env->target_coord.index < 0
            || env->target_coord.index >= env->size) {
        env->terminals[0] = 1;
        env->rewards[0] -= 1.0;
        c_reset(env);
        return;
    }

    if (env->tick > 2*env->size) {
        env->terminals[0] = 1;
        env->rewards[0] += 1.0;
        c_reset(env);
        return;
    }

    env->tick += 1;
}



// TODO: set up client

typedef struct Client Client;
struct Client {
    //Texture2D ball;
};

Client* make_client(Hexmap* env) {
    Client* client = (Client*)calloc(1, sizeof(Client));
    int px = 64*env->size;
    InitWindow(px, px, "Hexmap Testing");
    SetTargetFPS(5);

    //client->agent = LoadTexture("resources/puffers_128.png");
    return client;
}

void close_client(Client* client) {
    CloseWindow();
    free(client);
}

void c_render(Client* client, Hexmap* env) {
    if (IsKeyDown(KEY_ESCAPE)) {
        exit(0);
    }

    BeginDrawing();
    ClearBackground((Color){6, 24, 24, 255});

    int q;
    int r;
    int px = 64;

    Color color;


    for (int i = 0; i < env->size; i++) {
        q = env->terrain_coordinates[i].q;
        r = env->terrain_coordinates[i].r;

        if (env->terrain_elements[i] == WATER) {
            color = (Color){0, 0, 255, 255};
        } else if (env->terrain_elements[i] == EARTH) {
            color = (Color){255, 255, 0, 255};
        } else if (env->terrain_elements[i] == WOOD) {
            color = (Color){0, 255, 0, 255};
        } else {
            color = (Color){0, 0, 0, 255};
        }

        DrawRectangle(q*px, r*px, px, px, color);

    }

    /*int px = 64;
    for (int i = 0; i < env->size; i++) {
        for (int j = 0; j < env->size; j++) {
            int tex = env->observations[i*env->size + j];
            if (tex == EMPTY) {
                continue;
            }
            Color color = (tex == AGENT) ? (Color){0, 255, 255, 255} : (Color){255, 0, 0, 255};
            DrawRectangle(j*px, i*px, px, px, color);
        }
    }*/
    EndDrawing();
}
