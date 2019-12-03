#include<iostream>
#include<vector>
#include<cmath>
#include<map>
#include<climits>
#include<algorithm>
using namespace std;


/*

Basically, I changed the features because I felt that they didn't encompass the actual scoring criteria
(determined by #townhalls and the #soldiers remaining) very well. I'll explain later how the following features encompass
the criteria. Another reason was to incorporate dynamic change in the weights of the features, which will help the program
play aggressively or defensively (offense and defense have been given context below) depending on the current state of
the board.

*/

int m, n, timelimit = 0;
char player;
string player_key;

struct board_state_worth
{
    //offense-based features
    int avg_dist_off_player; //sum of the distances of your soldiers from your edge of the board.
                             //The higher this value, the closer your soldiers are to the enemy's townhalls. Value boosted by the presence of your cannons on the board.
                             //Represented by AOP in the heuristic function given in the next section

    int avg_dist_off_enemy; //sum of the distances of enemy soldiers from the enemy's edge of the board.
                            //The higher this value. the closer the enemy's soldiers are to your townhalls. Value boosted by the presence of enemy's cannons on the board.
                            //Represented by AOE

    //defense-based features
    int avg_dist_def_player; //Same as avg_dist_off_player, but changes when there are cannons on the board. Generally, represents the how far your soldiers are from your edge of the board.
                             //The larger the value, the less protection for your townhalls. Presence of your cannons decreases this value.
                             //Represented by ADP

    int avg_dist_def_enemy; //Same as avg_dist_off_enemy, but changes when there are cannons on the board. Generally, represents the how far the enemy soldiers are from their edge of the board.
                            //The larger the value, the less protection for your townhalls. Presence of enemy cannons decreases this value.
                            //Represented by ADE


    //weight contributors for the offense and defense
    int townhalls_player; //No. of your townhalls on the board. This value acts as one of the weight parameters for the offense and defense-based features.
                          //The idea is to play more aggressively i.e. boost your offensive parameters when you have more townhalls than your enemy, and (try to)play defensively when the enemy has more.
                          //Playing defensively may include going backwards, towards your townhalls and according to the rules, it's not easy to do.
                          //However, if circumstances allow it, it might be a good move to make. That will be checked by our program.
                          //Represented by TP

    int townhalls_enemy; //Similar to "townhalls_player"
                         //Represented by TE

    int soldiers_player; //No. of your soldiers on the board. This value acts as one of the weight parameters for the offense and defense-based features,
                         //but should have less weight than the no. of townhalls, according to how the final score of the match is calculated. It affects the aggressive/defensive play
                         //in the same way as the no. of townhalls, but again, to a lesser extent.
                         //Represented by SP

    int soldiers_enemy; //Similar to "soldiers_player"
                        //Represented by SE
};

/*

Note: - The "avg" in the first four features is misleading; they represent sum.

One possible heuristic function is given below:

F(x) = 4*(AOP(x) - AOE(x)) - 2*(((12-SP+10*(4-TP))*m + ADP(x)) - (((12-SE + 10*(4-TE))*m) + ADE(x)))
       -------------------   -----------------------------------------------------------------------
                (1)                                           (2)

F(x) needs to be maximized for us, and minimized for the enemy.

^Assuming 'x' is the current state of the board.

F(x) will initially favour offensive play, but as the number of townhalls and soldiers for the the player on the current turn,
decreases relative to its enemy's, it will start favouring defensive play, since expression (2) will start overpowering (1).

Simply put, "offense" and "defense" based features use the actual criteria for scoring (#townhalls and #soldiers) as their weights.
Our goal, ideally, is to take out two of our enemy's townhalls (if we have the resources to) and take out as many enemy soldiers
as we can, at the same time saving as many of our own soldiers and townhalls as possible. So, "offense" and "defense" encompass the two opposite aspects of our
goal, and when to play offensively or defensively is taken care of by the parameters used in the actual scoring criteria for the match.

My suggestion to you is that you try running the program and see how the values of the different features change as the state of the board changes.
After that, you can come up with your own heuristic function which best evaluates the current state of the board (you can judge the current state
with your intuition). I think that the actual score (Townhall margin + Army margin/100) needs to be kept in mind while creating the heuristic function, though.

Note: - I haven't tested the above heuristic function at all. It's just to give you an idea of what I'm trying to do.

*/

