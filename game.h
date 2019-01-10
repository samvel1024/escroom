
#ifndef ESCROOM_GAME_H
#define ESCROOM_GAME_H

#include <stdbool.h>
#include <limits.h>
#include <ctype.h>
#include "utils.h"
#include "messaging.h"

#define MAX_PLAYERS 1025
#define MAX_ROOMS 1025
#define MAX_TYPES 26
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

typedef struct game_def_t {
  char room_type;
  int types[MAX_TYPES];
  int ids[MAX_PLAYERS];
} GameDef;

void game_def_init(GameDef *def) {
  def->room_type = NONE;
  for (int i = 0; i < MAX_TYPES; ++i)
    def->types[i] = 0;
  def->ids[0] = NONE;
}

char *game_def_to_string(GameDef *def, char *buff) {
  buff[0] = '\0';
  sprintf(buff + strlen(buff), "room_type=%c ", def->room_type);
  for (int i = 0; i < MAX_TYPES; ++i) {
    if (def->types[i] > 0) {
      sprintf(buff + strlen(buff), "type_%c=%d ", 'A' + i, def->types[i]);
    }
  }
  for (int i = 0; def->ids[i] != NONE; ++i) {
    sprintf(buff + strlen(buff), "id_%d=%d ", i, def->ids[i]);
  }
  return buff;
}

GameDef *game_def_read_next(GameDef *def, FILE *f) {
  if (feof(f))
    return NULL;
  char buff[MSG_BUFF_LEN];
  char *line = fgets(buff, MSG_BUFF_LEN, f);
  assertion(ferror(f) == 0 && "There was error reading the file")
  if (line == NULL)
    return NULL;
  int ids = 0;
  char *token = strtok(line, " \n");
  game_def_init(def);
  while (token != NULL) {
    if (token == line) { // room type (has to be the first)
      assertion(strlen(token) == 1 && "Type of the room cannot be longer than 1 char");
      def->room_type = *token;
    } else if (isalpha(*token)) { // type
      def->types[*token - 'A']++;
    } else { // id
      int id = atoi(token) - 1;
      def->ids[ids++] = id;
    }
    token = strtok(NULL, " ");
  }
  def->ids[ids] = NONE;
  return def;
}

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

Game *game_shared_init(int players, int rooms) {
  Game *ptr = shared_mem_get("esc_game", sizeof(Game), true);
  game_init(ptr, players, rooms);
  return ptr;
}

Game *game_bind_shared() {
  return shared_mem_get("esc_game", sizeof(Game), false);
}

