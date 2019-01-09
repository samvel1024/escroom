#include "messaging.h"
#include "game.h"

void open_debug_input() {
#ifdef DEBUG
  freopen("../input/manager.in", "r", stdin);
#endif
}

Game *game;
IpcManager *ipc;

void init_rooms() {
  char r;
  int c;
  for(int i=0; i<game->room_count; ++i){
    scanf("%c %d\n", &r, &c);
    game_init_room(game, i, r, c);
  }
}

void spawn_players() {
  for (int i = 0; i < game->player_count; ++i) {
    switch (fork()) {
      case -1: {
        assertion(false && "Could not fork");
        break;
      }
      case 0: {
        char str[50];
        sprintf(str, "%d", i);
        execlp("./player", "player", str, NULL);
        assertion(false && "Could not start player");
      }
    }
  }
}

void game_loop() {
  for (int i = 0; i < game->player_count; ++i) {
    GameMsg *msg = game_read_client_event(ipc, ev_player_register);
    log_debug("Registered player %d", msg->player_id);
  }
  for (int i = 0; i < game->player_count; ++i) {
    game_send_server_welcome(ipc, i);
  }
  log_debug("Notified all clients");

  char bff[10000];
  while (stdin != NULL) {
      GameMsg *msg = game_read_client_event(ipc, ev_player_definition);
      log_debug("def from %d: %s", msg->player_id, game_def_to_string(&msg->game_def, bff));
      bool ok =  game_is_playable(game, &msg->game_def, msg->player_id);
      log_debug("Def from player %d is ok %d", msg->player_id, ok);
  }

}
int main() {
  open_debug_input();
  log_init("Manager");
  int players, rooms;
  scanf("%d %d\n", &players, &rooms);
  game = game_shared_init(players, rooms);
  ipc = ipc_create(true, "es", -1);
  init_rooms();
  spawn_players();
  game_loop();

  wait(NULL);
  ipc_close(ipc);
  log_debug("Finishing gracefully");
}