struct position
{
    int r;
    int c;
};

struct soldier_info
{
    position pos;
    char label;
};

struct cannon_info
{
    position start;
    position en;
    int slope;
    char soldier;
};

struct next_move_info
{
    vector<soldier_info> soldiers;
    vector<cannon_info> cannons;
};

struct found_direction
{
    bool NORTH;
    bool WEST;
    bool NORTHWEST;
    bool NORTHEAST;
};


board_state_worth update_board_state_worth(board_state_worth b, char** board, next_move_info nm_info)
{
    bool visited[m][n];
    for(int i = 0; i < m; i++)
        for(int j = 0; j < n; j++)visited[i][j] = false;
    for(int i = 0; i < nm_info.cannons.size(); i++)
    {
        int k, l;
        if(nm_info.cannons[i].slope == 0)
            continue;
        if(nm_info.cannons[i].slope == 2)
        {
            k = 1;
            l = 0;
        }
        if(nm_info.cannons[i].slope == 1)
        {
            k = 1;
            l = -1;
        }
        if(nm_info.cannons[i].slope == -1)
        {
            k = 1;
            l = 1;
        }
        position start = nm_info.cannons[i].start;
        position en = nm_info.cannons[i].en;
        if(nm_info.cannons[i].soldier == player)
        {
            b.avg_dist_off_player += (en.r - start.r+1)*((player == '2')?en.r:m-start.r-1);
            b.avg_dist_def_player += (en.r - start.r+1)*((player == '2')?start.r:m-en.r-1);
        }
        else
        {
            b.avg_dist_off_enemy += (en.r - start.r+1)*((player == '2')?m-start.r-1:en.r);
            b.avg_dist_def_enemy += (en.r - start.r+1)*((player == '2')?m-en.r-1:start.r);
        }
        while(start.r != en.r || start.c != en.c)
        {
            visited[start.r][start.c] = true;
            start.r += k;
            start.c += l;
        }
        visited[start.r][start.c] = true;
    }
    int avg_dist_player = 0, avg_dist_enemy = 0;
    for(int i = 0; i < m; i++)
    {
        for(int j = 0; j < n; j++)
        {
            if(board[i][j] == '2' && !visited[i][j])
            {
                if(player == '2')
                {
                    avg_dist_player += i;
                }
                else
                {
                    avg_dist_enemy += i;
                }
            }
            else if(board[i][j] == '1' && !visited[i][j])
            {
                if(player == '1')
                {
                    avg_dist_player += m - i-1;
                }
                else
                {
                    avg_dist_enemy += m - i-1;
                }
            }
            else if(board[i][j] == 'a')b.townhalls_player++;
            else if(board[i][j] == 'b')b.townhalls_enemy++;
        }
    }
    b.avg_dist_off_player += avg_dist_player;
    b.avg_dist_def_player += avg_dist_player;
    b.avg_dist_off_enemy += avg_dist_enemy;
    b.avg_dist_def_enemy += avg_dist_enemy;
    return b;
}

