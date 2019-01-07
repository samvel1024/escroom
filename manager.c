#include <stdio.h>
#include "utils.h"
#include "game.h"

void *init_debug_input() {
#ifdef DEBUG
    return freopen("../input/manager.in", "r", stdin);
#endif
}

Game g;


void mngr_init_rooms(){
    int p, r;
    scanf("%d %d ", &p, &r);
    game_init(&g, p, r);
    for(int i=0; i<r; ++i){
        char t;
        int s;
        scanf("%c %d ", &t, &s);
        game_init_room(&g, i, (int)t, s);
    }
}





int main() {
    log_init("Manager");
    init_debug_input();
    mngr_init_rooms();
}
