#ifdef __GNUC__
// GCC
#include <bits/stdc++.h>
#elif defined(_MSC_VER)
// MSVC
#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <random>
#include <algorithm>
#include <climits>
#include <cstring>
#include <stack>    
#include <set>      
#endif

using namespace std;

#define WHITE 0
#define GRAY 1
#define BLACK 2
#define DEAD 3

// Lookup Table
int tau1[16] = {
   2, 5, 0, 6, 3, 7, 4, 1, 3, 0, 6, 1, 5, 7, 4, 2 };
int tau2[8] = {
   4, 5, 6, 7, 4, 3, 2, 1 };
int S2[64] = {
   15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10, 3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5, 0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15, 13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9 };
int S6[64] = {
   12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11, 10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8, 9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6, 4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13 };
int S2_inv[16][4] = {
   52, 37, 2, 51, 4, 41, 30, 15, 40, 21, 58, 31, 24, 1, 54, 19, 28, 9, 22, 27, 56, 61, 34, 55, 16, 49, 46, 39, 36, 13, 10, 43, 8, 25, 38, 7, 32, 53, 50, 63, 60, 45, 18, 11, 20, 57, 14, 35, 48, 33, 42, 47, 44, 5, 26, 3, 12, 29, 6, 59, 0, 17, 62, 23 };
int S6_inv[16][4] = {
   32, 49, 38, 55, 4, 37, 50, 43, 20, 13, 18, 11, 40, 57, 30, 7, 44, 9, 42, 3, 56, 29, 14, 23, 24, 33, 62, 51, 52, 17, 34, 47, 28, 61, 22, 59, 16, 25, 2, 19, 8, 1, 46, 31, 60, 53, 58, 35, 0, 21, 26, 15, 36, 41, 54, 63, 48, 45, 6, 39, 12, 5, 10, 27 };

// Global Variables
// Secret session key, Public initial state and known keystream
bool K[8][8], IS[8][8], IS_tmp[8][8], known_keystream[15][8];
// Table, Color, Black Vertices
int Table[8][256][2];

// A5_GMR_2
void F(bool K[8][8], bool t, unsigned char c, bool p[8], bool O0[8], bool O1[4]);
void G(bool O0[8], bool O1[4], bool S0[8], bool O0_[6], bool O1_[6]);
void H(bool O0_[6], bool O1_[6], bool t, bool Z[8]);
void A5_GMR_2(bool K[8][8], bool IS[8][8], bool keystream[15][8]);

// Inverse of each component
void H_inv(bool Z[8], bool t, bool O0_[16][6], bool O1_[16][6]);
void G_inv(bool O0_[6], bool O1_[6], bool S0[8], bool O0[8], bool O1[4]);
void F_inv(bool O0[8], bool O1[4], bool p[8], bool t, bool Kc[16][8], bool Kt[16][8], int tau_a[16]);

// Initialization
void base();
void Keystream_Generate(int option);

// Cryptanalysis
void Table_Generate();