next_move_info update_cannons(char** board, next_move_info nm_info)
{
    char t_board[m][n];
    found_direction p_board[m][n];
    for(int i = 0; i < m; i++)
    {
        for(int j = 0; j < n; j++)
        {
            t_board[i][j] = board[i][j];
            p_board[i][j].NORTH = false;
            p_board[i][j].WEST = false;
            p_board[i][j].NORTHWEST = false;
            p_board[i][j].NORTHEAST = false;
        }
    }

    for(int i = 0; i < m; i++)
    {
        for(int j = 0; j < n; j++)
        {
            if(t_board[i][j] == '1' || t_board[i][j] == '2')
            {
                if(!p_board[i][j].NORTH)
                {
                    int k = 0;
                    char curr = t_board[i][j];
                    while(t_board[i+k][j] == curr && i+k < m)
                        p_board[i+(k++)][j].NORTH = true;
                    if(k >= 3)
                    {

                        cannon_info c;
                        c.start.r = i;
                        c.start.c = j;
                        c.en.r = i+k-1;
                        c.en.c = j;
                        c.slope = 2; //represents infinite slope
                        c.soldier = curr;
                        nm_info.cannons.push_back(c);
                    }
                }


                if(!p_board[i][j].WEST)
                {
                    int k = 0;
                    char curr = t_board[i][j];
                    while(t_board[i][j+k] == curr && j+k < n)
                        p_board[i][j+(k++)].WEST = true;
                    if(k >= 3)
                    {
                        cannon_info c;
                        c.start.r = i;
                        c.start.c = j;
                        c.en.r = i;
                        c.en.c = j+k-1;
                        c.slope = 0;
                        c.soldier = curr;
                        nm_info.cannons.push_back(c);
                    }
                }


                if(!p_board[i][j].NORTHWEST)
                {
                    int k = 0;
                    char curr = t_board[i][j];
                    while(t_board[i+k][j+k] == curr && i+k < m && j+k < n)
                    {
                        p_board[i+k][j+k].NORTHWEST = true;
                        k++;
                    }
                    if(k >= 3)
                    {
                        cannon_info c;
                        c.start.r = i;
                        c.start.c = j;
                        c.en.r = i+k-1;
                        c.en.c = j+k-1;
                        c.slope = -1;
                        c.soldier = curr;
                        nm_info.cannons.push_back(c);
                    }
                }


                if(!p_board[i][j].NORTHEAST)
                {
                    int k = 0;
                    char curr = t_board[i][j];
                    while(t_board[i+k][j-k] == curr && i+k < m && j-k >= 0)
                    {
                        p_board[i+k][j-k].NORTHEAST = true;
                        k++;
                    }
                    if(k >= 3)
                    {
                        cannon_info c;
                        c.start.r = i;
                        c.start.c = j;
                        c.en.r = i+k-1;
                        c.en.c = j-k+1;
                        c.slope = 1;
                        c.soldier = curr;
                        nm_info.cannons.push_back(c);
                    }
                }
            }
        }
    }

    return nm_info;
}

int count_cannons(next_move_info nm_info, char soldier_type)
{
    vector<cannon_info> cf = nm_info.cannons;
    int cnt = 0;
    for(int i = 0; i < cf.size(); i++)
    {
        if(cf[i].soldier == soldier_type)
        {
            if(cf[i].slope)cnt++;
        }
    }
    return cnt;
}

board_state_worth update_soldiers(char** board, board_state_worth b, next_move_info *nm_info)
{
    for(int i = 0; i < m; i++)
    {
        for(int j = 0; j < n; j++)
        {
            if(board[i][j] == '1' || board[i][j] == '2')
            {
                soldier_info sol;
                position p;
                p.r = i;
                p.c = j;
                sol.pos = p;
                sol.label = board[i][j];
                nm_info->soldiers.push_back(sol);
                if(player == board[i][j])b.soldiers_player++;
                else b.soldiers_enemy++;
            }
        }
    }
    return b;
}

board_state_worth initialize_board_state_worth()
{
    board_state_worth b;
    b.avg_dist_off_player = 0;
    b.avg_dist_def_player = 0;
    b.avg_dist_off_enemy = 0;
    b.avg_dist_def_enemy = 0;
    b.townhalls_player = 0;
    b.townhalls_enemy = 0;
    b.soldiers_player = 0;
    b.soldiers_enemy = 0;
    return b;
}


enum movetype{MOVE, BOMB, DUMMY};

struct direction
{
    int r;
    int c;
};

struct c_direction
{
    int r;
    int c;
};
direction dir[8] = {{1, 0}, {1, 1}, {1, -1}, {0, 1}, {0, -1}, {-2, 0}, {-2, -2}, {-2, 2}};
c_direction bomb_dir[4] = {{-2, -2}, {0, -2}, {-2, 2}, {-2, 0}};
c_direction c_dir[4] = {{-1, -1}, {0, -1}, {-1, 1}, {-1, 0}};

struct mov
{
    movetype type;
    position src;
    position dest;
};

