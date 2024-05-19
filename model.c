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
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "SDL2/SDL.h"
#define MAX_SURVIVOR_PER_CELL 3
#define MAX_DRONE_AMOUNT 1
#define MAX_DRONE_VELOCITY 0.5

sem_t semaphore;
// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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

        Coord coord = {random() % map.width, random() % map.height};

        time(&traw);
        localtime_r(&traw, &t);

        // printf("creating survivor...%s\n", asctime(&t));
        Survivor *s = create_survivor(&coord, info, &traw);

        // pthread_mutex_lock(&lock);
        /*add to general list*/
        add(survivors, s);

        /*add to the list in the cell*/
        List *list = map.cells[coord.y][coord.x].survivors;
        add(list, s);
        // pthread_mutex_unlock(&lock);

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
   /* if(drones==NULL){
        drones=create_list(sizeof(Drone),MAX_DRONE_AMOUNT);
    }
    drones->add(drones,newdrone);*/
    newdrone->status=STATIONARY;
    
    return newdrone;
    }


/** moves(flies) drone on the map:
based on its speed it jumps a minimum of 1 cell toward its destination*/
void move_drone(Drone *drone) {
    // drone->coord.x+=drone->velocity.x;
    // drone->coord.y+=drone->velocity.y;
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

void stop_drone(Drone *drone){
    // drone->velocity.x=0;
    // drone->velocity.y=0;
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
        survivor->status = UNDERHELP;
        SDL_Delay(100);

        survivor->helped_time=time(0);
        survivor->status = HELPED;
        add(helped_survivors,survivor); 
        numberofhelped++;
        // pthread_mutex_lock(&lock);
        if(removedata(survivors,survivor)){
            printf("there was an error removing survivior fron surviviors\n");
        }  // remove from survivors list
        if(removedata((map.cells[drone->coord.y][drone->coord.x]).survivors,survivor)){
            printf("there was an error removing survivior from cell\n");
        } // remove from cell list
        // pthread_mutex_unlock(&lock);

    }
}

// The drone helps the survivors in the cell its in
void help_cell(Drone *drone){
    MapCell cell;
    Survivor *survivor;
    cell=map.cells[drone->coord.y][drone->coord.x];

    drone->status=HELPING;
    for(int i=0; i<cell.survivors->number_of_elements ;i++){
        survivor=(Survivor *)getnindex(cell.survivors,i);
        help_survivor(drone,survivor);
    }
    drone->status=STATIONARY;

}

// Sets the drone's destination and sets the velocity of the drone accordingly
void set_drone_destination(Drone *drone,Coord destination){
    if(drone == NULL)
        return;
    // time_t traw;
    drone->destination.x=destination.x;
    drone->destination.y=destination.y;

    // double speed_multiplier=MAX_DRONE_VELOCITY / sqrt(pow(drone->coord.x - destination.x,2) + pow(drone->coord.y - destination.y,2)); 
    // drone->velocity.x= (drone->coord.x - destination.x) * speed_multiplier;
    // drone->velocity.y= (drone->coord.y - destination.y) * speed_multiplier;
    drone->status=MOVING;
    drone->stime=time(0);

    List* survivors_in_cell=map.cells[destination.y][destination.x].survivors;
    for(int i=0; i<survivors_in_cell->number_of_elements; i++){
        ((Survivor *)getnindex(survivors_in_cell,i))->status=HELPONWAY;
    }
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
void *drone_controller() {

    // pthread_mutex_init(&lock,NULL);

    int i,j;
    Drone *drone[MAX_DRONE_AMOUNT],*idealdrone;
    Coord survivor_coord,drone_coord,min_coord;
    min_coord.x=map.width;
    min_coord.y=map.height;
    Coord coord = {random() % map.width, random() % map.height};

    pthread_t drone_threads[MAX_DRONE_AMOUNT];

    if(drones == NULL){
        drones = create_list(sizeof(Drone), MAX_DRONE_AMOUNT);
    }
    void *addres;

    for(i=0; i<MAX_DRONE_AMOUNT; i++){
        drone[i]=create_drone(coord,"Big guy fr");
        add(drones,drone[i]);
        printf("%d",&drone[i]);
        pthread_create(&drone_threads[i],NULL,drone_runner,(void *)((Drone *)getnindex(drones,0)));
    }

    while(!done){
        // survivorlara bak, drone listesinde boş olanları survivorlara yönlendir
        for(i=0;i<survivors->number_of_elements;i++){
            if( ((Survivor *)(survivors->getnindex(survivors,i)))->status == NEEDHELP ){
                idealdrone=NULL;
                Survivor *survivor=getnindex(survivors,i);
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
            SDL_Delay(500);
        }


    }

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

/*you should add all the necessary functions
you can add more .c files e.g. survior.c, drone.c
But, for MVC model, controller is a bridge between view and model:
put data update functions only in model, not in your view or controller.
*/