void dfs(int v, vector<vector<int>>& e, vector<bool>& visited, vector<set<int>>& reachable) {
    visited[v] = true;
    reachable[v].insert(v);

    for (auto u : e[v]) {
        if (!visited[u]) dfs(u, e, visited, reachable);
        for (auto elem : reachable[u]) reachable[v].insert(elem);
    }
}
bool e_matrix[2048][2048] = { false };
// main
int main(int argc, char* argv[]) {
    clock_t start, end;
    int experiment_num = 1;
    int num_of_known_keystream;
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <num_of_known_keystream>" << endl;
        cout << "num_of_known_keystream must be between 1 and 5" << endl;
        cout << "Example: " << argv[0] << " 3" << endl;
        return 1;
    }

    num_of_known_keystream = atoi(argv[1]);
    if (num_of_known_keystream < 1 || num_of_known_keystream > 5) {
        cout << "Error: num_of_known_keystream must be between 1 and 5" << endl;
        cout << "Usage: " << argv[0] << " <num_of_known_keystream>" << endl;
        cout << "Example: " << argv[0] << " 3" << endl;
        return 1;
    }

    cout << "Using " << num_of_known_keystream << " known keystream frames" << endl;
    base();

    std::random_device rd;
    unsigned int seed = static_cast<unsigned int>(rd());
    srand(seed);
    std::cout << "Random seed: " << seed << std::endl;

    int tot_ind = 8 * 256;

    Keystream_Generate(0);
    Table_Generate();
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 256; j++) {
            int nb = Table[i][j][1];
            int ni = Table[i][j][0];

            e_matrix[i * 256 + j][nb * 256 + ni] = true;

        }
    }

    for (int k = 0; k < num_of_known_keystream - 1; k++) {
        Keystream_Generate(1);
        Table_Generate();
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 256; j++) {
                int nb = Table[i][j][1];
                int ni = Table[i][j][0];

                e_matrix[i * 256 + j][nb * 256 + ni] = true;

            }
        }
    }

    vector<vector<int>> e(2048);
    for (int i = 0; i < 2048; i++) {
        for (int j = 0; j < 2048; j++) {
            if (e_matrix[i][j]) e[i].push_back(j);
        }
    }

    vector<bool> visited(2048);
    vector<set<int>> reachable(2048);
    for (int i = 0; i < 2048; i++) {
        if (visited[i]) continue;
        dfs(i, e, visited, reachable);
    }

    vector<vector<int>> r(2048, vector<int>(8, -1));
    vector<bool> black(2048, true);
    for (int i = 0; i < 2048; i++) {
        for (auto elem : reachable[i]) {
            int x = elem / 256, y = elem % 256;
            if (r[i][x] != -1) {
                black[i] = false;
                break;
            }
            r[i][x] = y;
        }
    }

    vector<int> state(8, -1);
    vector<int> value(8, -1);
    stack<int> stk;

    int count = 0;

    int curi = 0, curj = 0;
    while (true) {
        while (curj == 256) {
            if (stk.empty()) break;
            curi = stk.top(); stk.pop();
            curj = value[curi];
            for (int i = 0; i < 8; i++) {
                if (state[i] == curi) {
                    value[i] = -1;
                    state[i] = -1;
                }
            }
            curj++;
        }
        if (curi == 0 && curj == 256) break;
        int idx = curi * 256 + curj;
        if (!black[idx]) {
            curj++;
            continue;
        }
        bool chk = true;
        for (int i = 0; i < 8; i++) {
            if (r[idx][i] != -1 && value[i] != -1 && r[idx][i] != value[i]) chk = false;
        }
        if (!chk) curj++;
        else {
            for (int i = 0; i < 8; i++) {
                if (state[i] == -1 && r[idx][i] != -1) {
                    value[i] = r[idx][i];
                    state[i] = curi;
                }
            }
            stk.push(curi);
            bool chk2 = true;
            for (int i = 0; i < 8; i++) {
                if (state[i] == -1) {
                    chk2 = false;
                    curi = i;
                    curj = 0;
                    break;
                }
            }
            if (chk2) {
                count++;
                for (int i = 0; i < 8; i++) {
                    if (state[i] == curi) {
                        state[i] = 0;
                        value[i] = -1;
                    }
                }
                stk.pop();
                curj++;
            }
        }
    }

    cout << "count: " << count << '\n';
}