bool same_moves(mov m1, mov m2)
{
    if(m1.type != m2.type)return false;
    if(m1.src.r != m2.src.r)return false;
    if(m1.src.c != m2.src.c)return false;
    if(m1.dest.r != m2.dest.r)return false;
    if(m1.dest.c != m2.dest.c)return false;
    return true;
}

string stringify(mov MOV)
{
    string s;
    if(MOV.type == MOVE)s = "S     M     \n";
    else s = "S     B     \n";
    s[2] = char(MOV.src.c + 48);
    s[4] = char(MOV.src.r + 48);
    s[8] = char(MOV.dest.c + 48);
    s[10] = char(MOV.dest.r + 48);
    return s;
}

mov movify(string s)
{
    mov MOV;
    if(s[6] == 'M')MOV.type = MOVE;
    else MOV.type = BOMB;
    MOV.src.c = s[2]-48;
    MOV.src.r = s[4]-48;
    MOV.dest.c = s[8]-48;
    MOV.dest.r = s[10]-48;
    return MOV;
}

map<string, mov> transposition_table_player;
map<string, mov> transposition_table_enemy;

void update_board(char **board, mov MOV)
{
    if(MOV.type == MOVE)
    {
        board[MOV.dest.r][MOV.dest.c] = board[MOV.src.r][MOV.src.c];
        board[MOV.src.r][MOV.src.c] = '-';
    }
    else if(MOV.type == BOMB)
        board[MOV.dest.r][MOV.dest.c] = '-';
    else
        return;
}

void free_board_info(char **t_board)
{
    for(int i = 0; i < m; i++)delete(t_board[i]);
    delete(t_board);
}

