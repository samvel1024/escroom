#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;

int players_size;
int rooms_size;

char rand_char() {
  return static_cast<char>('A' + (rand() % 26));
}

int rand_player() {
  return 1 + (rand() % (players_size + 10));
}

int rand_capacity() {
  return 1 + rand() % 99;
}

vector<pair<char, int>> rooms;
vector<char> players;

int min(int a, int b) {
  return a < b ? a : b;
}

using namespace std::chrono;
int main() {

  string dir;
  int games;
  int seed;
  cin >> players_size >> rooms_size >> games >>seed >> dir;
  srand(seed);

  freopen((dir + "/manager.in").c_str(), "w", stdout);
  cout << players_size << " " << rooms_size << endl;

  for (int i = 0; i < rooms_size; ++i) {
    rooms.emplace_back(rand_char(), rand_capacity());
    cout << rooms[i].first << " " << rooms[i].second << endl;
  }

  for (int i = 0; i < players_size; ++i) {
    players.emplace_back(rand_char());
  }

  for (int curr_player = 0; curr_player < players_size; ++curr_player) {
    freopen((dir + "/player-" + to_string(curr_player + 1) + ".in").c_str(), "w", stdout);
    cout << players[curr_player] << endl;
    //Generate games for player i
    for (int g = 0; g < games; ++g) {
      int room_id = rand() % rooms_size;
      char room_type = rooms[room_id].first;
      int room_size = rooms[room_id].second - 1; // The player i has to play anyway
      int game_size = min(room_size,  rand() % (players_size - 1));

      cout << room_type << " ";
      vector <int> participants;
      for(int pl=0; pl<players_size; ++pl){
        if (pl == curr_player) continue;
        participants.push_back(pl);
      }
      random_shuffle(participants.begin(), participants.end());
      // For each player in game
      for (int p = 0; p < game_size; ++p) {
        bool by_id = static_cast<bool>(rand() % 2);
        //The player i should not select himself
        int participant_id = participants[p];
        if (by_id) {
          cout << participant_id + 1 << " ";
        } else {
          cout << players[participant_id] << " ";
        }

      }
      cout << endl;

    }
  }

}


