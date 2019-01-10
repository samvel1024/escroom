#include "messaging.h"
#include "game.h"

void open_debug_input() {
#ifdef DEBUG
  freopen("../input/manager.in", "r", stdin);
#endif
}

typedef struct list_node_t {
  GameDef def;
  struct list_node_t *next;
} Node;

Game *game;
IpcManager *ipc;
GameMsg msg_buff;
Node head;
Node *tail = &head;
char debug_buff[10000];

void add_to_queue(GameDef *def) {
  Node *n = malloc(sizeof(Node));
  memcpy(&n->def, def, sizeof(GameDef));
  log_debug("Added to queue game def: %s", game_def_to_string(def, debug_buff));
  n->next = NULL;
  tail->next = n;
  tail = n;
}

int start_playable() {
  Node *prev = &head;
  while (prev->next != NULL) {
    Node *curr = prev->next;
    int room;
    if ((room = game_start_if_possible(game, &curr->def)) != NONE) {
      log_debug("Started game defined as: %s", game_def_to_string(&(curr->def), debug_buff));
      free(curr);
      prev->next = curr->next;
      if (prev->next == NULL){
        tail = prev;
      }
      return room;
    }
    prev = prev->next;
  }
  return NONE;
}

void init_rooms() {
  char r;
  int c;
  for (int i = 0; i < game->room_count; ++i) {
    scanf("%c %d\n", &r, &c);
    game_init_room(game, i, r, c);
  }
}

void spawn_players() {
  for (int i = 0; i < game->player_count; ++i) {
    switch (fork()) {
      case -1: {
        assertion(false && "Could not fork");
      }
      case 0: {
        char str[50];
        sprintf(str, "%d", i);
        execlp("./player", "player", str, NULL);
        assertion(false && "Could not start player");
      }
      default:break;
    }
  }
}

void notify_if_any_playable() {
  int r_id = start_playable();
  if (r_id == NONE) return;
  Room *r = &game->rooms[r_id];
  for (int i = 0; r->players_inside[i] != NONE; ++i) {
    game_send_server_invite_room(ipc, r_id, r->players_inside[i], game);
  }
}

void game_loop() {
  for (int i = 0; i < game->player_count; ++i) {
    GameMsg *msg = game_receive_client_event(ipc, ev_player_register, &msg_buff);
    game_init_player(game, msg->player_id, msg->player_type);
    log_debug("Registered player %d type %c", msg->player_id, game->players[msg->player_id].type);
  }
  for (int i = 0; i < game->player_count; ++i) {
    game_send_server_accepting_defs(ipc, i);
  }
  log_debug("Notified all clients");

  bool end = false;
  while (!end) {
    GameMsg *msg = game_receive_client_event(ipc, ev_player_joining_room |
        ev_player_def |
        ev_player_leaving_room |
        ev_player_finished, &msg_buff);

    switch (msg->type) {
      case ev_player_def: {
        log_debug("ev_player_def: Got def from player %d", msg->player_id);
        bool ok = game_is_ever_playable(game, &msg->game_def, msg->player_id);
        game_send_server_received_def(ipc, msg->player_id, ok);
        if (ok) {
          add_to_queue(&msg->game_def);
          notify_if_any_playable();
        }
        break;
      }
      case ev_player_joining_room: {
        log_debug("ev_player_joining_room: Player %d joined room %d", msg->player_id, msg->room_id);
        bool started = game_add_player_to_waiting_list(game, msg->room_id, msg->player_id);
        game_send_server_wait_for_players(ipc, msg->player_id, msg->room_id, game);
        if (started) {
          Room *r = &game->rooms[msg->room_id];
          for (int i = 0; r->players_inside[i] != NONE; ++i) {
            game_send_server_room_started(ipc, r->players_inside[i], msg->room_id);
          }
        }
        break;
      }
      case ev_player_leaving_room: {
        log_debug("ev_player_leaving_room: Player %d left room %d", msg->player_id, msg->room_id);
        int room_defined_by = game->rooms[game->players[msg->player_id].assigned_room].defined_by;
        bool room_empty = game_player_leave_room(game, msg->player_id);
        if (room_empty) {
          game_send_server_accepting_defs(ipc, room_defined_by);
          notify_if_any_playable();
        }
        break;
      }
      case ev_player_finished: {
        log_debug("ev_player_finished: Player %d finished", msg->player_id);
        bool game_finished = game_player_finished(game, msg->player_id);
        if (game_finished) {
          for (int i = 0; i < game->player_count; ++i) {
            game_send_server_finished(ipc, i);
          }
          end = tail == &head;
        }
        break;
      }
      default: assertion(false && "Illegal value of event");
    }
  }

}
int main() {
  open_debug_input();
  log_init("Manager");
  int players, rooms;
  scanf("%d %d\n", &players, &rooms);
  game = game_init(players, rooms);
  ipc = ipc_create(true, "e1", -1);
  init_rooms();
  spawn_players();
  game_loop();

  wait(NULL);
  ipc_close(ipc);
  log_debug("Finishing gracefully");
  game_destroy(game);
}