//depth limited alpha-beta pruning
int alpha_beta_pruning(char** board, mov MOVE_PROP, int parent, int curr_depth, int max_depth, bool node_max)
{
    int alpha, beta;
    alpha = (node_max) ? INT_MIN : parent;
    beta = (node_max)? parent : INT_MAX;
    board_state_worth b = initialize_board_state_worth();
    next_move_info nm_info;
    char** t_board;
    t_board = new char*[m];
    for(int i = 0; i < m; i++)t_board[i] = new char[n];
    for(int i = 0; i < m; i++)
        for(int j = 0; j < n; j++)t_board[i][j] = board[i][j];
    /*if(p2_moves == 4)
                {
                    cout << endl;
                    for(int i = 0; i < m; i++)
                    {
                        for(int j = 0; j < n; j++)cout << t_board[i][j] << " ";
                        cout << endl;
                    }
                    //cout << t_board[r+k*dir[j].r][c+dir[j].c] << " Right here!" << endl;
                }*/
    update_board(t_board, MOVE_PROP);
    b = update_soldiers(t_board, b, &nm_info);
    nm_info = update_cannons(t_board, nm_info);
    b = update_board_state_worth(b, t_board, nm_info);

    int AOP = b.avg_dist_off_player;
    int AOE = b.avg_dist_off_enemy;
    int ADP = b.avg_dist_def_player;
    int ADE = b.avg_dist_def_enemy;
    int TP = b.townhalls_player;
    int TE = b.townhalls_enemy;
    int SP = b.soldiers_player;
    int SE = b.soldiers_enemy;
    int CP = count_cannons(nm_info, player);
    int CE = count_cannons(nm_info, ((player == '1')?'2':'1'));

    /*This function seems to be working somewhat decently. For testing out your heuristic functions, don't delete this one, just put it under comments*/
    int h_function = 10000000*(TP - TE) + 100000*(SP - SE) + 3*(AOP - AOE) - (2 - (100*(TP - TE) + 20*(SP - SE)))*(((12-SP)*m + ADP) - ((12-SE)*m + ADE)) + 10*(CP - CE);

    if(curr_depth == max_depth)return h_function;
    else if(TP == 2 || TE == 2)return h_function;
    else if(SP == 0 || SE == 0)return h_function;

    bool move_available = false;

    string key = "";
    mov best_move;
    for(int i = 0; i < nm_info.soldiers.size(); i++)
    {
        key += nm_info.soldiers[i].label;
        key += char(nm_info.soldiers[i].pos.r + 48);
        key += char(nm_info.soldiers[i].pos.c + 48);
    }

    if(!curr_depth)player_key = key;

    if(((node_max) ? (transposition_table_player.find(key) != transposition_table_player.end()) : (transposition_table_enemy.find(key) != transposition_table_enemy.end())))
    {
        best_move = ((node_max) ? transposition_table_player[key] : transposition_table_enemy[key]);
        /*if(p2_moves == 4)
                    {
                        cout << node_max << endl;
                        //cout << "Opponent: " << copponent << endl;
                        cout << endl << "Move type: " << best_move.type << endl;
                        cout << "Move source: " << best_move.src.r << " " << best_move.src.c << endl;
                        cout << "Move destination: " << best_move.dest.r << " " << best_move.dest.c << endl;
                        cout << endl;
                    }*/
        int temp = alpha_beta_pruning(t_board, best_move, ((node_max) ? alpha:beta), curr_depth+1, max_depth, !node_max);
        if(node_max)
        {
            alpha = max(temp, alpha);
            if(alpha >= beta)
            {
                free_board_info(t_board);
                return alpha;
            }
        }
        else
        {
            beta = min(temp, beta);
            if(alpha >= beta)
            {
                free_board_info(t_board);
                return beta;
            }
        }
    }
    char cplayer, copponent;
    if(node_max)
    {
        cplayer = player;
        copponent = ((player == '1') ? '2' : '1');
    }
    else
    {
        copponent = player;
        cplayer = ((player == '1') ? '2' : '1');
    }

    mov MOV;
    //consider soldier moves
    for(int i = 0; i < nm_info.soldiers.size(); i++)
    {
        if(nm_info.soldiers[i].label != cplayer)continue;
        position pos = nm_info.soldiers[i].pos;
        int r = pos.r;
        int c = pos.c;
        MOV.src.r = r;
        MOV.src.c = c;
        MOV.type = MOVE;
        bool surrounded = false;
        for(int j = 0; j < 8; j++)
        {
            int k = ((nm_info.soldiers[i].label == '1') ? -1 : 1);
            if(r+k*dir[j].r < m && r+k*dir[j].r >= 0 && c+dir[j].c < n && c+dir[j].c >= 0)
            {
                /*if(p2_moves == 4 && r+k*dir[j].r == 6 && c+dir[j].c == 7)
                {
                    cout << endl;
                    for(int i = 0; i < m; i++)
                    {
                        for(int j = 0; j < n; j++)cout << t_board[i][j] << " ";
                        cout << endl;
                    }
                    //cout << t_board[r+k*dir[j].r][c+dir[j].c] << " Right here!" << endl;
                }*/
                if(j >= 5)
                {
                    if(!surrounded)continue;
                    if(t_board[(r+k*dir[j].r)/2][(c+dir[j].c)/2] != '-')continue;
                }
                if(j < 5)
                {
                    if(t_board[r+k*dir[j].r][c+dir[j].c] == copponent)surrounded = true;
                }
                if(t_board[r+k*dir[j].r][c+dir[j].c] == cplayer)continue;
                if(j == 3 || j == 4)
                    if(t_board[r+k*dir[j].r][c+dir[j].c] != copponent && ((node_max) ? t_board[r+k*dir[j].r][c+dir[j].c] != 'b' : t_board[r+k*dir[j].r][c+dir[j].c] != 'a'))continue;
                MOV.dest.r = r+k*dir[j].r;
                MOV.dest.c = c+dir[j].c;
                if(!same_moves(MOV, best_move))
                {
                    /*if(p2_moves == 4)
                    {
                        cout << node_max << endl;
                        cout << "Opponent: " << copponent << endl;
                        cout << endl << "Move type: " << MOV.type << endl;
                        cout << "Move source: " << MOV.src.r << " " << MOV.src.c << endl;
                        cout << "Move destination: " << MOV.dest.r << " " << MOV.dest.c << endl;
                        cout << endl;
                    }*/
                    move_available = true;
                    int temp = alpha_beta_pruning(t_board, MOV, ((node_max) ? alpha:beta), curr_depth+1, max_depth, !node_max);
                    if(node_max)
                    {
                        if(temp > alpha)
                        {
                            alpha = temp;
                            transposition_table_player[key] = MOV;
                        }
                        if(alpha >= beta)
                        {
                            free_board_info(t_board);
                            return alpha;
                        }
                    }
                    else
                    {
                        if(temp < beta)
                        {
                            beta = temp;
                            transposition_table_enemy[key] = MOV;
                        }
                        if(alpha >= beta)
                        {
                            free_board_info(t_board);
                            return beta;
                        }
                    }
                }
                else
                {
                    /*if(p2_moves == 4)
                    {
                        cout << node_max << endl;
                        cout << "Opponent: " << copponent << endl;
                        cout << endl << "Move type: " << MOV.type << endl;
                        cout << "Move source: " << MOV.src.r << " " << MOV.src.c << endl;
                        cout << "Move destination: " << MOV.dest.r << " " << MOV.dest.c << endl;
                        cout << endl;
                    }*/
                }
            }
        }
    }

    //considering cannon bomb moves
    for(int i = 0; i < nm_info.cannons.size(); i++)
    {
        if(nm_info.cannons[i].soldier != cplayer)continue;
        position start = nm_info.cannons[i].start;
        position en = nm_info.cannons[i].en;
        int slope = nm_info.cannons[i].slope;
        int len = (slope) ? en.r-start.r+1 : en.c-start.c+1;
        bool shot_at_empty = false;
        for(int j = 0; j < 2; j++)
        {
            int r = bomb_dir[slope+1].r;
            int c = bomb_dir[slope+1].c;
            int rf = c_dir[slope+1].r;
            int cf = c_dir[slope+1].c;
            if(start.r + r + j*rf < m && start.r + r + j*rf >= 0 && start.c + c + j*cf < m && start.c + c + j*cf >= 0)
            {
                if(t_board[start.r + rf][start.c + cf] != '-')continue;
                if(t_board[start.r + r + j*rf][start.c + c + j*cf] == cplayer)continue;
                MOV.type = BOMB;
                MOV.src.r = start.r;
                MOV.src.c = start.c;
                MOV.dest.r = start.r + r + j*rf;
                MOV.dest.c = start.c + c + j*cf;
                if(!same_moves(MOV, best_move))
                {
                    move_available = true;
                    if(t_board[start.r + r + j*rf][start.c + c + j*cf] != '-' || !shot_at_empty)
                    {
                        /*if(p2_moves == 4)
                    {
                        cout << node_max << endl;
                        cout << endl << "Move type: " << MOV.type << endl;
                        cout << "Move source: " << MOV.src.r << " " << MOV.src.c << endl;
                        cout << "Move destination: " << MOV.dest.r << " " << MOV.dest.c << endl;
                        cout << endl;
                    }*/
                        int temp = alpha_beta_pruning(t_board, MOV, ((node_max) ? alpha:beta), curr_depth+1, max_depth, !node_max);
                        if(t_board[start.r + r + j*rf][start.c + c + j*cf] == '-')shot_at_empty = true;
                        if(node_max)
                        {
                            if(temp > alpha)
                            {
                                alpha = temp;
                                transposition_table_player[key] = MOV;
                            }
                            if(alpha >= beta)
                            {
                                free_board_info(t_board);
                                return alpha;
                            }
                        }
                        else
                        {
                            if(temp < beta)
                            {
                                beta = temp;
                                transposition_table_enemy[key] = MOV;
                            }
                            if(alpha >= beta)
                            {
                                free_board_info(t_board);
                                return beta;
                            }
                        }
                    }
                }
            }

            if(en.r - r - j*rf < m && en.r - r - j*rf >= 0 && en.c - c - j*cf < m && en.c - c - j*cf >= 0)
            {
                if(t_board[en.r - rf][en.c - cf] != '-')continue;
                if(t_board[en.r - r - j*rf][en.c - c - j*cf] == cplayer)continue;
                MOV.type = BOMB;
                MOV.src.r = en.r;
                MOV.src.c = en.c;
                MOV.dest.r = en.r - r - j*rf;
                MOV.dest.c = en.c - c - j*cf;
                if(!same_moves(MOV, best_move))
                {
                    move_available = true;
                    if(t_board[en.r - r - j*rf][en.c - c - j*cf] != '-' || !shot_at_empty)
                    {
                        int temp = alpha_beta_pruning(t_board, MOV, ((node_max) ? alpha:beta), curr_depth+1, max_depth, !node_max);
                        if(t_board[en.r - r - j*rf][en.c - c - j*cf] == '-')shot_at_empty = true;
                        if(node_max)
                        {
                            if(temp > alpha)
                            {
                                alpha = temp;
                                transposition_table_player[key] = MOV;
                            }
                            if(alpha >= beta)
                            {
                                free_board_info(t_board);
                                return alpha;
                            }
                        }
                        else
                        {
                            if(temp < beta)
                            {
                                beta = temp;
                                transposition_table_enemy[key] = MOV;
                            }
                            if(alpha >= beta)
                            {
                                free_board_info(t_board);
                                return beta;
                            }
                        }
                    }
                }
            }
        }
    }

    //considering cannon movement
    for(int i = 0; i < nm_info.cannons.size(); i++)
    {
        if(nm_info.cannons[i].soldier != cplayer)continue;
        position start = nm_info.cannons[i].start;
        position en = nm_info.cannons[i].en;
        int slope = nm_info.cannons[i].slope;
        int len = (slope) ? en.r-start.r+1 : en.c-start.c+1;

        int r = c_dir[slope+1].r;
        int c = c_dir[slope+1].c;

        if(abs(en.r - start.r) >= 3 || abs(en.c-start.c) >= 3)continue;

        if(start.r + r < m && start.r + r >= 0 && start.c + c < n && start.c + c >= 0)
        {
            if(t_board[start.r+r][start.c+c] != '-')continue;
            MOV.type = MOVE;
            MOV.src.r = en.r;
            MOV.src.c = en.c;
            MOV.dest.r = start.r + r;
            MOV.dest.c = start.c + c;
            if(!same_moves(MOV, best_move))
            {
                move_available = true;
                int temp = alpha_beta_pruning(t_board, MOV, ((node_max) ? alpha:beta), curr_depth+1, max_depth, !node_max);
                if(node_max)
                {
                    if(temp > alpha)
                    {
                        alpha = temp;
                        transposition_table_player[key] = MOV;
                    }
                    if(alpha >= beta)
                    {
                        free_board_info(t_board);
                        return alpha;
                    }
                }
                else
                {
                    if(temp < beta)
                    {
                        beta = temp;
                        transposition_table_enemy[key] = MOV;
                    }
                    if(alpha >= beta)
                    {
                        free_board_info(t_board);
                        return beta;
                    }
                }
            }
        }

        if(en.r - r < m && en.r - r >= 0 && en.c - c < n && en.c - c >= 0)
        {
            if(t_board[en.r-r][en.c-c] != '-')continue;
            MOV.type = MOVE;
            MOV.src.r = start.r;
            MOV.src.c = start.c;
            MOV.dest.r = en.r - r;
            MOV.dest.c = en.c - c;
            if(!same_moves(MOV, best_move))
            {
                move_available = true;
                int temp = alpha_beta_pruning(t_board, MOV, ((node_max) ? alpha:beta), curr_depth+1, max_depth, !node_max);
                if(node_max)
                {
                    if(temp > alpha)
                    {
                        alpha = temp;
                        transposition_table_player[key] = MOV;
                    }
                    if(alpha >= beta)
                    {
                        free_board_info(t_board);
                        return alpha;
                    }
                }
                else
                {
                    if(temp < beta)
                    {
                        beta = temp;
                        transposition_table_enemy[key] = MOV;
                    }
                    if(alpha >= beta)
                    {
                        free_board_info(t_board);
                        return beta;
                    }
                }
            }
        }
    }
    free_board_info(t_board);
    if(!move_available) return h_function;
    return ((node_max) ? alpha : beta);
}

