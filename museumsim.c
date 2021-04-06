#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "museumsim.h"

//
// In all of the definitions below, some code has been provided as an example
// to get you started, but you do not have to use it. You may change anything
// in this file except the function signatures.
//
/**
 * Set up the shared variables for your implementation.
 * 
 * `museum_init` will be called before any threads of the simulation
 * are spawned.
 */
struct shared_data {
    pthread_mutex_t t_mutex; 
    pthread_mutex_t v_mutex;
    pthread_mutex_t g_mutex; 

    pthread_cond_t g_is_working;
    pthread_cond_t g_finishes;
    pthread_cond_t visitor_entered_tb;

    pthread_cond_t ive_been_admitted;
    pthread_cond_t ive_been_kicked_out;

    pthread_cond_t next_come;
    pthread_cond_t next_leave;
    
    int tickets;
    int num_full;

    int visitors_inside;
    int visitors_outside;
    int visitors_kicked_out;

    int guides_outside;
    int guides_inside;

    int tickets_per_guide;
};

static struct shared_data museum;

void museum_init(int num_guides, int num_visitors)
{
    pthread_mutex_init(&museum.t_mutex, NULL);
    pthread_mutex_init(&museum.v_mutex, NULL);
    pthread_mutex_init(&museum.g_mutex, NULL);

    pthread_cond_init(&museum.g_is_working, NULL);
    pthread_cond_init(&museum.g_finishes, NULL);
    
    pthread_cond_init(&museum.visitor_entered_tb, NULL);
    pthread_cond_init(&museum.ive_been_kicked_out, NULL);
    pthread_cond_init(&museum.ive_been_admitted, NULL);
    pthread_cond_init(&museum.next_leave, NULL);
    pthread_cond_init(&museum.next_come, NULL);


    // protected by t_mutex
    museum.tickets = MIN(VISITORS_PER_GUIDE * num_guides, num_visitors);
    museum.num_full = 0;

    // protected by v_mutex
    museum.visitors_outside= 0;
    museum.visitors_inside  =0;
    museum.visitors_kicked_out=0;
    
    // protected by g_mutex
    museum.tickets_per_guide = MIN(VISITORS_PER_GUIDE * num_guides, num_visitors);
    museum.guides_inside   = 0;
    museum.guides_outside  = 0;
}

/**
 * Tear down the shared variables for your implementation.
 * 
 * `museum_destroy` will be called after all threads of the simulation
 * are done executing.
 */
void museum_destroy()
{
    pthread_mutex_destroy(&museum.t_mutex);
    pthread_mutex_destroy(&museum.v_mutex);
    pthread_mutex_destroy(&museum.g_mutex);
    
    pthread_cond_destroy(&museum.g_is_working);
    pthread_cond_destroy(&museum.g_finishes);
    pthread_cond_destroy(&museum.visitor_entered_tb);
    pthread_cond_destroy(&museum.ive_been_admitted);
    pthread_cond_destroy(&museum.ive_been_kicked_out);
    pthread_cond_destroy(&museum.next_leave);
    pthread_cond_destroy(&museum.next_come);
}

/**
 * Visitor enters the ticket booth and buys a ticket if available.
 * Return 0 if successful purchase
 * Return 1 if soldout
 */
int enter_tb(int id){
    pthread_mutex_lock(&museum.t_mutex);
    {
        if (museum.tickets == 0) {                     //  tickets soldout
            visitor_leaves(id);
            pthread_cond_broadcast(&museum.g_finishes);
            pthread_mutex_unlock(&museum.t_mutex);
            return 1;
        } else {                                       //  ticket purchased
            museum.tickets -= 1;
            museum.num_full += 1;
            pthread_cond_broadcast(&museum.visitor_entered_tb);
        }
    }
    pthread_mutex_unlock(&museum.t_mutex);
    return 0;
}

/**
 * Guide removes a visitor from the ticket booth
 */
void grab_from_tb(){
    pthread_mutex_lock(&museum.t_mutex);
    {
        while(museum.num_full == 0 && museum.tickets != 0){
            pthread_cond_wait(&museum.visitor_entered_tb, &museum.t_mutex);
        }
        museum.num_full -= 1;
    }
    pthread_mutex_unlock(&museum.t_mutex);
}

/**
 * Manages the number of visitors each guide should expect
 */
int allot_tickets(){
    int n_tickets = -1;
    pthread_mutex_lock(&museum.g_mutex);
    {
        n_tickets = MIN(museum.tickets_per_guide, VISITORS_PER_GUIDE);
        museum.tickets_per_guide -= VISITORS_PER_GUIDE;
    }
    pthread_mutex_unlock(&museum.g_mutex);
    return n_tickets;
}

