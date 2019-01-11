
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
#define NONE -1

typedef struct room_t {
  //
  short max_size;
  short type;
  // Mutable
  short inside_count;
  short waiting_game_size;
  bool game_started;
  short defined_by;
  short players_inside[MAX_PLAYERS];
} Room;

typedef struct player_t {
  //
  short id;
  short type;
  // Mutable
  short assigned_room;

} Player;

typedef struct game_t {
  Room rooms[MAX_ROOMS];
  Player players[MAX_PLAYERS];
  short player_count;
  short room_count;
  short finished_players;
} Game;

typedef struct game_def_t {
  short created_by;
  char room_type;
  short types[MAX_TYPES];
  short ids[MAX_PLAYERS];
} GameDef;

void game_def_init(GameDef *def) {
  def->created_by = NONE;
  def->room_type = NONE;
  for (int i = 0; i < MAX_TYPES; ++i)
    def->types[i] = 0;
  def->ids[0] = NONE;
}

char *game_def_to_string(GameDef *def, char *buff) {
  buff[0] = '\0';
  sprintf(buff + strlen(buff), "room_type=%c created_by=%d ", def->room_type, def->created_by);
  sprintf(buff + strlen(buff), "types={");
  for (int i = 0; i < MAX_TYPES; ++i) {
    if (def->types[i] > 0) {
      sprintf(buff + strlen(buff), "%c:%d ", 'A' + i, def->types[i]);
    }
  }
  sprintf(buff + strlen(buff), "} ");
  sprintf(buff + strlen(buff), "ids={");
  char b[5000];
  sprintf(buff + strlen(buff), "%s}",  arr_to_str(def->ids, NONE, b));
  return buff;
}

GameDef *game_def_read_next(GameDef *def, FILE *f, short player_id, char *raw) {
  if (feof(f))
    return NULL;
  char buff[MSG_BUFF_LEN];
  char *line = fgets(buff, MSG_BUFF_LEN, f);
  assertion(ferror(f) == 0 && "There was error reading the file")
  if (line == NULL)
    return NULL;
  strcpy(raw, line);
  strtok(raw, "\n");
  int ids = 0;
  char *token = strtok(line, " \n");
  game_def_init(def);
  def->ids[ids++] = player_id;
  while (token != NULL) {
    if (token == line) { // room type (has to be the first)
      assertion(strlen(token) == 1 && "Type of the room cannot be longer than 1 char");
      def->room_type = *token;
    } else if (isalpha(*token)) { // type
      def->types[*token - 'A']++;
    } else { // id
      short id = (short) (atoi(token) - 1);
      def->ids[ids++] = id;
    }
    token = strtok(NULL, " ");
  }
  def->ids[ids] = NONE;
  def->created_by = player_id;
  return def;
}

void player_set_free(Player *p) {
  p->assigned_room = NONE;
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

Game *game_init(short players, short rooms) {
  Game *g = malloc(sizeof(Game));
  g->player_count = players;
  g->room_count = rooms;
  g->finished_players = 0;
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
  return g;
}

void game_destroy(Game *g) {
  free(g);
}


void game_init_room(Game *g, short room, short type, short size) {
  assertion(room < g->room_count);
  assertion(type <= 'Z' && type >= 'A');
  g->rooms[room].type = type;
  g->rooms[room].max_size = size;
  log_debug("Initialized room r=%d t=%c s=%d", room, (char) type, size);
}

void game_init_player(Game *g, short p, short t) {
  assertion(p < g->player_count && 'A' <= t && 'Z' >= t);
  g->players[p].type = t;
  g->players[p].assigned_room = NONE;
}

bool game_player_finished(Game *g){
  g->finished_players++;
  return g->finished_players == g->player_count;
}


void game_define_new_game(Game *g, short room, short owner, short *players) {
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
    assertion((p->assigned_room == NONE) && "Player is not free");
    r->players_inside[i] = players[i];
    r->waiting_game_size++;
    g->players[players[i]].assigned_room = room;
  }
  char msg[5000];
  log_debug("Defined a game in room owned by r=%d own=%d players=%s", room, owner, arr_to_str(players, NONE,msg ));
}