int main()
{
    int cnt = 0, cspace = 0, level = 7;
    string init, initf = "";
    getline(cin, init);
    player = init[0];
    for(int i = 0; i < init.size(); i++)
    {
        if(init[i] == ' ')cspace++;
        if(cspace == 3)break;
        initf += init[i];
    }
    if(initf.size() == 5)
    {
        m = 8;
        n = 8;
        level = 7;
    }
    else if(initf.size() == 6)
    {
        m = 10;
        n = 8;
        level = 7;
    }
    else
    {
        m = 10;
        n = 10;
        level = 6;
    }
    //m = init[2]-48;
    //n = init[4]-48;
    //for(int i = init.size()-1; i >= 6; i--)timelimit = timelimit*10 + (init[i]-48);

    char** board, **tboard;
    next_move_info nm_info;
    board = new char*[m];
    for(int i = 0; i < m; i++)
        board[i] = new char[n];

    for(int i = 0; i < m; i++)
        for(int j = 0; j < n; j++)
            board[i][j] = '-';

    for(int j = 0; j < n; j++)
    {
        if(!(j%2))
        {
            board[0][j] = (player == '1') ? 'b' : 'a';
            board[m-1][j] = '1';
            board[m-2][j] = '1';
            board[m-3][j] = '1';
        }
        else
        {
            board[m-1][j] = (player == '1') ? 'a' : 'b';
            board[0][j] = '2';
            board[1][j] = '2';
            board[2][j] = '2';
        }
    }

    char curr_player = '1';
    mov MOV;
    while(1)
    {
        if(curr_player != player)
        {
            string s;
            getline(cin, s);
            MOV = movify(s);
            update_board(board, MOV);
        }
        else
        {
            cnt++;
            /*if(cnt == 7)
            {
                transposition_table_player.clear();
                transposition_table_enemy.clear();
                cnt = 0;
            }*/
            MOV.type = DUMMY;
            MOV.src.r = -1;
            MOV.src.c = -1;
            MOV.dest.r = -1;
            MOV.dest.c = -1;

            for(int i = 0; i < level; i++)
            {
                int temp = alpha_beta_pruning(board, MOV, INT_MAX, 0, i, true);
            }

            MOV = transposition_table_player[player_key];
            update_board(board, MOV);
            string player_move = stringify(MOV);
            //cerr << "Hello\n";
            cout << player_move;

        }
        curr_player = (curr_player == '1') ? '2' : '1';
    }
    return 0;
}