// A5_GMR_2
void F(bool K[8][8], bool t, unsigned char c, bool p[8], bool O0[8], bool O1[4]) {
    int i, a;
    if (t == 0)
        a = (K[c][3] ^ p[3]) * 8 + (K[c][2] ^ p[2]) * 4 + (K[c][1] ^ p[1]) * 2 + (K[c][0] ^ p[0]);
    else
        a = (K[c][7] ^ p[7]) * 8 + (K[c][6] ^ p[6]) * 4 + (K[c][5] ^ p[5]) * 2 + (K[c][4] ^ p[4]);

    int temp = tau2[tau1[a]];
    for (i = 0; i < (8 - temp); i++)
        O0[i] = K[tau1[a]][i + temp];
    for (i = (8 - temp); i < 8; i++)
        O0[i] = K[tau1[a]][i + temp - 8];

    for (i = 0; i < 4; i++)
        O1[i] = K[c][i + 4] ^ p[i + 4] ^ K[c][i] ^ p[i];
}
void G(bool O0[8], bool O1[4], bool S0[8], bool O0_[6], bool O1_[6]) {
    O0_[5] = O0[7] ^ O0[4] ^ S0[5];
    O0_[4] = O0[7] ^ O0[6] ^ O0[4] ^ S0[7];
    O0_[3] = O0[7] ^ S0[4];
    O0_[2] = O0[5] ^ S0[6];
    O0_[1] = O1[3] ^ O1[1] ^ O1[0];
    O0_[0] = O1[3] ^ O1[0];

    O1_[5] = O0[3] ^ O0[0] ^ S0[1];
    O1_[4] = O0[3] ^ O0[2] ^ O0[0] ^ S0[3];
    O1_[3] = O0[3] ^ S0[0];
    O1_[2] = O0[1] ^ S0[2];
    O1_[1] = O1[2];
    O1_[0] = O1[0];
}
void H(bool O0_[6], bool O1_[6], bool t, bool Z[8]) {
    int temp;
    if (t == 0) {
        temp = S2[16 * (O1_[1] * 2 + O1_[0]) + (O1_[5] * 8 + O1_[4] * 4 + O1_[3] * 2 + O1_[2])];
        Z[7] = (temp >> 3) & 1; Z[6] = (temp >> 2) & 1; Z[5] = (temp >> 1) & 1; Z[4] = temp & 1;
        temp = S6[16 * (O0_[1] * 2 + O0_[0]) + (O0_[5] * 8 + O0_[4] * 4 + O0_[3] * 2 + O0_[2])];
        Z[3] = (temp >> 3) & 1; Z[2] = (temp >> 2) & 1; Z[1] = (temp >> 1) & 1; Z[0] = temp & 1;
    }
    else {
        temp = S2[16 * (O0_[1] * 2 + O0_[0]) + (O0_[5] * 8 + O0_[4] * 4 + O0_[3] * 2 + O0_[2])];
        Z[7] = (temp >> 3) & 1; Z[6] = (temp >> 2) & 1; Z[5] = (temp >> 1) & 1; Z[4] = temp & 1;
        temp = S6[16 * (O1_[1] * 2 + O1_[0]) + (O1_[5] * 8 + O1_[4] * 4 + O1_[3] * 2 + O1_[2])];
        Z[3] = (temp >> 3) & 1; Z[2] = (temp >> 2) & 1; Z[1] = (temp >> 1) & 1; Z[0] = temp & 1;
    }
}
void A5_GMR_2(bool K[8][8], bool IS[8][8], bool keystream[15][8]) {
    bool O0[8] = { 0, }, O1[4] = { 0, }, O0_[6] = { 0, }, O1_[6] = { 0, }, p[8] = { 0, }, t = 0;
    int i, j, k;
    unsigned char c = 0;

    for (i = 0; i < 15; i++) {
        for (j = 0; j < 8; j++)
            p[j] = IS[7][j];

        F(K, t, c, p, O0, O1);
        G(O0, O1, IS[0], O0_, O1_);
        H(O0_, O1_, t, keystream[i]);

        c = (c + 1) % 8;
        t ^= 1;
        for (j = 0; j < 7; j++) {
            for (k = 0; k < 8; k++)
                IS[j][k] = IS[j + 1][k];
        }
        for (j = 0; j < 8; j++)
            IS[7][j] = keystream[i][j];
    }
}