bool game_add_player_to_waiting_list(Game *g, short room, short player) {
  assertion((room < g->room_count && player < g->player_count) && "Indexes out of range");
  assertion((!g->rooms[room].game_started) && "Game is already started, cannot join the room");
  assertion((g->rooms[room].inside_count + 1 <= g->rooms[room].max_size) && "Room is full, cannot join");
  assertion((g->players[player].assigned_room == room) && "The requested player is not assigned to this room");

  Room *r = &(g->rooms[room]);
  int player_index = NONE;
  for (int i = 0; i < MAX_PLAYERS && r->players_inside[i] != NONE; ++i) {
    if (r->players_inside[i] == player) {
      player_index = i;
    }
  }

  assertion((player_index != NONE) && "The game hosted in this room is not expecting the player to join");
  assertion((player_index >= r->inside_count) && "The player is already waiting for the game to start");

  short tmp = r->players_inside[r->inside_count];
  r->players_inside[r->inside_count] = r->players_inside[player_index];
  r->players_inside[player_index] = tmp;

  r->inside_count++;
  g->players[player].assigned_room = room;

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

void append_arr(short *arr, int *len, short val) {
  arr[*len] = val;
  arr[*len + 1] = NONE;
  (*len)++;
}

int game_start_if_possible(Game *g, GameDef *def) {
  short owner_id = def->created_by;
  // The owner of the definition has to be free
  if (g->players[owner_id].assigned_room != NONE)
    return NONE;

  // iff Selected[i] then player i will take part in the game
  bool selected[MAX_PLAYERS];
  for (int i = 0; i < MAX_PLAYERS; ++i)
    selected[i] = false;
  short player_arr[MAX_PLAYERS];
  int len = 0;
  player_arr[0] = NONE;

  // Make sure all the directly specified players are free
  for (int i = 0; def->ids[i] != NONE; ++i) {
    if (g->players[def->ids[i]].assigned_room != NONE && !selected[def->ids[i]]) {
      log_debug("Cannot play def by player %d, cannot find players by ids", owner_id);
      return NONE;
    } else {
      selected[i] = true;
      append_arr(player_arr, &len, def->ids[i]);
    }
  }

  // Choose players by their type
  int wanted_types[MAX_TYPES];
  memcpy(wanted_types, def->types, sizeof(wanted_types));
  for (short i = 0; i < g->player_count; ++i) { //TODO change i initial value to be more fair
    Player *p = &g->players[i];
    int p_type = p->type - 'A';
    if (wanted_types[p_type] > 0 && p->assigned_room == NONE && !selected[i]) {
      wanted_types[p_type]--;
      append_arr(player_arr, &len, i);
    }
  }
  for (int i = 0; i < MAX_TYPES; ++i) {
    if (wanted_types[i] > 0) {
      log_debug("Cannot play def by player %d, cannot find players by types", owner_id);
      return NONE;
    }
  }

  //Choose the room
  short r_id = NONE;
  short r_cap = SHRT_MAX;
  for (short i = 0; i < g->room_count; ++i) {
    Room *r = &g->rooms[i];
    if (r->type == def->room_type && r->max_size >= len && r_cap > r->max_size) {
      r_cap = r->max_size;
      r_id = i;
    }
  }

  if (r_id == NONE) {
    log_debug("Cannot play def by player %d, lack of rooms", owner_id);
    return false;
  }

  game_define_new_game(g, r_id, owner_id, player_arr);
  return r_id;
}

bool game_player_leave_room(Game *g, int player) {
  assertion((player < g->player_count) && "Player out of bounds");
  Player *p = &(g->players[player]);
  assertion((p->assigned_room != NONE) && "Player is free, cannot leave a room");
  Room *r = &(g->rooms[p->assigned_room]);
  r->inside_count--;
  log_debug("Player leaved room %d %d", player, p->assigned_room);
  player_set_free(p);
  if (r->inside_count == 0) {
    room_set_empty(r);
    log_debug("Game is finished in room %d", p->assigned_room);
  }
  return !r->game_started;
}

typedef enum game_event_type {
  ev_player_register = 1,
  ev_server_accepting_defs = 1 << 1,

  ev_player_def = 1 << 2,
  ev_server_received_def = 1 << 3,
  ev_server_invite_room = 1 << 4,
  ev_player_joining_room = 1 << 5,
  ev_server_wait_for_players = 1 << 6,
  ev_server_room_started = 1 << 7,
  ev_player_leaving_room = 1 << 8,

  ev_player_finished = 1 << 9,
  ev_server_finished = 1 << 10,

} GameEventType;

typedef struct game_message {
  short player_id;
  short player_type;
  short room_id;
  short players_in_room[MAX_PLAYERS];
  bool def_valid;
  int room_owner;
  GameEventType type;
  GameDef game_def;
} GameMsg;

GameMsg buff_msg;
const int MSG_SIZE = sizeof(GameMsg);

void game_msg_init(GameMsg *msg) {
  msg->player_id = NONE;
  msg->room_id = NONE;
  msg->type = NONE;
  msg->players_in_room[0] = NONE;
  msg->player_type = NONE;
  msg->def_valid = false;
  msg->room_owner = NONE;
  game_def_init(&msg->game_def);
}

void game_msg_init_players(const short *players){
  int i;
  for(i=0; players[i] != NONE; ++i)
    buff_msg.players_in_room[i] = players[i];
  buff_msg.players_in_room[i] = NONE;
}

/******************************* Sending events by server  *************************
 ***********************************************************************************/

void game_send_server_accepting_defs(IpcManager *ipc, short player_id){
  game_msg_init(&buff_msg);
  buff_msg.type = ev_server_accepting_defs;
  buff_msg.player_id = player_id;
  ipc_sendto_client(ipc, &buff_msg, MSG_SIZE, player_id);
}

void game_send_server_room_started(IpcManager *ipc, short player_id, short room_id) {
  game_msg_init(&buff_msg);
  buff_msg.type = ev_server_room_started;
  buff_msg.room_id = room_id;
  buff_msg.player_id = player_id;
  ipc_sendto_client(ipc, &buff_msg, MSG_SIZE, player_id);
}


void game_send_server_received_def(IpcManager *ipc, short player_id, bool ok){
  game_msg_init(&buff_msg);
  buff_msg.type = ev_server_received_def;
  buff_msg.player_id = player_id;
  buff_msg.def_valid = ok;
  ipc_sendto_client(ipc, &buff_msg, MSG_SIZE, player_id);
}

void game_send_server_invite_room(IpcManager *ipc, short room_id, short player_id, Game *g){
  game_msg_init(&buff_msg);
  buff_msg.type = ev_server_invite_room;
  game_msg_init_players(g->rooms[room_id].players_inside);
  buff_msg.player_id = player_id;
  buff_msg.room_id = room_id;
  buff_msg.room_owner = g->rooms[room_id].defined_by;
  ipc_sendto_client(ipc, &buff_msg, MSG_SIZE, player_id);
}

void game_send_server_wait_for_players(IpcManager *ipc, short player_id, short room_id, Game *g) {
  game_msg_init(&buff_msg);
  game_msg_init_players(g->rooms[room_id].players_inside);
  buff_msg.type = ev_server_wait_for_players;
  buff_msg.room_id = room_id;
  buff_msg.player_id = player_id;
  buff_msg.room_owner = g->rooms[room_id].defined_by;
  ipc_sendto_client(ipc, &buff_msg, MSG_SIZE, player_id);
}

void game_send_server_finished(IpcManager *ipc, short player_id){
  game_msg_init(&buff_msg);
  buff_msg.type = ev_server_finished;
  ipc_sendto_client(ipc, &buff_msg, MSG_SIZE, player_id);
}

/******************************* Sending events by client  *************************
 ***********************************************************************************/


void game_send_player_register(IpcManager *ipc, short player_id, short player_type) {
  game_msg_init(&buff_msg);
  buff_msg.player_type = player_type;
  buff_msg.type = ev_player_register;
  buff_msg.player_id = player_id;
  ipc_sendto_server(ipc, &buff_msg, MSG_SIZE);
}


void game_send_player_finished(IpcManager *ipc, short player_id) {
  game_msg_init(&buff_msg);
  buff_msg.type = ev_player_finished;
  buff_msg.player_id = player_id;
  ipc_sendto_server(ipc, &buff_msg, MSG_SIZE);
}

void game_send_player_definition(IpcManager *ipc, short player_id, GameDef *def) {
  game_msg_init(&buff_msg);
  buff_msg.type = ev_player_def;
  buff_msg.player_id = player_id;
  memcpy(&(buff_msg.game_def), def, sizeof(GameDef));
  ipc_sendto_server(ipc, &buff_msg, MSG_SIZE);
}

void game_send_player_joining_room(IpcManager *ipc, short player_id, short room_id) {
  game_msg_init(&buff_msg);
  buff_msg.type = ev_player_joining_room;
  buff_msg.room_id = room_id;
  buff_msg.player_id = player_id;
  ipc_sendto_server(ipc, &buff_msg, MSG_SIZE);
}

void game_send_player_leaving_room(IpcManager *ipc, short player_id){
  game_msg_init(&buff_msg);
  buff_msg.type = ev_player_leaving_room;
  buff_msg.player_id = player_id;
  ipc_sendto_server(ipc, &buff_msg, MSG_SIZE);
}


/******************************* Receiving events  *********************************
 ***********************************************************************************/

GameMsg *game_receive_client_event(IpcManager *ipc, short expected_ev, GameMsg *msg) {
  ipc_getfrom_client(ipc, msg, MSG_SIZE);
  assertion((expected_ev & msg->type) != 0 && "Unexpected event type");
  return msg;
}

GameMsg *game_receive_server_event(IpcManager *ipc, short client, short expected_ev, GameMsg *msg) {
  ipc_getfrom_server(ipc, msg, MSG_SIZE, client);
  if ((expected_ev & msg->type) == 0){
    log_debug("Error: expected %d but got %d", expected_ev, msg->type);
  }
  assertion((expected_ev & msg->type) != 0 && "Unexpected event type");
  return msg;
}

#endif // ESCROOM_GAME_H
