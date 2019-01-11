#ifndef ESCROOM_GAME_H
#define ESCROOM_GAME_H

#include <stdbool.h>
#include <limits.h>
#include <ctype.h>
#include "utils.h"

#define MAX_PLAYERS 1025
#define MAX_ROOMS 1025
#define MAX_TYPES 26
#define NONE -1

typedef struct room_t {
  short max_size;
  short type;
  // Mutable
  short inside_count;
  short waiting_game_size;
  bool game_started;
  bool game_planned;
  short defined_by;
  short players_inside[MAX_PLAYERS];
} Room;

typedef struct player_t {
  short type;
  // Mutable
  short assigned_room;
  short games_played;

} Player;

typedef struct game_t {
  Room rooms[MAX_ROOMS];
  Player players[MAX_PLAYERS];
  short player_count;
  short room_count;
  short finished_players;
  int player_search;
} Game;

typedef struct game_def_t {
  short created_by;
  char room_type;
  short types[MAX_TYPES];
  short ids[MAX_PLAYERS];
} GameDef;

void game_def_init(GameDef *def);

char *game_def_to_string(GameDef *def, char *buff);

GameDef *game_def_read_next(GameDef *def, FILE *f, short player_id, char *raw);

Game *game_init(short players, short rooms);

void game_destroy(Game *g);

void game_init_room(Game *g, short room, short type, short size);

void game_init_player(Game *g, short p, short t);

bool game_player_finished(Game *g);

void game_define_new_game(Game *g, short room, short owner, short *players);

bool game_add_player_to_waiting_list(Game *g, short room, short player);

bool game_is_ever_playable(Game *g, GameDef *def, int player_id);

int game_start_if_possible(Game *g, GameDef *def);

bool game_player_leave_room(Game *g, int player);

#endif // ESCROOM_GAME_H
