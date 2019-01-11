#ifndef ESCROOM_MESSAGING_H
#define ESCROOM_MESSAGING_H

#include "game.h"
#include "ipc.h"

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

/******************************* Sending events by server  *************************
 ***********************************************************************************/

void game_send_server_accepting_defs(IpcManager *ipc, short player_id);

void game_send_server_room_started(IpcManager *ipc, short player_id, short room_id);

void game_send_server_received_def(IpcManager *ipc, short player_id, bool ok);

void game_send_server_invite_room(IpcManager *ipc, short room_id, short player_id, Game *g);

void game_send_server_wait_for_players(IpcManager *ipc, short player_id, short room_id, Game *g);

void game_send_server_finished(IpcManager *ipc, short player_id);

/******************************* Sending events by client  *************************
 ***********************************************************************************/

void game_send_player_register(IpcManager *ipc, short player_id, short player_type);

void game_send_player_finished(IpcManager *ipc, short player_id);

void game_send_player_definition(IpcManager *ipc, short player_id, GameDef *def);

void game_send_player_joining_room(IpcManager *ipc, short player_id, short room_id);

void game_send_player_leaving_room(IpcManager *ipc, short player_id);

/******************************* Receiving events  *********************************
 ***********************************************************************************/

GameMsg *game_receive_client_event(IpcManager *ipc, short expected_ev, GameMsg *msg);

GameMsg *game_receive_server_event(IpcManager *ipc, short client, short expected_ev, GameMsg *msg);

#endif //ESCROOM_MESSAGING_H
