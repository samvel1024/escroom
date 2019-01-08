#include "messaging.h"
#include "game.h"




Game *game;
IpcManager *ipc;
GameMsg *msg;
char msg_str[MSG_BUFF_LEN];
int player_id;

void open_input(int id){
  char p[40];
#ifdef DEBUG
  sprintf(p, "../input/player-%d.in", id+1);
#else
  sprintf(p, "player-%d.in", id+1);
#endif
  freopen(p, "r", stdin);
}


void init_player(){
  char c;
  scanf("%c", &c);
  log_debug("%c", c);
  game_init_player(game, player_id, c);
  log_debug("Sending");
  game_send_player_register(ipc, player_id);
}


void wait_for_game_start(){
  game_read_server_event(ipc, player_id, ev_server_game_started);
  log_debug("Got game started message");
}


int main(int argc, char **argv){
  assert(argc > 0 && "Count has to be greater than 0");
  player_id = atoi(argv[1]);
  char buff[MSG_BUFF_LEN];
  sprintf(buff, "Player-%d", player_id);

  open_input(player_id);

  log_init(buff);
  game = game_bind_shared();
  IpcManager *ipc = ipc_create(false, "es", player_id);

  init_player();
  wait_for_game_start();


  ipc_close(ipc);
}