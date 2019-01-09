#include "game.h"

int main() {
  freopen("../test.in", "r", stdin);
  GameDef d;
  GameDef *def = game_def_read_next(&d, stdin);
  while(def != NULL){

    printf("%c ", def->room_type);
    for(int i=0; i<26; ++i){
      if (def->types[i] > 0){
        printf("%c:%d ", 'A' + i, def->types[i]);
      }
    }

    for(int i=0; def->ids[i] != NONE; ++i){
      printf("%d ", def->ids[i]);
    }
    def = game_def_read_next(&d, stdin);
    printf("\n");
  }
}