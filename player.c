
#include "utils.h"
#include "game.h"

int player_id;

#ifdef DEBUG
const char *test_dir = "../input/";
#else
const char *test_dir = "";
#endif


Game *game;


void player_init_type(){
    char t;
    scanf("%c\n", &t);
    game_init_player(game, player_id, t);
    log_debug("Initialized player p=%d t=%c", player_id, t);
}


int main(int argc, char **argv){
    assertion(argc == 2);
    int parsed = sscanf(argv[1], "%d", &player_id);
    assertion(parsed > 0 && "Could not parse arguments");
    char input_file[100];
    sprintf(input_file, "%splayer-%d.in", test_dir, player_id);
    freopen(input_file, "r", stdin);

    log_init(input_file);


    return 0;
}