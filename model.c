/**
 * @file model.c
 * @author Enhar Apuhan & Mervenur Saraç
 * @brief
 * @version 0.1
 * @date 2024-04-21
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "simulator.h"
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "SDL2/SDL.h"
#define MAX_SURVIVOR_PER_CELL 3
#define MAX_DRONE_AMOUNT 5
#define MAX_DRONE_VELOCITY 1

extern SDL_bool done;

/*SOME EXAMPLE FUNCTIONS GIVEN BELOW*/
Map map;
int numberofcells = 0;
int numberofhelped = 0;
List *survivors;
List *drones;
List *helped_survivors;

void init_map(int height, int width) {
    map.height = height;
    map.width = width;
    numberofcells = height * width;
    survivors = create_list(sizeof(Survivor), numberofcells * MAX_SURVIVOR_PER_CELL);

    /*pointer array*/
    map.cells = malloc(sizeof(MapCell *) * map.height);

    for (int j = 0; j < map.height; j++) {
        /*rows for each pointer*/
        map.cells[j] = malloc(sizeof(MapCell) * map.width);
    }

    for (int i = 0; i < map.height; i++) {
        for (int j = 0; j < map.width; j++) {
            map.cells[i][j].coord.x = i;
            map.cells[i][j].coord.y = j; /**/
            map.cells[i][j].survivors = create_list(sizeof(Survivor), MAX_SURVIVOR_PER_CELL);
        }
    }
    printf("Map height: %d, width:%d\n", map.height, map.width);
}
void freemap() {
    for (int i = 0; i < map.height; i++) {
        for (int j = 0; j < map.width; j++) {
            List *list = map.cells[i][j].survivors;
            list->destroy(list);
        }
        free(map.cells[i]);
    }
    free(map.cells);
}
Survivor *create_survivor(Coord *coord, char *info, time_t *discovery_time) {
    Survivor *s = malloc(sizeof(Survivor));
    memset(s, 0, sizeof(Survivor));
    memcpy(&(s->discovery_time), discovery_time, sizeof(time_t));
    strncpy(s->info, info, sizeof(s->info));
    memcpy(&(s->coord), coord, sizeof(Coord));

    s->status = NEEDHELP;

    struct tm localt;
    localtime_r(&s->discovery_time,&localt);

    printf("survivor: %s\n", asctime(&localt));
    printf("%s\n\n", s->info);
    return s;
}

/*THREAD FUNCTION: generates random survivor
 */
void survivor_generator(void *args) {
    // generate random location
    if (map.cells != NULL) {
        time_t traw;
        struct tm t; /*used for localtime*/

        /*survivor info*/
        char info[5] = {'A' + (random() % 26), 
                        'A' + (random() % 26),
                        '0' + (random() % 9),
                        '0' + (random() % 9)}; 

        Coord coord = {random() % map.height, random() % map.width};

        time(&traw);
        localtime_r(&traw, &t);

        printf("creating survivor...%s\n", asctime(&t));
        Survivor *s = create_survivor(&coord, info, &traw);

        /*add to general list*/
        add(survivors, s);

        /*add to the list in the cell*/
        List *list = map.cells[coord.x][coord.y].survivors;
        add(list, s);

        printf("survivor added, celllist-size:%d\n", list->number_of_elements);
    }
}

// creates a new drone
Drone *create_drone(Coord coord, char *info) { 
    int i,info_length=30;

    Drone *newdrone=malloc(sizeof(Drone));
    memset(newdrone,0,sizeof(Drone));

    newdrone->stime=time(0);
    newdrone->coord=coord;
    for(i=0;i<info_length;i++){
        newdrone->info[i]=info[i];
    }
    if(drones==NULL){
        drones=create_list(sizeof(Drone),MAX_DRONE_AMOUNT);
    }
    drones->add(drones,newdrone);
    newdrone->status=STATIONARY;
    
    return newdrone;
    }


/** moves(flies) drone on the map:
based on its speed it jumps cells toward its destination*/
void move_drone(Drone *drone) {
    drone->coord.x+=drone->velocity.x;
    drone->coord.y+=drone->velocity.y;
}

void stop_drone(Drone *drone){
    drone->velocity.x=0;
    drone->velocity.y=0;
    drone->status=STATIONARY;
}

/** a drone delivers aid pack to survivor,
the survivor is marked as helped and removed*/
void help_survivor(Drone *drone, Survivor *survivor) {
    /*TODO: remove survivor from survivorlist, mapcell.survivors*/
    /*TODO: edit help_time, add to the helped_survivors*/
    /*TODO: numberofhelped++,
    drone->status is idle, destination: empty*/
    if(helped_survivors == NULL){
        helped_survivors = create_list(sizeof(Survivor), numberofcells * MAX_SURVIVOR_PER_CELL);    
    }
    if(survivor != NULL){

        // time_t t;
        // struct tm help_time;
        // time(&t); // calender time?
        // localtime_r(&t, &help_time); // to convert local time
        // memcpy(&(survivor->helped_time), &help_time, sizeof(struct tm));
        survivor->status=UNDERHELP;
        usleep(2000);

        survivor->helped_time=time(0);
        survivor->status = HELPED;
        add(helped_survivors,survivor); 
        numberofhelped++;

        removedata(survivors,survivor);  // remove from survivors list
        removedata((map.cells[drone->coord.x][drone->coord.y].survivors),survivor); // remove from cell list

    }
}