// Inverse of each component
void H_inv(bool Z[8], bool t, bool O0_[16][6], bool O1_[16][6]) {
    int i;
    bool temp[6];
    if (t == 0) {
        for (i = 0; i < 6; i++) {
            temp[i] = (S2_inv[Z[7] * 8 + Z[6] * 4 + Z[5] * 2 + Z[4]][0] >> i) & 1;
            O1_[0][i] = temp[i];
            O1_[1][i] = temp[i];
            O1_[2][i] = temp[i];
            O1_[3][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S2_inv[Z[7] * 8 + Z[6] * 4 + Z[5] * 2 + Z[4]][1] >> i) & 1;
            O1_[4][i] = temp[i];
            O1_[5][i] = temp[i];
            O1_[6][i] = temp[i];
            O1_[7][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S2_inv[Z[7] * 8 + Z[6] * 4 + Z[5] * 2 + Z[4]][2] >> i) & 1;
            O1_[8][i] = temp[i];
            O1_[9][i] = temp[i];
            O1_[10][i] = temp[i];
            O1_[11][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S2_inv[Z[7] * 8 + Z[6] * 4 + Z[5] * 2 + Z[4]][3] >> i) & 1;
            O1_[12][i] = temp[i];
            O1_[13][i] = temp[i];
            O1_[14][i] = temp[i];
            O1_[15][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S6_inv[Z[3] * 8 + Z[2] * 4 + Z[1] * 2 + Z[0]][0] >> i) & 1;
            O0_[0][i] = temp[i];
            O0_[4][i] = temp[i];
            O0_[8][i] = temp[i];
            O0_[12][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S6_inv[Z[3] * 8 + Z[2] * 4 + Z[1] * 2 + Z[0]][1] >> i) & 1;
            O0_[1][i] = temp[i];
            O0_[5][i] = temp[i];
            O0_[9][i] = temp[i];
            O0_[13][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S6_inv[Z[3] * 8 + Z[2] * 4 + Z[1] * 2 + Z[0]][2] >> i) & 1;
            O0_[2][i] = temp[i];
            O0_[6][i] = temp[i];
            O0_[10][i] = temp[i];
            O0_[14][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S6_inv[Z[3] * 8 + Z[2] * 4 + Z[1] * 2 + Z[0]][3] >> i) & 1;
            O0_[3][i] = temp[i];
            O0_[7][i] = temp[i];
            O0_[11][i] = temp[i];
            O0_[15][i] = temp[i];
        }
    }
    else {
        for (i = 0; i < 6; i++) {
            temp[i] = (S2_inv[Z[7] * 8 + Z[6] * 4 + Z[5] * 2 + Z[4]][0] >> i) & 1;
            O0_[0][i] = temp[i];
            O0_[1][i] = temp[i];
            O0_[2][i] = temp[i];
            O0_[3][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S2_inv[Z[7] * 8 + Z[6] * 4 + Z[5] * 2 + Z[4]][1] >> i) & 1;
            O0_[4][i] = temp[i];
            O0_[5][i] = temp[i];
            O0_[6][i] = temp[i];
            O0_[7][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S2_inv[Z[7] * 8 + Z[6] * 4 + Z[5] * 2 + Z[4]][2] >> i) & 1;
            O0_[8][i] = temp[i];
            O0_[9][i] = temp[i];
            O0_[10][i] = temp[i];
            O0_[11][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S2_inv[Z[7] * 8 + Z[6] * 4 + Z[5] * 2 + Z[4]][3] >> i) & 1;
            O0_[12][i] = temp[i];
            O0_[13][i] = temp[i];
            O0_[14][i] = temp[i];
            O0_[15][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S6_inv[Z[3] * 8 + Z[2] * 4 + Z[1] * 2 + Z[0]][0] >> i) & 1;
            O1_[0][i] = temp[i];
            O1_[4][i] = temp[i];
            O1_[8][i] = temp[i];
            O1_[12][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S6_inv[Z[3] * 8 + Z[2] * 4 + Z[1] * 2 + Z[0]][1] >> i) & 1;
            O1_[1][i] = temp[i];
            O1_[5][i] = temp[i];
            O1_[9][i] = temp[i];
            O1_[13][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S6_inv[Z[3] * 8 + Z[2] * 4 + Z[1] * 2 + Z[0]][2] >> i) & 1;
            O1_[2][i] = temp[i];
            O1_[6][i] = temp[i];
            O1_[10][i] = temp[i];
            O1_[14][i] = temp[i];
        }
        for (int i = 0; i < 6; i++) {
            temp[i] = (S6_inv[Z[3] * 8 + Z[2] * 4 + Z[1] * 2 + Z[0]][3] >> i) & 1;
            O1_[3][i] = temp[i];
            O1_[7][i] = temp[i];
            O1_[11][i] = temp[i];
            O1_[15][i] = temp[i];
        }
    }
}
void G_inv(bool O0_[6], bool O1_[6], bool S0[8], bool O0[8], bool O1[4]) {
    O0[7] = O0_[3] ^ S0[4];
    O0[6] = O0_[5] ^ O0_[4] ^ S0[7] ^ S0[5];
    O0[5] = O0_[2] ^ S0[6];
    O0[4] = O0_[5] ^ O0_[3] ^ S0[5] ^ S0[4];
    O0[3] = O1_[3] ^ S0[0];
    O0[2] = O1_[5] ^ O1_[4] ^ S0[3] ^ S0[1];
    O0[1] = O1_[2] ^ S0[2];
    O0[0] = O1_[5] ^ O1_[3] ^ S0[1] ^ S0[0];

    O1[3] = O0_[0] ^ O1_[0];
    O1[2] = O1_[1];
    O1[1] = O0_[1] ^ O0_[0];
    O1[0] = O1_[0];
}
void F_inv(bool O0[8], bool O1[4], bool p[8], bool t, bool Kc[16][8], bool Kt[16][8], int tau_a[16]) {
    if ((O1[3] ^ p[7] ^ p[3]) == 0) {
        Kc[0][7] = 0;
        Kc[0][3] = 0;
        Kc[1][7] = 0;
        Kc[1][3] = 0;
        Kc[2][7] = 0;
        Kc[2][3] = 0;
        Kc[3][7] = 0;
        Kc[3][3] = 0;
        Kc[4][7] = 0;
        Kc[4][3] = 0;
        Kc[5][7] = 0;
        Kc[5][3] = 0;
        Kc[6][7] = 0;
        Kc[6][3] = 0;
        Kc[7][7] = 0;
        Kc[7][3] = 0;

        Kc[8][7] = 1;
        Kc[8][3] = 1;
        Kc[9][7] = 1;
        Kc[9][3] = 1;
        Kc[10][7] = 1;
        Kc[10][3] = 1;
        Kc[11][7] = 1;
        Kc[11][3] = 1;
        Kc[12][7] = 1;
        Kc[12][3] = 1;
        Kc[13][7] = 1;
        Kc[13][3] = 1;
        Kc[14][7] = 1;
        Kc[14][3] = 1;
        Kc[15][7] = 1;
        Kc[15][3] = 1;
    }
    else {
        Kc[0][7] = 1;
        Kc[0][3] = 0;
        Kc[1][7] = 1;
        Kc[1][3] = 0;
        Kc[2][7] = 1;
        Kc[2][3] = 0;
        Kc[3][7] = 1;
        Kc[3][3] = 0;
        Kc[4][7] = 1;
        Kc[4][3] = 0;
        Kc[5][7] = 1;
        Kc[5][3] = 0;
        Kc[6][7] = 1;
        Kc[6][3] = 0;
        Kc[7][7] = 1;
        Kc[7][3] = 0;

        Kc[8][7] = 0;
        Kc[8][3] = 1;
        Kc[9][7] = 0;
        Kc[9][3] = 1;
        Kc[10][7] = 0;
        Kc[10][3] = 1;
        Kc[11][7] = 0;
        Kc[11][3] = 1;
        Kc[12][7] = 0;
        Kc[12][3] = 1;
        Kc[13][7] = 0;
        Kc[13][3] = 1;
        Kc[14][7] = 0;
        Kc[14][3] = 1;
        Kc[15][7] = 0;
        Kc[15][3] = 1;
    }
    if ((O1[2] ^ p[6] ^ p[2]) == 0) {
        Kc[0][6] = 0;
        Kc[0][2] = 0;
        Kc[1][6] = 0;
        Kc[1][2] = 0;
        Kc[2][6] = 0;
        Kc[2][2] = 0;
        Kc[3][6] = 0;
        Kc[3][2] = 0;
        Kc[8][6] = 0;
        Kc[8][2] = 0;
        Kc[9][6] = 0;
        Kc[9][2] = 0;
        Kc[10][6] = 0;
        Kc[10][2] = 0;
        Kc[11][6] = 0;
        Kc[11][2] = 0;

        Kc[4][6] = 1;
        Kc[4][2] = 1;
        Kc[5][6] = 1;
        Kc[5][2] = 1;
        Kc[6][6] = 1;
        Kc[6][2] = 1;
        Kc[7][6] = 1;
        Kc[7][2] = 1;
        Kc[12][6] = 1;
        Kc[12][2] = 1;
        Kc[13][6] = 1;
        Kc[13][2] = 1;
        Kc[14][6] = 1;
        Kc[14][2] = 1;
        Kc[15][6] = 1;
        Kc[15][2] = 1;
    }
    else {
        Kc[0][6] = 1;
        Kc[0][2] = 0;
        Kc[1][6] = 1;
        Kc[1][2] = 0;
        Kc[2][6] = 1;
        Kc[2][2] = 0;
        Kc[3][6] = 1;
        Kc[3][2] = 0;
        Kc[8][6] = 1;
        Kc[8][2] = 0;
        Kc[9][6] = 1;
        Kc[9][2] = 0;
        Kc[10][6] = 1;
        Kc[10][2] = 0;
        Kc[11][6] = 1;
        Kc[11][2] = 0;

        Kc[4][6] = 0;
        Kc[4][2] = 1;
        Kc[5][6] = 0;
        Kc[5][2] = 1;
        Kc[6][6] = 0;
        Kc[6][2] = 1;
        Kc[7][6] = 0;
        Kc[7][2] = 1;
        Kc[12][6] = 0;
        Kc[12][2] = 1;
        Kc[13][6] = 0;
        Kc[13][2] = 1;
        Kc[14][6] = 0;
        Kc[14][2] = 1;
        Kc[15][6] = 0;
        Kc[15][2] = 1;
    }
    if ((O1[1] ^ p[5] ^ p[1]) == 0) {
        Kc[0][5] = 0;
        Kc[0][1] = 0;
        Kc[1][5] = 0;
        Kc[1][1] = 0;
        Kc[4][5] = 0;
        Kc[4][1] = 0;
        Kc[5][5] = 0;
        Kc[5][1] = 0;
        Kc[8][5] = 0;
        Kc[8][1] = 0;
        Kc[9][5] = 0;
        Kc[9][1] = 0;
        Kc[12][5] = 0;
        Kc[12][1] = 0;
        Kc[13][5] = 0;
        Kc[13][1] = 0;

        Kc[2][5] = 1;
        Kc[2][1] = 1;
        Kc[3][5] = 1;
        Kc[3][1] = 1;
        Kc[6][5] = 1;
        Kc[6][1] = 1;
        Kc[7][5] = 1;
        Kc[7][1] = 1;
        Kc[10][5] = 1;
        Kc[10][1] = 1;
        Kc[11][5] = 1;
        Kc[11][1] = 1;
        Kc[14][5] = 1;
        Kc[14][1] = 1;
        Kc[15][5] = 1;
        Kc[15][1] = 1;
    }
    else {
        Kc[0][5] = 1;
        Kc[0][1] = 0;
        Kc[1][5] = 1;
        Kc[1][1] = 0;
        Kc[4][5] = 1;
        Kc[4][1] = 0;
        Kc[5][5] = 1;
        Kc[5][1] = 0;
        Kc[8][5] = 1;
        Kc[8][1] = 0;
        Kc[9][5] = 1;
        Kc[9][1] = 0;
        Kc[12][5] = 1;
        Kc[12][1] = 0;
        Kc[13][5] = 1;
        Kc[13][1] = 0;

        Kc[2][5] = 0;
        Kc[2][1] = 1;
        Kc[3][5] = 0;
        Kc[3][1] = 1;
        Kc[6][5] = 0;
        Kc[6][1] = 1;
        Kc[7][5] = 0;
        Kc[7][1] = 1;
        Kc[10][5] = 0;
        Kc[10][1] = 1;
        Kc[11][5] = 0;
        Kc[11][1] = 1;
        Kc[14][5] = 0;
        Kc[14][1] = 1;
        Kc[15][5] = 0;
        Kc[15][1] = 1;
    }
    if ((O1[0] ^ p[4] ^ p[0]) == 0) {
        Kc[0][4] = 0;
        Kc[0][0] = 0;
        Kc[2][4] = 0;
        Kc[2][0] = 0;
        Kc[4][4] = 0;
        Kc[4][0] = 0;
        Kc[6][4] = 0;
        Kc[6][0] = 0;
        Kc[8][4] = 0;
        Kc[8][0] = 0;
        Kc[10][4] = 0;
        Kc[10][0] = 0;
        Kc[12][4] = 0;
        Kc[12][0] = 0;
        Kc[14][4] = 0;
        Kc[14][0] = 0;

        Kc[1][4] = 1;
        Kc[1][0] = 1;
        Kc[3][4] = 1;
        Kc[3][0] = 1;
        Kc[5][4] = 1;
        Kc[5][0] = 1;
        Kc[7][4] = 1;
        Kc[7][0] = 1;
        Kc[9][4] = 1;
        Kc[9][0] = 1;
        Kc[11][4] = 1;
        Kc[11][0] = 1;
        Kc[13][4] = 1;
        Kc[13][0] = 1;
        Kc[15][4] = 1;
        Kc[15][0] = 1;
    }
    else {
        Kc[0][4] = 1;
        Kc[0][0] = 0;
        Kc[2][4] = 1;
        Kc[2][0] = 0;
        Kc[4][4] = 1;
        Kc[4][0] = 0;
        Kc[6][4] = 1;
        Kc[6][0] = 0;
        Kc[8][4] = 1;
        Kc[8][0] = 0;
        Kc[10][4] = 1;
        Kc[10][0] = 0;
        Kc[12][4] = 1;
        Kc[12][0] = 0;
        Kc[14][4] = 1;
        Kc[14][0] = 0;

        Kc[1][4] = 0;
        Kc[1][0] = 1;
        Kc[3][4] = 0;
        Kc[3][0] = 1;
        Kc[5][4] = 0;
        Kc[5][0] = 1;
        Kc[7][4] = 0;
        Kc[7][0] = 1;
        Kc[9][4] = 0;
        Kc[9][0] = 1;
        Kc[11][4] = 0;
        Kc[11][0] = 1;
        Kc[13][4] = 0;
        Kc[13][0] = 1;
        Kc[15][4] = 0;
        Kc[15][0] = 1;
    }
    int i, j, a, temp;
    if (t == 0) {
        for (i = 0; i < 16; i++) {
            a = (Kc[i][3] ^ p[3]) * 8 + (Kc[i][2] ^ p[2]) * 4 + (Kc[i][1] ^ p[1]) * 2 + (Kc[i][0] ^ p[0]);
            tau_a[i] = tau1[a];
            temp = tau2[tau_a[i]];
            for (j = 0; j < temp; j++)
                Kt[i][j] = O0[8 + j - temp];
            for (j = temp; j < 8; j++)
                Kt[i][j] = O0[j - temp];
        }
    }
    else {
        for (i = 0; i < 16; i++) {
            a = (Kc[i][7] ^ p[7]) * 8 + (Kc[i][6] ^ p[6]) * 4 + (Kc[i][5] ^ p[5]) * 2 + (Kc[i][4] ^ p[4]);
            tau_a[i] = tau1[a];
            temp = tau2[tau_a[i]];
            for (j = 0; j < temp; j++)
                Kt[i][j] = O0[8 + j - temp];
            for (j = temp; j < 8; j++)
                Kt[i][j] = O0[j - temp];
        }
    }
}

