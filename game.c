

#include "game.h"

#define MAX_MSG_LEN 7000

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
  sprintf(buff + strlen(buff), "%s}", arr_to_str(def->ids, NONE, b));
  return buff;
}

GameDef *game_def_read_next(GameDef *def, FILE *f, short player_id, char *raw) {
  if (feof(f))
    return NULL;
  char buff[MAX_MSG_LEN];
  char *line = fgets(buff, MAX_MSG_LEN, f);
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

bool game_player_finished(Game *g) {
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
  log_debug("Defined a game in room owned by r=%d own=%d players=%s", room, owner, arr_to_str(players, NONE, msg));
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

int game_find_room(Game *g, int type, int size) {
  assertion(type >= 'A' && type <= 'Z');
  for (int i = 0; i < g->room_count; ++i) {
    if (g->rooms[i].type == type && g->rooms[i].max_size >= size)
      return i;
  }
  return NONE;
}

bool game_is_ever_playable(Game *g, GameDef *def, int player_id) {
  assertion(player_id < g->player_count && player_id >= 0);

  int player_count = 0;
  bool busy[MAX_PLAYERS];
  for (int i = 0; i < MAX_PLAYERS; ++i)
    busy[i] = false;
  for (int i = 0; i < MAX_PLAYERS && def->ids[i] != NONE; ++i) {
    if (def->ids[i] < 0 || def->ids[i] >= g->player_count) // No player with that id
      return false;
    busy[def->ids[i]] = true;
    player_count++;
  }

  short wanted_types[MAX_TYPES];
  memcpy(wanted_types, def->types, MAX_TYPES * sizeof(short));
  for (int i = 0; i < g->player_count; ++i) {
    if (!busy[i]) {
      wanted_types[g->players[i].type - 'A']--;
      player_count++;
    }
  }
  for (int i = 0; i < MAX_TYPES; ++i) {
    if (wanted_types[i] > 0)
      return false;
  }

  if (game_find_room(g, def->room_type, player_count) == NONE)
    return false;

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
      selected[def->ids[i]] = true;
      append_arr(player_arr, &len, def->ids[i]);
    }
  }

  // Choose players by their type
  short wanted_types[MAX_TYPES];
  memcpy(wanted_types, def->types, MAX_TYPES * sizeof(short));
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
    return NONE;
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
