/**
 * @file model.c
 * @author Enhar Apuhan & Mervenur Saraç
 * @brief
 * @version 0.1
 * @date 2024-04-21
 *
 * @copyright Copyright (c) 2024
 *This program simulates a drone-based rescue operation where drones are deployed
 * to help survivors in a mapped are.The simulation uses multithreading to generate 
 * survivors and control drones in real-time, ensuring synchronized access to shared
 * resources with mutex locks.
 * 

 */
#include "simulator.h"
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "SDL2/SDL.h"
#define MAX_SURVIVOR_PER_CELL 3
#define MAX_DRONE_AMOUNT 10
#define MAX_DRONE_VELOCITY 1
#define MAX_RESCUED_SURVIVOR 200
#define SURVIVOR_PER_SECOND 8


pthread_mutex_t lock;

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
    survivors = create_list(sizeof(Survivor *), numberofcells * MAX_SURVIVOR_PER_CELL);

    /*pointer array*/
    map.cells = malloc(sizeof(MapCell *) * map.height);

    for (int j = 0; j < map.height; j++) {
        /*rows for each pointer*/
        map.cells[j] = malloc(sizeof(MapCell) * map.width);
    }

    for (int i = 0; i < map.height; i++) {
        for (int j = 0; j < map.width; j++) {
            map.cells[i][j].coord.y = i;
            map.cells[i][j].coord.x = j; /**/
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
/* creates a survivor and edits its attribute
*/
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
void *survivor_generator(void *args) {
    
    while(!done){
        // generate random location
        if (map.cells != NULL) {
            time_t traw;
            struct tm t; /*used for localtime*/

            /*survivor info*/
            char info[5] = {'A' + (random() % 26), 
                            'A' + (random() % 26),
                            '0' + (random() % 9),
                            '0' + (random() % 9)}; 

            Coord coord = {random() % map.width, random() % map.height};

            time(&traw);
            localtime_r(&traw, &t);

            // printf("creating survivor...%s\n", asctime(&t));
            Survivor *s = create_survivor(&coord, info, &traw);

            pthread_mutex_lock(&lock);
            /*add to general list*/
            
            add(survivors, &s);

            /*add to the list in the cell*/
            List *list = map.cells[coord.y][coord.x].survivors;
            add(list, &s);
            pthread_mutex_unlock(&lock);

            printf("survivor added, celllist-size:%d\n", list->number_of_elements);
        }
        SDL_Delay(1000/SURVIVOR_PER_SECOND); /*waits for creating a new survivor */
    }
    return NULL;
}

/* creates a new drone and edits its attributes
*/
Drone *create_drone(Coord coord, char *info) { 
    int i,info_length=30;

    Drone *newdrone=malloc(sizeof(Drone));
    memset(newdrone,0,sizeof(Drone));

    newdrone->stime=time(0);
    newdrone->coord=coord;
    for(i=0;i<info_length;i++){
        newdrone->info[i]=info[i];
    }

    newdrone->status=STATIONARY;
    
    return newdrone;
    }


/** moves(flies) drone on the map:
based on its speed it jumps a minimum of 1 cell toward its destination*/
void move_drone(Drone *drone) {

    double xdiff=(drone->destination.x - drone->coord.x);
    double ydiff=(drone->destination.y - drone->coord.y);
    double distance=sqrt((xdiff*xdiff) + (ydiff*ydiff));
    double xadd,yadd;
    if(distance <= MAX_DRONE_VELOCITY){
        drone->coord=drone->destination;
    }
    else{
        xadd=xdiff*(MAX_DRONE_VELOCITY/distance);
        yadd=ydiff*(MAX_DRONE_VELOCITY/distance);
        if(xadd >0)
            xadd=ceil(xadd);
        if(yadd >0)
            yadd=ceil(yadd);
        drone->coord.x+=(xadd);
        drone->coord.y+=(yadd);
    }
    printf("Drone Moved x:%d y:%d\n",drone->coord.x,drone->coord.y);
}

/*when drone come to the destination this function stops  it*/
void stop_drone(Drone *drone){

    drone->status=STATIONARY;

}

/*calculates survivors rescue time and prints it*/
void print_rescue_time(Survivor *survivor,Drone *drone){

    survivor->helped_time= time(0) - survivor->discovery_time;
    printf("Survivor %s was rescued in %ld minutes and %ld seconds.\n",survivor->info,(survivor->helped_time/60),survivor->helped_time%60);

}

/** a drone delivers aid pack to survivor,
the survivor is marked as helped and removed*/
void help_survivor(Drone *drone, Survivor *survivor) {

    if(helped_survivors == NULL){
        helped_survivors = create_list(sizeof(Survivor), numberofcells * MAX_SURVIVOR_PER_CELL);    
    }
    if(survivor != NULL){

        survivor->status = UNDERHELP;
        SDL_Delay(100);

        print_rescue_time(survivor,drone);

        survivor->helped_time=time(0);
        survivor->status = HELPED;
        pthread_mutex_lock(&lock);
        numberofhelped++;
        add(helped_survivors, &survivor);
        if(numberofhelped == MAX_RESCUED_SURVIVOR){
            printf("survivors per second:%d\n",SURVIVOR_PER_SECOND);
            done = SDL_TRUE;
        }
        pthread_mutex_unlock(&lock);

        }

    }


// The drone helps the survivors in the cell its in
void help_cell(Drone *drone){
    MapCell cell;
    Survivor *survivor;
    cell=map.cells[drone->coord.y][drone->coord.x];

    drone->status=HELPING;
    for(int i=0; i<cell.survivors->number_of_elements ;i++){
        survivor=*(Survivor **)getnindex(cell.survivors,i);
        if(survivor->status == HELPONWAY){

        help_survivor(drone,survivor);

        pthread_mutex_lock(&lock);// access to list is controlled with mutex 
        if(removedata(survivors, getnindex(cell.survivors,i))){
            printf("there was an error removing survivior from survivors\n");
        }  // remove from survivors list
        if(removedata((map.cells[drone->coord.y][drone->coord.x]).survivors, getnindex(cell.survivors,i))){
            printf("there was an error removing survivior from cell\n");
        } // remove from cell list
        pthread_mutex_unlock(&lock);

        break; // her sseferinde HELPONWAY olan yalnız 1 survivora yardım edilir
        }
    }
    drone->status=STATIONARY;

}

// Sets the drone's destination and sets the velocity of the drone accordingly
void set_drone_destination(Drone *drone,Coord destination){
    if(drone == NULL)
        return;

    drone->destination.x=destination.x;
    drone->destination.y=destination.y;

    drone->status=MOVING;
    drone->stime=time(0);

    List* survivors_in_cell=map.cells[destination.y][destination.x].survivors;
    
    if(survivors_in_cell->number_of_elements > 0) 
        (*(Survivor **)getnindex(survivors_in_cell, 0))->status = HELPONWAY;
               
    printf("New Destination x:%d y:%d\n",destination.x,destination.y);
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
            else {
                move_drone(drone);
            }
        }
        else if(drone->status==HELPING){

        }
        SDL_Delay(1000);
    }
    return NULL;
}