// All vertices in List_7 is black
void base() {
    for (int i = 0; i < 256; i++) {
        Table[7][i][0] = i;
        Table[7][i][1] = 7;
    }
}

// option 0-> Create a new session key and initial state
// option 1-> Initialize vertices that became black after pre-filtration with first keystream frame to white
void Keystream_Generate(int option) {
    for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) {
        if (!option) K[i][j] = rand() % 2;
        IS[i][j] = rand() % 2;
        IS_tmp[i][j] = IS[i][j];
    }
    if (!option) {
        for (int i = 0; i < 8; i++) {
            int temp = i * 256;
            for (int j = 0; j < 8; j++) temp += K[i][j] * (1 << (7 - j));
            cout << temp << ' ';
        }
        cout << '\n';
        for (int i = 0; i < 8; i++) {
            int temp = i * 256;
            for (int j = 0; j < 8; j++) temp += K[i][j] * (1 << (j));
            cout << temp << ' ';
        }
        cout << '\n';
    }
    A5_GMR_2(K, IS_tmp, known_keystream);
}
// Table Generate Phase
void Table_Generate() {
    int tau_a[16];
    bool O0_[16][6], O1_[16][6], O0[8], O1[4], Kc[16][8], Kt[16][8];

    for (int i = 8; i < 15; i++) {
        H_inv(known_keystream[i], i % 2, O0_, O1_);
        for (int j = 0; j < 16; j++) {
            G_inv(O0_[j], O1_[j], known_keystream[i - 8], O0, O1);
            F_inv(O0, O1, known_keystream[i - 1], i % 2, Kc, Kt, tau_a);
            for (int k = 0; k < 16; k++) {
                int row = Kc[k][7] * 128 + Kc[k][6] * 64 + Kc[k][5] * 32 + Kc[k][4] * 16 + Kc[k][3] * 8 + Kc[k][2] * 4 + Kc[k][1] * 2 + Kc[k][0];
                Table[i - 8][row][0] = Kt[k][7] * 128 + Kt[k][6] * 64 + Kt[k][5] * 32 + Kt[k][4] * 16 + Kt[k][3] * 8 + Kt[k][2] * 4 + Kt[k][1] * 2 + Kt[k][0];
                if ((i % 2) == 0)
                    Table[i - 8][row][1] = tau1[(Kc[k][3] ^ known_keystream[i - 1][3]) * 8 + (Kc[k][2] ^ known_keystream[i - 1][2]) * 4 + (Kc[k][1] ^ known_keystream[i - 1][1]) * 2 + (Kc[k][0] ^ known_keystream[i - 1][0])];
                else
                    Table[i - 8][row][1] = tau1[(Kc[k][7] ^ known_keystream[i - 1][7]) * 8 + (Kc[k][6] ^ known_keystream[i - 1][6]) * 4 + (Kc[k][5] ^ known_keystream[i - 1][5]) * 2 + (Kc[k][4] ^ known_keystream[i - 1][4])];
            }
        }
    }
}