#include "messaging.h"
#include "game.h"

GameDef def;
Game *game;
IpcManager *ipc;
int player_id;

void open_input(int id) {
  char p[40];
#ifdef DEBUG
  sprintf(p, "../input/player-%d.in", id + 1);
#else
  sprintf(p, "player-%d.in", id+1);
#endif
  freopen(p, "r", stdin);
}

void game_loop() {
  char c;
  scanf("%c\n", &c);
  game_init_player(game, player_id, c);
  game_send_player_register(ipc, player_id);
  game_read_server_event(ipc, player_id, ev_server_welcome);
  log_debug("Got game started message");
  GameDef *d = game_def_read_next(&def, stdin);

  while (d != NULL) {
    log_debug("Sending game definition");
    game_send_player_definition(ipc, player_id, d);
    d = game_def_read_next(&def, stdin);
  }
}

int main(int argc, char **argv) {
  assert(argc > 0 && "Count has to be greater than 0");
  player_id = atoi(argv[1]);
  char buff[MSG_BUFF_LEN];
  sprintf(buff, "Player-%d", player_id);

  open_input(player_id);

  log_init(buff);
  game = game_bind_shared();
  ipc = ipc_create(false, "es", player_id);
  game_loop();
  ipc_close(ipc);
}