/*int main()
{
    string init;
    getline(cin, init);
    player = init[0];
    m = init[2]-48;
    n = init[4]-48;
    for(int i = init.size()-1; i >= 6; i--)timelimit = timelimit*10 + (init[i]-48);

    char** board, **tboard;
    next_move_info nm_info;
    board = new char*[m];
    for(int i = 0; i < m; i++)
        board[i] = new char[n];

    for(int i = 0; i < m; i++)
        for(int j = 0; j < n; j++)
            board[i][j] = '-';

    for(int j = 0; j < n; j++)
    {
        if(!(j%2))
        {
            board[0][j] = (player == '1') ? 'b' : 'a';
            board[m-1][j] = '1';
            board[m-2][j] = '1';
            board[m-3][j] = '1';
        }
        else
        {
            board[m-1][j] = (player == '1') ? 'a' : 'b';
            board[0][j] = '2';
            board[1][j] = '2';
            board[2][j] = '2';
        }
    }

    char curr_player = '1';
    mov MOV;
    while(1)
    {
        if(curr_player != player)
        {
            string s;
            getline(cin, s);
            MOV = movify(s);
            update_board(board, MOV);
        }
        else
        {
            transposition_table_player.clear();
            transposition_table_enemy.clear();

            MOV.type = DUMMY;
            MOV.src.r = -1;
            MOV.src.c = -1;
            MOV.dest.r = -1;
            MOV.dest.c = -1;

            for(int i = 0; i < 7; i++)
            {
                int temp = alpha_beta_pruning(board, MOV, INT_MAX, 0, i, true);
            }

            MOV = transposition_table_player[player_key];
            update_board(board, MOV);
            string player_move = stringify(MOV);
            cout << player_move;

        }
        curr_player = (curr_player == '1') ? '2' : '1';
    }
    return 0;
}*/
