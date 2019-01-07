
#ifndef ESCROOM_GAME_H
#define ESCROOM_GAME_H

#include <stdbool.h>
#include "utils.h"

#define MAX_PLAYERS 1025
#define MAX_ROOMS 1025
#define MSG_BUFFER_SIZE 10000
#define NONE -1

typedef struct room_t {
    // 
    int max_size;
    int type;
    // Mutable
    int inside_count;
    int waiting_game_size;
    bool game_started;
    int defined_by;
    int players_inside[MAX_PLAYERS];
} Room;


typedef struct player_t {
    // 
    int id;
    int type;
    // Mutable
    int in_room;

} Player;


typedef struct game_t {
    Room rooms[MAX_ROOMS];
    Player players[MAX_PLAYERS];
    int player_count;
    int room_count;
} Game;


void player_set_free(Player *p) {
    p->in_room = 0;
}

void room_set_empty(Room *r) {
    r->inside_count = 0;
    r->waiting_game_size = 0;
    r->game_started = false;
    r->defined_by = NONE;
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        r->players_inside[i] = NONE;
    }
}

void game_init(Game *g, int players, int rooms) {
    g->player_count = players;
    g->room_count = rooms;
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        g->players[i].type = NONE;
        g->players[i].id = NONE;
        player_set_free(&(g->players[i]));
    }
    for (int i = 0; i < MAX_ROOMS; ++i) {
        g->rooms[i].type = NONE;
        g->rooms[i].max_size = NONE;
        room_set_empty(&(g->rooms[i]));
    }
}


void game_init_room(Game *g, int room, int type, int size) {
    assertion(room < g->room_count);
    assertion(type <= 'Z' && type >= 'A');
    g->rooms[room].type = type;
    g->rooms[room].max_size = size;
    log_debug("Initialized room r=%d t=%c s=%d", room, (char) type, size);
}

void game_init_player(Game *g, int p, int t) {
    assertion(p < g->player_count && 'A' <= t && 'Z' >= t);
    g->players[p].type = t;
}

void game_define_new_game(Game *g, int room, int owner, int *players) {
    assertion((room < g->room_count) && "Index out of range");
    Room *r = &(g->rooms[room]);
    assertion((!r->game_started) && "The requested room is busy");
    assertion((owner < g->player_count) && "Owner out of bounds");
    r->inside_count = 0;
    r->waiting_game_size = 0;
    r->game_started = false;
    r->defined_by = owner;
    for (int i = 0; players[i] != NONE; ++i) {
        assertion((players[i] < g->player_count) && "Player out of range")
        Player *p = &(g->players[players[i]]);
        assertion((p->in_room == NONE) && "Player is not free");
        r->players_inside[i] = players[i];
        r->waiting_game_size++;
    }
    log_debug("Defined a game in room owned by r=%d own=%d", room, owner);
}

bool game_add_player_to_waiting_list(Game *g, int room, int player) {
    assertion((room < g->room_count && player < g->player_count) && "Indexes out of range");
    assertion((!g->rooms[room].game_started) && "Game is already started, cannot join the room");
    assertion((g->rooms[room].inside_count + 1 <= g->rooms[room].max_size) && "Room is full, cannot join");
    assertion((g->players[player].in_room == NONE) && "The requested player is already busy playing");

    Room *r = &(g->rooms[room]);
    int player_index = NONE;
    for (int i = 0; i < MAX_PLAYERS && r->players_inside[i] != NONE; ++i) {
        if (r->players_inside[i] == player) {
            player_index = i;
        }
    }

    assertion((player_index != NONE) && "The game hosted in this room is not expecting the player to join");
    assertion((player_index >= r->inside_count) && "The player is already waiting for the game to start");

    int tmp = r->players_inside[r->inside_count];
    r->players_inside[r->inside_count] = r->players_inside[player_index];
    r->players_inside[player_index] = tmp;

    r->inside_count++;
    g->players[player].in_room = room;


    log_debug("Added player to waiting list of room. p=%d r=%d", player, room);
    if (r->inside_count == r->waiting_game_size) {
        r->game_started = true;
        log_debug("Game is started in room %d", room);
    }
    return r->game_started;
}


bool game_player_leave_room(Game *g, int player) {
    assertion((player < g->player_count) && "Player out of bounds");
    Player *p = &(g->players[player]);
    assertion((p->in_room != NONE) && "Player is free, cannot leave a room");
    Room *r = &(g->rooms[p->in_room]);
    r->inside_count--;
    log_debug("Player leaved room %d %d", player, p->in_room);
    player_set_free(p);
    if (r->inside_count == 0) {
        room_set_empty(r);
        log_debug("Game is finished in room %d", p->in_room);
    }
    return !r->game_started;
}


int bool_choose_room(Game *g, int p_ids[], char types[]) {

}



#endif // ESCROOM_GAME_H