/*THREAD FUNCTION: an AI that controls drones based on survivors*/
/*
creates drones and drone threads,finds the nearest survivor for all drones and sends help
*/
void *drone_controller() {
    int i,j;
    Drone *drone[MAX_DRONE_AMOUNT];
    Coord survivor_coord,drone_coord,min_coord;
    Survivor *idealsurvivor,*survivor;
    min_coord.x=map.width;
    min_coord.y=map.height;
    Coord coord;
    coord.x = random() % map.width;
    coord.y = random() % map.height;
    pthread_t drone_threads[MAX_DRONE_AMOUNT];

    if(drones == NULL)
        drones = create_list(sizeof(Drone), MAX_DRONE_AMOUNT);
    
    for(i=0; i<MAX_DRONE_AMOUNT; i++){
        drone[i]=create_drone(coord,"Big guy fr");
        add(drones, &drone[i]);
        pthread_create(&drone_threads[i],NULL,drone_runner,(void *)(*(Drone **)getnindex(drones,0)));
    }
 /*search entire survivor list to find neasrest survivor for current drone*/
   while(!done){ 
        for(i = 0; i < (drones->number_of_elements); i++){
            if((*(Drone **)getnindex(drones,i))->status == STATIONARY){   
                
                idealsurvivor = NULL;
                Drone *current_drone= (*(Drone **)getnindex(drones,i));
                pthread_mutex_lock(&lock);
                min_coord.x=map.width;
                min_coord.y=map.height;

                for(j = 0; j < (survivors->number_of_elements); j++){
                    survivor=*(Survivor **)getnindex(survivors,j);
                    if(( survivor->status == NEEDHELP )){

                        drone_coord=current_drone->coord;
                        survivor_coord=survivor->coord; 
                     
                        coord.x = (drone_coord.x - survivor_coord.x);
                        coord.y = (drone_coord.y - survivor_coord.y);
                        if(coord.x < 0)
                            coord.x = coord.x*-1;
                        if(coord.y < 0)
                            coord.y = coord.y*-1;
                        
                        if((coord.x*coord.x + coord.y*coord.y) < (min_coord.x*min_coord.x + min_coord.y*min_coord.y) ){
                            min_coord = coord;
                            idealsurvivor = survivor;
                        }
                    }
                
                } 
                pthread_mutex_unlock(&lock);

                if(idealsurvivor == NULL)
                    continue;
                /*sends drone to the ideal(nearest) survivor*/
                set_drone_destination(current_drone,idealsurvivor->coord);


            } 

        SDL_Delay(500);
        }

    }

    pthread_mutex_lock(&lock);
    printf("%d survivors helped\n",numberofhelped);
    pthread_mutex_unlock(&lock);

    for (i=0;i<MAX_DRONE_AMOUNT-1;i++){
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