/**
 * @file simulator.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-04-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "list.h"

/*type for a coordinate*/
typedef struct coord {
    int x;
    int y;
} Coord;

// Drone statuses
typedef enum {STATIONARY,MOVING,HELPING} drone_status;

// survivor status
typedef enum {NEEDHELP,HELPONWAY,UNDERHELP,HELPED} survivor_status;

/*type for a survivor*/
typedef struct survivor {
    survivor_status status;
    Coord coord;
    time_t discovery_time; /*you can use struct tm, too*/
    time_t helped_time;
    char info[25];
} Survivor;

/*type for a cell in the map*/
typedef struct mapcell {
    Coord coord;       /*in our program simply i,j*/
    List *survivors; /*the survivors waiting help in this cell*/
} MapCell;

/*2d map type*/
typedef struct map {
    int height; /*height of the map*/
    int width;  /*width of the map*/
    MapCell **cells; /*pointer to 2d MapCell*/
} Map;


/*type for a drone*/
typedef struct drone {
    drone_status status;
    int numberofhelped;
    Coord destination;
    Coord coord;
    time_t stime;
    char info[4];
    Coord velocity;
    /*you can add more fields*/
} Drone;

extern Map map;

/*lists*/
extern List *survivors;/* survivor list, waiting for help*/         
extern List *helped_survivors; /* helped survivor list*/
extern List *drones;  /*drone list*/

/*model.c*/
void init_map(int height, int width);
void freemap();
Survivor *create_survivor(Coord *coord, char *info, time_t *discovery_time);
void *survivor_generator(void *args);
void *drone_controller(); 

/*view.c*/
int init_sdl_window();
int draw_map();
int check_events();
int quit_all();