void game_close_shared(Game *g) {
  shared_mem_close("esc_room", g, sizeof(Game));
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
  log_debug("Defined a game in room owned by r=%d own=%d players=%s", room, owner);
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

int game_find_room(Game *g, int type) {
  assertion(type >= 'A' && type <= 'Z');
  for (int i = 0; i < g->room_count; ++i) {
    if (g->rooms[i].type == type)
      return i;
  }
  return NONE;
}

bool game_is_ever_playable(Game *g, GameDef *def, int player_id) {
  assertion(player_id < g->player_count && player_id >= 0);
  if (game_find_room(g, def->room_type) == NONE)
    return false;

  bool busy[MAX_PLAYERS];
  for (int i = 0; i < MAX_PLAYERS; ++i)
    busy[i] = false;
  for (int i = 0; i < MAX_PLAYERS && def->ids[i] != NONE; ++i) {
    if (def->ids[i] < 0 || def->ids[i] >= g->player_count) // No player with that id
      return false;
    busy[def->ids[i]] = true;
  }

  int wanted_types[MAX_TYPES];
  memcpy(wanted_types, def->types, sizeof(wanted_types));
  for (int i = 0; i < g->player_count; ++i) {
    if (!busy[i])
      wanted_types[g->players[i].type - 'A']--;
  }
  for (int i = 0; i < MAX_TYPES; ++i) {
    if (wanted_types[i] > 0)
      return false;
  }
  return true;
}

void append_arr(int *arr, int *len, int val){
  arr[*len] = val;
  arr[*len+1] = NONE;
  (*len)++;
}

int game_start_if_possible(Game *g, GameDef *def, int player_id) {
  // The owner of the definition has to be free
  if (g->players[player_id].in_room != NONE)
    return NONE;

  // iff Selected[i] then player i will take part in the game
  bool selected[MAX_PLAYERS];
  for(int i=0; i<MAX_PLAYERS; ++i)
    selected[i] = false;
  int player_arr[MAX_PLAYERS];
  int len = 0;
  player_arr[0] = NONE;

  // Make sure all the directly specified players are free
  for (int i = 0; def->ids[i] != NONE; ++i) {
    if (g->players[def->ids[i]].in_room != NONE && !selected[def->ids[i]]) {
      log_debug("Cannot play def by player %d, cannot find players by ids", player_id);
      return NONE;
    }
    else {
      selected[i] = true;
      append_arr(player_arr, &len, def->ids[i]);
    }
  }

  // Choose players by their type
  int wanted_types[MAX_TYPES];
  memcpy(wanted_types, def->types, sizeof(wanted_types));
  for(int i=0; i<g->player_count; ++i){ //TODO change i initial value to be more fair
    Player *p = &g->players[i];
    int p_type = p->type = 'A';
    if (wanted_types[p_type] > 0 && p->in_room == NONE && !selected[i]){
      wanted_types[p_type]--;
      append_arr(player_arr, &len, i);
    }
  }
  for(int i=0; i<MAX_TYPES; ++i){
    if (wanted_types[i] > 0){
      log_debug("Cannot play def by player %d, cannot find players by types", player_id);
      return NONE;
    }
  }

  //Choose the room
  int r_id = NONE;
  int r_cap = INT_MAX;
  for(int i=0; i<g->room_count; ++i){
    Room *r = &g->rooms[i];
    if (r->type == def->room_type && r->max_size >= len && r_cap > r->max_size){
      r_cap = r->max_size;
      r_id = i;
    }
  }

  if (r_id == NONE){
    log_debug("Cannot play def by player %d, lack of rooms", player_id);
    return false;
  }

  game_define_new_game(g, r_id, player_id, player_arr);
  return r_id;
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

typedef enum game_event_type {
  ev_player_register = 1,
  ev_server_welcome = 1 << 1,

  ev_player_definition = 1 << 2,

  ev_server_ready_room = 1 << 3,
  ev_player_joining_room = 1 << 4,
  ev_server_room_started = 1 << 5,
  ev_player_leaving_room = 1 << 6,

  ev_player_finished = 1 << 7,
  ev_sever_finished = 1 << 8,

} GameEventType;

typedef struct game_message {
  GameEventType type;
  int player_id;
  GameDef game_def;
} GameMsg;

GameMsg buff_msg;
const int MSG_SIZE = sizeof(GameMsg);

void game_send_server_welcome(IpcManager *ipc, int client) {
  buff_msg.type = ev_server_welcome;
  buff_msg.player_id = NONE;
  game_def_init(&buff_msg.game_def);
  ipc_sendto_client(ipc, &buff_msg, MSG_SIZE, client);
}

void game_send_player_register(IpcManager *ipc, int player_id) {
  buff_msg.type = ev_player_register;
  buff_msg.player_id = player_id;
  game_def_init(&buff_msg.game_def);
  ipc_sendto_server(ipc, &buff_msg, MSG_SIZE);
}

void game_send_player_definition(IpcManager *ipc, int player_id, GameDef *def) {
  buff_msg.type = ev_player_definition;
  buff_msg.player_id = player_id;
  memcpy(&(buff_msg.game_def), def, sizeof(GameDef));
  ipc_sendto_server(ipc, &buff_msg, MSG_SIZE);
}

GameMsg *game_read_client_event(IpcManager *ipc, int expected_ev) {
  ipc_getfrom_client(ipc, &buff_msg, MSG_SIZE);
  assertion((expected_ev & buff_msg.type) != 0 && "Unexpected event type");
  return &buff_msg;
}

GameMsg *game_read_server_event(IpcManager *ipc, int client, int expected_ev) {
  ipc_getfrom_server(ipc, &buff_msg, MSG_SIZE, client);
  assertion((expected_ev & buff_msg.type) != 0 && "Unexpected event type");
  return &buff_msg;
}

#endif // ESCROOM_GAME_H
