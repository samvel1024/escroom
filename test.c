#include "game.h"

int main() {
  freopen("../test.in", "r", stdin);
  char c;
  scanf("%c\n", &c);
  GameDef d;
  GameDef *def = game_def_read_next(&d, stdin);
  char b[1000];
  while(def != NULL){
    printf("%s\n", game_def_to_string(def, b));
    def = game_def_read_next(&d, stdin);
  }
}