/**
 * Guide begins working + waits for one to leave if necessary
 */
void start_work(int id){
    pthread_mutex_lock(&museum.g_mutex);
    {
        guide_arrives(id);
        museum.guides_outside += 1;
        while(museum.guides_inside == GUIDES_ALLOWED_INSIDE){
            pthread_cond_wait(&museum.g_finishes, &museum.g_mutex);
        }
        guide_enters(id);
        museum.guides_outside-= 1;
        museum.guides_inside += 1;
        pthread_cond_broadcast(&museum.g_is_working);
    }
    pthread_mutex_unlock(&museum.g_mutex);
}

/**
 *  Visitor waits for a guide to arrive
 */
void wait_for_guide(int id){
    pthread_mutex_lock(&museum.g_mutex);
    while (museum.guides_inside == 0){
        pthread_cond_wait(&museum.g_is_working, &museum.g_mutex);
    }
    pthread_mutex_unlock(&museum.g_mutex);
}

/**
 *  Guide leaves work and notifies the waiting ones.
 */
void leave_work(int id){
    pthread_mutex_lock(&museum.g_mutex);
    {
        museum.guides_inside -= 1;
        guide_leaves(id);
        pthread_cond_broadcast(&museum.g_finishes);
    }
    pthread_mutex_unlock(&museum.g_mutex);
}

/**
 *  Visitor waits it turn, then begins touring the museum.
 */
void start_tour(int id){
    pthread_mutex_lock(&museum.v_mutex);
	{  
        while(museum.visitors_outside == 0){
            pthread_cond_wait(&museum.next_come, &museum.v_mutex);
        }
        museum.visitors_outside = 0;
        pthread_cond_signal(&museum.ive_been_admitted);
    }
    pthread_mutex_unlock(&museum.v_mutex);
    visitor_tours(id);
}

/**
 *  Guide admits a single visitor into the museum.
 */
void bring_in_visitor(int id){
    pthread_mutex_lock(&museum.v_mutex);
	{  
        while(museum.visitors_outside != 0){
            pthread_cond_wait(&museum.ive_been_admitted, &museum.v_mutex);
        }
        museum.visitors_outside = 1;
        museum.visitors_inside += 1;
        guide_admits(id);
        pthread_cond_signal(&museum.next_come);
    }
    pthread_mutex_unlock(&museum.v_mutex);
}


/**
 *  Guide removes a single visitor from the museum after waiting until the previous one left.
 */
void release_visitor(){
    pthread_mutex_lock(&museum.v_mutex);
    {
        while(museum.visitors_kicked_out != 0){
            pthread_cond_wait(&museum.ive_been_kicked_out, &museum.v_mutex);
        }
        museum.visitors_kicked_out = 1;
        museum.visitors_inside -= 1;
        pthread_cond_signal(&museum.next_leave);
    }
    pthread_mutex_unlock(&museum.v_mutex);
}

/**
 *  Visitor leaves the museum and notifies the next one to go.
 */
void leave_tour(int id)
{   
	pthread_mutex_lock(&museum.v_mutex);
	{  
        if (museum.visitors_inside == VISITORS_PER_GUIDE*GUIDES_ALLOWED_INSIDE){
	        museum.visitors_kicked_out = 0;
            visitor_leaves(id);
            pthread_cond_signal(&museum.ive_been_kicked_out);
        }
        while (museum.visitors_kicked_out == 0){
            pthread_cond_wait(&museum.next_leave, &museum.v_mutex);
        }
        museum.visitors_kicked_out = 0;
        visitor_leaves(id);
        pthread_cond_signal(&museum.ive_been_kicked_out);
	}
	pthread_mutex_unlock(&museum.v_mutex);
}


/**
 * Implements the visitor arrival, touring, and leaving sequence.
 */
void visitor(int id)
{
    visitor_arrives(id);
    int is_soldout = enter_tb(id);  // enter ticket booth and aquire a ticket
    if (is_soldout == 1){ return; }
    wait_for_guide(id);     
    start_tour(id);
    leave_tour(id);  
}

/**
 * Implements the guide arrival, entering, admitting, and leaving sequence.
 */
void guide(int id)
{
    start_work(id);
    int n_visitors = 0;
    int n_tickets = allot_tickets();
    while (n_visitors != n_tickets){
		grab_from_tb();
        n_visitors += 1;
        bring_in_visitor(id);
    }
    while (n_visitors >= 0){    // tell all visitors associaed with guide to leave
      release_visitor();
      n_visitors -= 1;
    }
    leave_work(id);
}