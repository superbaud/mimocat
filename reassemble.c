/* mimocat -- netcat that distributes stdin/stdout over multiple TCP streams

   Copyright (c) 2013 Susan Werner <heinousbutch@gmail.com>
   
   reassemble.c -- handle reordering of cells and data stream reassembly 
 */

#include "mimocat.h"



/* there are two different steps in taking data from more than one network 
   stream, reassembling cells, and reordering them into one stdout stream.

   1) for each input stream, we convert non-delimited data into cells, and 
   unpack those cells, and add them to the linked list.

   2) look at the linked list and reassemble cells (in order!) into one output 
   data stream.


   step one needs to have one instance of state per each network stream, while 
   step two works on one linked list (for all the incoming network streams).

 */


#define INITIALBUFFER (1024) /* initial length of the reassembly buffer. 
                                this is totally arbitrary */

/* this is the state for step one. there is one instance of this structure for
   each input stream */

/* this structure is used to manage a buffer called "incomplete". We maintain
   two pointers within this buffer -- one called writepos, one called readpos.

   writepos points to the byte AFTER the last byte of the last chunk of data 
   we wrote to the buffer. writepos is only to be modified by code that adds
   data to the buffer. adding x bytes to the buffer means writepos will be 
   incremented by x.

   readpos points to the initial byte of valid cell data. readpos 
   is only to be modified by code that takes valid cells out of the buffer.
   */


typedef struct reassembly_state {
    uint8_t *writepos;             
    uint8_t *readpos;              
    uint8_t *incomplete;            
    size_t incomplete_len;          
} reassembly_state_t;



/* this is the state for step 2. there is one instance of this structure per 
   output stream (so just 1 instance assuming we're just writing to stdout) */

typedef struct reordering_state{
    unpacked_cell_t *head;            /* the head of the linked list where we 
                                         store not-yet-processed cells */
    uint32_t last;                    /* the sequence number of the last cell 
                                         we processed (aka, the sequence number
                                         of the last cell that got removed from
                                         the linked list */
} reordering_state_t;





reassembly_state_t * initialize_reass()
{

    reassembly_state_t * state=malloc(sizeof(reassembly_state_t));
    assert(state != NULL);
    state->incomplete = malloc(sizeof(uint8_t) * INITIALBUFFER);
    assert(state->incomplete != NULL);
    state->writepos = state->incomplete;
    state->readpos = state->incomplete;
    state->incomplete_len = INITIALBUFFER;
    return state;
}

/* takes a blob of data (with no specific requirements on it) and adds it to
   the end of the incomplete buffer*/
void push_data(uint8_t *data, size_t len, reassembly_state_t *state)
{
    size_t bytes_left; /* how many bytes are available inside the buffer */
    bytes_left = state->incomplete_len - (state->writepos - state->incomplete);
    
    if(bytes_left < len)
    { /* ok, we need to grow our buffer to accomodate the incoming data */
       
       size_t grow  = len; /* we can grow it by how much we want, but it needs
                               to be at least big enough to hold the new data */
       size_t offset_w = state->writepos - state->incomplete;
       size_t offset_r = state->readpos - state->incomplete;
          

       uint8_t *newbuf = realloc(state->incomplete, state->incomplete_len + grow );
       assert (newbuf!= NULL);

       state->incomplete = newbuf; /* ok, realloc worked */

       state->incomplete_len += grow; /* keep track of the current buffer size */

       state->writepos = state->incomplete + offset_w; 
       state->readpos = state->incomplete + offset_r; 
    }
    
    memcpy(state->writepos, data, len);
    state->writepos += len;

}


int main() {
    uint8_t data [] = {0x41,0x41,0x41,0x41,0x41,0x41,0x42};
    reassembly_state_t * state = initialize_reass();
    push_data(data, 7, state); 
    push_data(data, 7, state); 
    free(state->incomplete);
    free(state);
    return 0;

    
}

