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
  milliseconds ms = duration_cast<milliseconds>(
      system_clock::now().time_since_epoch()
  );
  srand(static_cast<unsigned int>(ms.count()));

  string dir;
  int games;
  cin >> players_size >> rooms_size >> games >> dir;
  freopen((dir + "/manager.in").c_str(), "w", stdout);
  cout << players_size << " " << rooms_size << endl;

  for (int i = 0; i < rooms_size; ++i) {
    rooms.emplace_back(rand_char(), rand_capacity());
    cout << rooms[i].first << " " << rooms[i].second << endl;
  }

  for (int i = 0; i < players_size; ++i) {
    players.emplace_back(rand_char());
  }

  for (int i = 0; i < players_size; ++i) {
    freopen((dir + "/player-" + to_string(i + 1) + ".in").c_str(), "w", stdout);
    cout << players[i] << endl;
    for (int g = 0; g < games; ++g) {
      pair<char, int> r = rooms[rand() % rooms.size()];
      cout << r.first << " ";
      int room_cap = r.second;
      vector<int> p_index;
      for (int j = 0; j < players_size; ++j) p_index.emplace_back(j);
      random_shuffle(p_index.begin(), p_index.end());
      for (int j = 0; j < min(players.size() - 1, room_cap); ++j) {
        if (p_index[j] == i) continue;
        bool by_type = static_cast<bool>(rand() % 2);
        if (by_type) {
          cout << players[p_index[j]] << " ";
        } else {
          cout << p_index[j] + 1 << " ";
        }
      }
      cout << endl;
    }
  }

}


