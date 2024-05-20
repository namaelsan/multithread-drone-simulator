/*
 * controller.c
 *      Author: adaskin
 */

#include "SDL2/SDL.h"
#include "simulator.h"
#include <pthread.h>

/*do not change any of this*/
extern SDL_bool done;
extern pthread_mutex_t lock;

int main(int argc, char* argv[]) {
    time_t start_time = time(0);
    time_t total_time;
    printf("srand seed:%d",start_time);
    /*initializes map*/
    init_map(40, 30);

    /*initializes window*/
    printf("initialize window\n");
    init_sdl_window(map);

    printf("draw map\n");
    /*draws initial map*/
    draw_map();

    /*initialize the mutex*/
    pthread_mutex_init(&lock,NULL);
    srand(start_time);

    /*initializes the drones and starts the ai controlling them*/
    pthread_t controller_thread;
    pthread_create(&controller_thread,NULL,drone_controller,NULL);

    /* update model:survivors, drones etc. */
    pthread_t survivor_thread;
    pthread_create(&survivor_thread,NULL,survivor_generator,NULL);

    /* repeat until window is closed */
    while (!done) {

        /*check user events: e.g. click to close x*/
        check_events();

        /*draws new updated map*/
        draw_map();
   
        SDL_Delay(300); /*sleep(1);*/
  
    }
    
    printf("quitting...\n");
    total_time = time(0) - start_time;
    printf("program finished in %ld minutes %ld seconds\n",(total_time/60),(total_time%60));

    /*quit everything*/
    freemap();
    pthread_join(controller_thread,NULL);
    pthread_join(survivor_thread,NULL);
    pthread_mutex_destroy(&lock);
    quit_all();
    return 0;
}