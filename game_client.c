#include "messaging.h"
#include "game.h"
#include <stdarg.h>

GameDef def_buff;
GameMsg msg_buff;
IpcManager *ipc;
int player_id;
char debug_str[10000];
char raw_input[10000];

char *create_str(char *format, ...) {
  va_list args;
  va_start(args, format);
  vsprintf(debug_str, format, args);
  va_end(args);
  return debug_str;
}

void open_input(int id) {
#ifdef DEBUG
  freopen(create_str("../input/player-%d.in", id), "r", stdin);
#else
  freopen(create_str("player-%d.in", id), "r", stdin);
#endif
}

FILE *open_output(int id) {
#ifdef DEBUG
  return fopen(create_str("../output/player-%d.out", id), "w");
#else
  return fopen(create_str("player-%d.out", id), "w");
#endif
}

void read_and_send_def() {
  GameDef *def = game_def_read_next(&def_buff, stdin, player_id, raw_input);
  if (def == NULL) {
    game_send_player_finished(ipc, player_id);
  } else {
    game_send_player_definition(ipc, player_id, def);
  }
}

void game_loop() {
  char c;
  scanf("%c\n", &c);
  game_send_player_register(ipc, player_id, c);
  bool end = false;
  while (!end) {
    log_debug("Waiting for base events");
    GameMsg *msg = game_receive_server_event(ipc, player_id, ev_server_accepting_defs |
                                                 ev_server_received_def |
                                                 ev_server_invite_room |
                                                 ev_server_wait_for_players |
                                                 ev_server_room_started |
                                                 ev_server_finished,
                                             &msg_buff);
    switch (msg->type) {
      case ev_server_accepting_defs: {
        log_debug("Sending def to server");
        read_and_send_def();
        break;
      }
      case ev_server_received_def: {
        if (!msg->def_valid) {
          log_info("Invalid game \"%s\"\n", raw_input);
          log_debug("Resending def to server");
          read_and_send_def();
        } else {
          log_debug("Def was accepted by server");
        }
        break;
      }
      case ev_server_invite_room: {
        log_debug("Got invite to join room %d", msg->room_id);
        game_send_player_joining_room(ipc, player_id, msg->room_id);
        break;
      }
      case ev_server_wait_for_players: {
        log_debug("Waiting in room %d , player list %s",
                  msg->room_id,
                  arr_to_str(msg->players_in_room, NONE, debug_str));
        int players[MAX_PLAYERS];
        int players_len;
        for (players_len = 0; msg->players_in_room[players_len] != NONE; ++players_len) {
          players[players_len] = msg->players_in_room[players_len] + 1;
        }
        players[players_len] = NONE;
        log_info("Game defined by %d is going to start: room %d, players %s\n", msg->room_owner + 1,
                 msg->room_id + 1, arr_to_str(players, NONE, debug_str));

        int m_index = NONE;
        for (int i = 0; msg->players_in_room[i] != NONE; ++i) {
          if (msg->players_in_room[i] == player_id) {
            m_index = i;
            break;
          }
        }
        assertion(m_index != NONE);
        log_debug("Waiting for players %s", arr_to_str(&msg->players_in_room[m_index + 1], NONE, debug_str));
        log_info("Entered room %d, game defined by %d, waiting for players %s\n", msg->room_id + 1,
                 msg->room_owner + 1, arr_to_str(&players[m_index+1], NONE, debug_str));

        break;
      }
      case ev_server_room_started : {
        log_debug("Game finished in room %d", msg->room_id);
        game_send_player_leaving_room(ipc, player_id);
        log_info("Left room %d\n", msg->room_id + 1);
        break;
      }
      case ev_server_finished: {
        log_debug("Server finished");
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

  open_input(player_id + 1);
  log_init(buff, open_output(player_id + 1));
  ipc = ipc_create(false, "e1", player_id);
  game_loop();
  ipc_close(ipc);
  log_debug("Finishing gracefully");
}
