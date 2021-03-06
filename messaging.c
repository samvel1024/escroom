
#include "messaging.h"

#define MSG_SIZE sizeof(GameMsg)
GameMsg buff_msg;

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

void game_msg_init_players(const short *players) {
  int i;
  for (i = 0; players[i] != NONE; ++i)
    buff_msg.players_in_room[i] = players[i];
  buff_msg.players_in_room[i] = NONE;
}

/******************************* Sending events by server  *************************
 ***********************************************************************************/

void game_send_server_accepting_defs(IpcManager *ipc, short player_id) {
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

void game_send_server_received_def(IpcManager *ipc, short player_id, bool ok) {
  game_msg_init(&buff_msg);
  buff_msg.type = ev_server_received_def;
  buff_msg.player_id = player_id;
  buff_msg.def_valid = ok;
  ipc_sendto_client(ipc, &buff_msg, MSG_SIZE, player_id);
}

void game_send_server_invite_room(IpcManager *ipc, short room_id, short player_id, Game *g) {
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

void game_send_server_finished(IpcManager *ipc, short player_id) {
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

void game_send_player_leaving_room(IpcManager *ipc, short player_id) {
  game_msg_init(&buff_msg);
  buff_msg.type = ev_player_leaving_room;
  buff_msg.player_id = player_id;
  ipc_sendto_server(ipc, &buff_msg, MSG_SIZE);
}

/******************************* Receiving events  *********************************
 ***********************************************************************************/

GameMsg *game_receive_client_event(IpcManager *ipc, short expected_ev, GameMsg *msg) {
  supress_unused(expected_ev);
  ipc_getfrom_client(ipc, msg, MSG_SIZE);
  dassert((expected_ev & msg->type) != 0 && "Unexpected event type");
  return msg;
}

GameMsg *game_receive_server_event(IpcManager *ipc, short client, short expected_ev, GameMsg *msg) {
  supress_unused(expected_ev);
  ipc_getfrom_server(ipc, msg, MSG_SIZE, client);
  dassert((expected_ev & msg->type) != 0 && "Unexpected event type");
  return msg;
}

