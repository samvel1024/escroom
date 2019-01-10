#include "messaging.h"
#include "game.h"

GameDef def_buff;
IpcManager *ipc;
int player_id;
char debug_str[10000];

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
  game_send_player_register(ipc, player_id, c);
  bool end = false;
  while (!end) {
    GameMsg *msg =
        game_receive_server_event(ipc, player_id, ev_server_accepting_defs | ev_server_invite_room | ev_server_finished);
    switch (msg->type) {
      case ev_server_accepting_defs: {
        bool send_again = true;
        while (send_again) {
          GameDef *def = game_def_read_next(&def_buff, stdin, player_id);
          if (def == NULL) {
            game_send_player_finished(ipc, player_id);
            send_again = false;
          } else {
            game_send_player_definition(ipc, player_id, def);
            GameMsg *resp = game_receive_server_event(ipc, player_id, ev_server_received_def);
            send_again = !resp->def_valid;
            if (!resp->def_valid){
              log_debug("Illegal game def: %s", game_def_to_string(def, debug_str));
            }else {
              log_debug("Game def was added to queue: %s", game_def_to_string(def, debug_str));
            }
          }
        }
        break;
      }
      case ev_server_invite_room: {
        game_send_player_joining_room(ipc, player_id, msg->room_id);
        GameMsg *resp = game_receive_server_event(ipc, player_id, ev_server_wait_for_players);
        log_debug("Waiting in room %d , player list %s", resp->room_id, arr_to_str(resp->players_in_room, -1, debug_str));
        int m_index = NONE;
        for(int i=0; resp->players_in_room[i] != NONE; ++i){
          if (resp->players_in_room[i] == player_id){
            m_index = i;
            break;
          }
        }
        assertion(m_index != NONE);
        log_debug("Waiting for players %s", arr_to_str(&resp->players_in_room[m_index+1], -1, debug_str));
        game_receive_server_event(ipc, player_id, ev_server_room_started);
        //Game started
        game_send_player_leaving_room(ipc, player_id);
        break;
      }
      case ev_server_finished: {
        end = true;
        break;
      }
      default: assertion(false && "Unexpected message");
    }
  }
}

int main(int argc, char **argv) {
  assert(argc > 0 && "Count has to be greater than 0");
  player_id = atoi(argv[1]);
  char buff[MSG_BUFF_LEN];
  sprintf(buff, "Player-%d", player_id);

  open_input(player_id);

  log_init(buff);
  ipc = ipc_create(false, "es", player_id);
  game_loop();
  ipc_close(ipc);
  log_debug("Finishing gracefully");
}