// The drone helps the survivors in the cell its in
void help_cell(Drone *drone){
    MapCell cell;
    Survivor *survivor;
    cell=map.cells[drone->coord.x][drone->coord.y];

    drone->status=HELPING;
    for(int i=0; i<cell.survivors->number_of_elements ;i++){
        survivor=(Survivor *)cell.survivors->getnindex(cell.survivors,i+1);
        help_survivor(drone,survivor);
    }

}

// Sets the drone's destination and sets the velocity of the drone accordingly
void set_drone_destination(Drone *drone,Coord destination){
    if(drone == NULL)
        return;
    // time_t traw;
    drone->destination.x=destination.x;
    drone->destination.y=destination.y;

    double speed_multiplier=MAX_DRONE_VELOCITY / sqrt(pow(drone->coord.x - destination.x,2) + pow(drone->coord.y - destination.y,2)); 
    drone->velocity.x= (drone->coord.x - destination.x) * speed_multiplier;
    drone->velocity.y= (drone->coord.y - destination.y) * speed_multiplier;
    drone->status=MOVING;
    drone->stime=time(0);

    List* survivors_in_cell=map.cells[destination.x][destination.y].survivors;
    for(int i=0; i<survivors_in_cell->number_of_elements; i++){
        ((Survivor *)getnindex(survivors_in_cell,i))->status=HELPONWAY;
    }
}

/*THREAD FUNCTION: simulates a drone: */
void *drone_runner(void *vdrone) {
    Drone *drone=vdrone;
    while(!done){
        if(drone->status==STATIONARY){

        }
        else if(drone->status==MOVING){
            if(drone->destination.x==drone->coord.x && drone->destination.y==drone->coord.y){
                stop_drone(drone);
                help_cell(drone);
            }
            move_drone(drone);
        }
        else if(drone->status==HELPING){

        }
        usleep(2000);
    }
    return NULL;
}

/*THREAD FUNCTION: an AI that controls drones based on survivors*/
void *drone_controller() {

    int i,j;
    Drone *drone[MAX_DRONE_AMOUNT],*idealdrone;
    Coord survivor_coord,drone_coord,min_coord;
    Coord coord = {random() % map.height, random() % map.width};

    pthread_t drone_threads[MAX_DRONE_AMOUNT];

    if(drones == NULL){
        drones = create_list(sizeof(Drone), MAX_DRONE_AMOUNT);
    }

    for(i=0; i<MAX_DRONE_AMOUNT; i++){
        drone[i]=create_drone(coord,"Big guy fr");
        add(drones,&drone[i]);
        pthread_create(&drone_threads[i],NULL,drone_runner,(void *)&drone[i]);
    }

    while(!done){
        // survivorlara bak, drone listesinde boş olanları survivorlara yönlendir
        for(i=0;i<survivors->number_of_elements;i++){
            if( ((Survivor *)(survivors->peek(survivors)))->status == NEEDHELP ){
                Survivor *survivor=peek(survivors);
                for(j=0; j<drones->number_of_elements; j++){
                    if(((Drone *)getnindex(drones,j))->status != STATIONARY)
                        continue;

                    drone_coord=((Drone *)getnindex(drones,j))->coord;
                    survivor_coord=survivor->coord;

                    coord.x=(drone_coord.x - survivor_coord.x);
                    coord.y=(drone_coord.y - survivor_coord.y);
                    if(coord.x < 0)
                        coord.x=coord.x*-1;
                    if(coord.y < 0)
                        coord.y=coord.y*-1;

                    if((coord.x*coord.x + coord.y*coord.y) < (min_coord.x*min_coord.x + min_coord.y*min_coord.y) ){
                        min_coord=coord;
                        idealdrone=(Drone *)getnindex(drones,j);
                    }

                }
                set_drone_destination(idealdrone,survivor->coord);


            }
        }


    }

    for (i=0;i<MAX_DRONE_AMOUNT;i++){
        pthread_join(drone_threads[i], NULL);
    }

    for(i=0;i<survivors->number_of_elements;i++){
        free(*(Survivor **)survivors->getnindex(survivors,i));
    }
    for(i=0;i<drones->number_of_elements;i++){
        free(*(Drone **)drones->getnindex(drones,i));
    }
    survivors->destroy(survivors);
    drones->destroy(drones);
    return NULL;
}

/*you should add all the necessary functions
you can add more .c files e.g. survior.c, drone.c
But, for MVC model, controller is a bridge between view and model:
put data update functions only in model, not in your view or controller.
*/