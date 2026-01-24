#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <queue>
#include <set>
#include <climits>
#include <random>
#include "DieTechLib.h"
using namespace std;

#define NUM_OF_DIE 2

struct CompareCell {
    bool operator()(Cell* a, Cell* b) {
        // if(a->gain_test == b->gain_test){
        //     if(a->die_test == b->die_test && a->die_test == "A"){
        //         return a->area_in_B < b->area_in_B;
        //     }
        //     if(a->die_test == b->die_test && a->die_test == "B"){
        //         return a->area_in_A < b->area_in_A;
        //     }
        //     if(a->die_test == "A" && b->die_test == "B"){
        //         return a->area_in_B < b->area_in_A;
        //     }
        //     if(a->die_test == "B" && b->die_test == "A"){
        //         return a->area_in_A < b->area_in_B;
        //     }
        // }
        if (a->gain_test == b->gain_test) {
            if(a->nets.size() == b->nets.size()) {
                long long min_area_a = min(a->area_in_A, a->area_in_B);
                long long min_area_b = min(b->area_in_A, b->area_in_B);
                if(min_area_a == min_area_b) {
                    if (a->area_in_A != b->area_in_A) {
                        return a->area_in_A < b->area_in_A; 
                    }else {
                        return a->area_in_B < b->area_in_B;
                    }
                    // else{
                        // return a->connected_weight < b->connected_weight;
                    // }
                }
                return min_area_a < min_area_b; 
            }
            return a->nets.size() > b->nets.size(); 
        }
        return a->gain_test > b->gain_test;
    }
};

// struct CompareNet {
//     bool operator()(Net* a, Net* b) {
//         return min(a->max_area_in_A, a->max_area_in_B) < min(b->max_area_in_A, b->max_area_in_B);
//     }
// };


vector<string> split(string str) {
    vector<string> result;
    istringstream iss(str);
    for (string s; iss >> s;) {
        result.push_back(s);
    }
    return result;
}

vector<string> input_preprocess(ifstream &input_file) {
    vector<string> objects;
    string line;
    while (getline(input_file, line)) 
        objects.push_back(line);
    return objects;
}

void update_test_values(Die* die, vector<Net*>& nets_vector, unordered_map<string, Cell*>& cells_map) {
    for (int i = 0; i < NUM_OF_DIE; i++) die[i].update_test();
    for (auto net : nets_vector) net->update_test();
    for (auto cell : cells_map) cell.second->update_test();
}

void output_result(Die* die, long long cut_size, ofstream &output_file) {
    output_file << "CutSize " << cut_size << endl;
    output_file << "DieA " << die[0].cells_map.size() << endl;
    for (auto cell : die[0].cells_map) {
        output_file << cell.first << endl;
    }
    output_file << "DieB " << die[1].cells_map.size() << endl;
    for (auto cell : die[1].cells_map) {
        output_file << cell.first << endl;
    }
}

multiset<Cell*, CompareCell> cells_set;
unordered_map<string, Tech*> techs_map;
Die die[NUM_OF_DIE];
unordered_map<string, Cell*> cells_map;
vector<Cell*> cells_vector;
// unordered_map<string, Net*> nets_map;
// multiset<Net*, CompareNet> nets_set;
vector<Net*> nets_vector;

int main(int argc, char *argv[]) {
    // Check if the number of arguments is correct
    // if (argc != 3) {
    //     cout << "Wrong number of arguments" << endl;
    //     return 1;
    // }

    // Open the input file
    ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        cout << "Error: could not open file " << argv[1] << endl;
        return 1;
    }

    // Open the output file
    ofstream output_file(argv[2]);
    if (!output_file.is_open()) {
        cout << "Error: could not open file " << argv[2] << endl;
        return 1;
    }

    vector<string> input_objects = input_preprocess(input_file);
    int object_size = input_objects.size();
    int num_techs = 0;

    for (int index = 0; index < object_size; index++) {
        vector<string> object = split(input_objects[index++]);
        if (object.size() == 0)  continue;
        string object_name = object[0];
        if (object_name == "NumTechs") {
            num_techs = stoi(object[1]);
            // build techs
            for (int i = 0; i < num_techs; i++) {
                vector<string> object = split(input_objects[index++]);
                string tech_name = object[1];
                int lib_cells_num = stoi(object[2]);
                Tech* tech = new Tech();
                techs_map[tech_name] = tech;
                // build lib cells
                for (int j = 0; j < lib_cells_num; j++){
                    vector<string> object = split(input_objects[index++]);
                    string lib_name = object[1];
                    long long width = stoll(object[2]);
                    long long height = stoll(object[3]);
                    long long area = width * height;
                    tech->lib_area_map[lib_name] = area;
                }
            }
        }
        else if(object_name == "DieSize") {
            long long die_width = stoll(object[1]);
            long long die_height = stoll(object[2]);
            long long die_area = die_width * die_height;
            // build dies
            for (int i = 0; i < NUM_OF_DIE; i++) {
                vector<string> object = split(input_objects[index++]);
                string tech_name = object[1];
                double max_area_utilization = stoi(object[2]) / (double)100;
                long long max_area = (long long) floor(max_area_utilization * die_area);
                die[i] = Die(techs_map[tech_name], max_area);
            }
        }
        else if(object_name == "NumCells") {
            long long num_cells = stoi(object[1]);
            // build cells
            for (int i = 0; i < num_cells; i++) {
                vector<string> object = split(input_objects[index++]);
                string cell_name = object[1];
                string lib_cell_name = object[2];
                long long area_in_A = die[0].tech->lib_area_map[lib_cell_name];
                long long area_in_B = die[1].tech->lib_area_map[lib_cell_name];
                cells_map[cell_name] = new Cell(area_in_A, area_in_B, cell_name);
                cells_vector.push_back(cells_map[cell_name]);
            }
        }
        else if(object_name == "NumNets") {
            long long num_nets = stoll(object[1]);
            // build nets
            for (int i = 0; i < num_nets; i++) {
                vector<string> object = split(input_objects[index++]);
                string net_name = object[1];
                int num_of_connected_cells = stoi(object[2]);
                long long weight = stoll(object[3]);
                // build net
                Net* net = new Net(weight);
                // nets_map[net_name] = net;
                nets_vector.push_back(net);
                for (int j = 0; j < num_of_connected_cells; j++) {
                    vector<string> object = split(input_objects[index++]);
                    string cell_name = object[1];
                    Cell* cell = cells_map[cell_name];
                    // net->cells.push_back(cell); // 也許可以刪掉
                    cell->nets.push_back(net);
                    cell->connected_weight += weight;
                    net->cells[cell_name] = cell;
                }
            }
        }
    }

    // for(auto& net:nets_vector){
    //     for(auto& cell:net->cells){
    //         net->max_area_in_A += cell.second->area_in_A;
    //         net->max_area_in_B += cell.second->area_in_B;
    //     }
    // }

    // sort(nets_vector.begin(), nets_vector.end(), [](Net* a, Net* b) {
    //     return min(a->max_area_in_A, a->max_area_in_B) < min(b->max_area_in_A, b->max_area_in_B);
    // });
    // random_device rd;
    // mt19937 gen(rd());
    // shuffle(nets_vector.begin(), nets_vector.end(), gen);    
    sort(cells_vector.begin(), cells_vector.end(), [](Cell* a, Cell* b) {
        long long min_area_a = min(a->area_in_A, a->area_in_B);
        long long min_area_b = min(b->area_in_A, b->area_in_B);
        if (min_area_a == min_area_b) {
            if(a->connected_weight == b->connected_weight) {
                if(a->nets.size() == b->nets.size()) {
                    if(a->area_in_A != b->area_in_A) {
                        return a->area_in_A < b->area_in_A;
                    }else{
                        return a->area_in_B < b->area_in_B;
                    }
                }
                return a->nets.size() > b->nets.size();
            }
            return a->connected_weight > b->connected_weight;
        }
        return min_area_a < min_area_b;
    });

    // shuffle(cells_vector.begin(), cells_vector.end(), gen);
    // reset
    for (auto cell : cells_map) {
        cell.second->gain = 0;
        cell.second->is_locked = false;
    }
    for (auto net : nets_vector) {
        net->cells_in_A.clear();
        net->cells_in_B.clear();
    }
    for (int i = 0; i < NUM_OF_DIE; i++) {
        die[i].cells_map.clear();
        die[i].area = 0;
    }

    for (auto cell : cells_vector) {
        // cout << min(cell->area_in_A, cell->area_in_B) << endl;
        if (cell->area_in_A < cell->area_in_B) {
            if ((die[0].area + cell->area_in_A) <= die[0].max_area) {
                cell->die = "A";
                die[0].cells_map[cell->cell_name] = cell;
                die[0].area += cell->area_in_A;
                for (auto net : cell->nets) {
                    net->cells_in_A[cell->cell_name] = cell;
                }
            }
            else {
                cell->die = "B";
                die[1].cells_map[cell->cell_name] = cell;
                die[1].area += cell->area_in_B;
                for (auto net : cell->nets) {
                    net->cells_in_B[cell->cell_name] = cell;
                }
            }
        }
        else {
            if ((die[1].area + cell->area_in_B) <= die[1].max_area) {
                cell->die = "B";
                die[1].cells_map[cell->cell_name] = cell;
                die[1].area += cell->area_in_B;
                for (auto net : cell->nets) {
                    net->cells_in_B[cell->cell_name] = cell;
                }
            }
            else {
                cell->die = "A";
                die[0].cells_map[cell->cell_name] = cell;
                die[0].area += cell->area_in_A;
                for (auto net : cell->nets) {
                    net->cells_in_A[cell->cell_name] = cell;
                }
            }
        }
    }

    // for (Cell* cell : cells_vector) {
    //     bool assigned = false;
    //     if ((rand() & 1) == 0 && (die[0].area + cell->area_in_A) <= die[0].max_area) {
    //         cell->die = "A";
    //         die[0].cells_map[cell->cell_name] = cell;
    //         die[0].area += cell->area_in_A;
    //         for (auto net : cell->nets) {
    //             net->cells_in_A[cell->cell_name] = cell;
    //         }
    //     } else if ((die[1].area + cell->area_in_B) <= die[1].max_area) {
    //         cell->die = "B";
    //         die[1].cells_map[cell->cell_name] = cell;
    //         die[1].area += cell->area_in_B;
    //         for (auto net : cell->nets) {
    //             net->cells_in_B[cell->cell_name] = cell;
    //         }
    //     } else {
    //         cell->die = "A";
    //         die[0].cells_map[cell->cell_name] = cell;
    //         die[0].area += cell->area_in_A;
    //         for (auto net : cell->nets) {
    //             net->cells_in_A[cell->cell_name] = cell;
    //         }
    //     }
    // }
    // cout << "Max area: " << die[0].max_area << " " << die[1].max_area << endl;
    // for(auto& net:nets_vector){
    //     for(auto& cell:net->cells){
    //         if(cell.second->is_locked){
    //             if(cell.second->die == "A"){
    //                 net->cells_in_A[cell.first] = cell.second;
    //             }
    //             else{
    //                 net->cells_in_B[cell.first] = cell.second;
    //             }
    //             continue;
    //         }
            // if((die[0].area + cell.second->area_in_A) <= die[0].max_area){
            //     cell.second->die = "A";
            //     die[0].cells_map[cell.first] = cell.second;
            //     die[0].area += cell.second->area_in_A;
            //     net->cells_in_A[cell.first] = cell.second;
            // }
            // else {
            //     cell.second->die = "B";
            //     die[1].cells_map[cell.first] = cell.second;
            //     die[1].area += cell.second->area_in_B;
            //     net->cells_in_B[cell.first] = cell.second;
            // }

            // if((die[1].area + cell.second->area_in_B) <= die[1].max_area){
            //     cell.second->die = "B";
            //     die[1].cells_map[cell.first] = cell.second;
            //     die[1].area += cell.second->area_in_B;
            //     net->cells_in_B[cell.first] = cell.second;
            // }
            // else {
            //     cell.second->die = "A";
            //     die[0].cells_map[cell.first] = cell.second;
            //     die[0].area += cell.second->area_in_A;
            //     net->cells_in_A[cell.first] = cell.second;
            // }
            // cout << min(net->max_area_in_A, net->max_area_in_B) << endl;

        //     if(net->max_area_in_A < net->max_area_in_B){
        //         if((die[0].area + cell.second->area_in_A) <= die[0].max_area){
        //             cell.second->die = "A";
        //             die[0].cells_map[cell.first] = cell.second;
        //             die[0].area += cell.second->area_in_A;
        //             net->cells_in_A[cell.first] = cell.second;
        //         }
        //         else {
        //             cell.second->die = "B";
        //             die[1].cells_map[cell.first] = cell.second;
        //             die[1].area += cell.second->area_in_B;
        //             net->cells_in_B[cell.first] = cell.second;
        //         }
        //     }else {
        //         if((die[1].area + cell.second->area_in_B) <= die[1].max_area){
        //             cell.second->die = "B";
        //             die[1].cells_map[cell.first] = cell.second;
        //             die[1].area += cell.second->area_in_B;
        //             net->cells_in_B[cell.first] = cell.second;
        //         }
        //         else {
        //             cell.second->die = "A";
        //             die[0].cells_map[cell.first] = cell.second;
        //             die[0].area += cell.second->area_in_A;
        //             net->cells_in_A[cell.first] = cell.second;
        //         }
        //     }
        //     cell.second->is_locked = true;
        // }
        // cout << "net: " << net.second->cells_in_A.size() << " " << net.second->cells_in_B.size() << endl;
    // }
    // print utilization of area
    cout << "max area: " << die[0].max_area << " " << die[1].max_area << endl;
    cout << "    area: " << die[0].area << " " << die[1].area << endl;
    // 算出每個 cell 的 initial gain
    for (auto& net : nets_vector) {
        long long weight = net->weight;
        // 如果只有一個 cell 在某 die，則 gain += weight，更傾向這個 cell 被移動 (會減少 cut size)
        if (net->cells_in_A.size() == 1) {
            for (auto cell : net->cells_in_A) {
                cell.second->gain += weight;
            }
        }
        if (net->cells_in_B.size() == 1) {
            for (auto cell : net->cells_in_B) {
                cell.second->gain += weight;
            }
        }
        // 如果某個 die 沒有 cell，則 gain -= weight，不希望這些 cell 被移動 (會多算 cut size)
        if (net->cells_in_A.size() == 0) {
            for (auto cell : net->cells_in_B) {
                cell.second->gain -= weight;
            }
        }
        if (net->cells_in_B.size() == 0) {
            for (auto cell : net->cells_in_A) {
                cell.second->gain -= weight;
            }
        }
    }


    // 算出 initial cut size
    long long cut_size = 0;
    for (auto& net : nets_vector) {
        if (net->cells_in_A.size() > 0 && net->cells_in_B.size() > 0) {
            cut_size += net->weight;
        }
    }

    // debug
    // while (!pq.empty()) {
    //     Cell* cell = pq.top();
    //     pq.pop();
    //     cout << cell->lib_cell_name << " " << cell->gain << endl;
    // }

    // Step 3: Move the cell with the largest gain

    while(1){
        cout << "cut size: " << cut_size << endl;
        update_test_values(die, nets_vector, cells_map);
        vector<long long> gain_record;
        vector<Cell*> cells_to_move;
        cells_set.clear();
        for (auto& cell : cells_map) {
            cell.second->is_locked = false;
            cells_set.insert(cell.second);
        }
        while (!cells_set.empty()) {
            // 找到最大 gain 的 cell
            // Cell* base_cell = *cells_set.begin();
            auto it = cells_set.begin();
            Cell* base_cell = NULL;
            // pq.pop();
            // cnt--;
            // cells_set.erase(base_cell);
            // base_cell->is_locked = true;
            // 如果移動後，超過 die 的面積限制，就不要動
            // if((base_cell->die_test == "A" && (die[1].area_test + base_cell->area_in_B > die[1].max_area)) || \
            //     (base_cell->die_test == "B" && (die[0].area_test + base_cell->area_in_A > die[0].max_area))) {
            //     continue;
            // }
            while(it != cells_set.end()) {
                base_cell = *it;
                if((base_cell->die_test == "A" && (die[1].area_test + base_cell->area_in_B > die[1].max_area)) || \
                    (base_cell->die_test == "B" && (die[0].area_test + base_cell->area_in_A > die[0].max_area))) {
                    it++;
                    continue;
                }
                break;
            }

            if(base_cell == NULL || it == cells_set.end()) break;
            if (base_cell->is_locked) continue;

            cells_set.erase(it);
            base_cell->is_locked = true;
            long long gain = 0;
                
            // update gain
            for (auto& net : base_cell->nets) {
                long long weight = net->weight;
                // before move
                if (base_cell->die_test == "A") {
                    if (net->cells_in_B_test.size() == 0) {
                        for (auto& cell : net->cells_in_A_test) {
                            if (cell.second->is_locked) continue;
                            cells_set.erase(cell.second);
                            cell.second->gain_test += weight;
                            cells_set.insert(cell.second);
                        }
                        gain -= weight;
                    }
                    else if (net->cells_in_B_test.size() == 1) {
                        for (auto& cell : net->cells_in_B_test) {
                            if (cell.second->is_locked) continue;
                            cells_set.erase(cell.second);
                            cell.second->gain_test -= weight;
                            cells_set.insert(cell.second);
                        }
                    }
                }
                else {
                    if (net->cells_in_A_test.size() == 0) {
                        for (auto& cell : net->cells_in_B_test) {
                            if (cell.second->is_locked) continue;
                            cells_set.erase(cell.second);
                            cell.second->gain_test += weight;
                            cells_set.insert(cell.second);
                        }
                        gain -= weight;
                    }
                    else if (net->cells_in_A_test.size() == 1) {
                        for (auto& cell : net->cells_in_A_test) {
                            if (cell.second->is_locked) continue;
                            cells_set.erase(cell.second);
                            cell.second->gain_test -= weight;
                            cells_set.insert(cell.second);
                        }
                    }
                }

                // update net
                if (base_cell->die_test == "A") {
                    net->cells_in_A_test.erase(base_cell->cell_name);
                    net->cells_in_B_test[base_cell->cell_name] = base_cell;
                }
                else {
                    net->cells_in_B_test.erase(base_cell->cell_name);
                    net->cells_in_A_test[base_cell->cell_name] = base_cell;
                }

                // after move
                if (base_cell->die_test == "A") {
                    if (net->cells_in_A_test.size() == 0) {
                        for (auto& cell : net->cells_in_B_test) {
                            if (cell.second->is_locked) continue;
                            cells_set.erase(cell.second);
                            cell.second->gain_test -= weight;
                            cells_set.insert(cell.second);
                        }
                        gain += weight;
                    }
                    else if (net->cells_in_A_test.size() == 1) {
                        for (auto& cell : net->cells_in_A_test) {
                            if (cell.second->is_locked) continue;
                            cells_set.erase(cell.second);
                            cell.second->gain_test += weight;
                            cells_set.insert(cell.second);
                        }
                    }
                }
                else {
                    if (net->cells_in_B_test.size() == 0) {
                        for (auto& cell : net->cells_in_A_test) {
                            if (cell.second->is_locked) continue;
                            cells_set.erase(cell.second);
                            cell.second->gain_test -= weight;
                            cells_set.insert(cell.second);
                        }
                        gain += weight;
                    }
                    else if (net->cells_in_B_test.size() == 1) {
                        for (auto& cell : net->cells_in_B_test) {
                            if (cell.second->is_locked) continue;
                            cells_set.erase(cell.second);
                            cell.second->gain_test += weight;
                            cells_set.insert(cell.second);
                        }
                    }
                }
            }

            // update die
            if (base_cell->die_test == "A") {
                die[0].area_test -= base_cell->area_in_A;
                die[1].area_test += base_cell->area_in_B;
                base_cell->die_test = "B";
            }
            else {
                die[0].area_test += base_cell->area_in_A;
                die[1].area_test -= base_cell->area_in_B;
                base_cell->die_test = "A";
            }
            gain_record.push_back(gain);
            cells_to_move.push_back(base_cell);
        }

        // calculate max partial sum of gain vector
        long long max_partial_gain_sum = 0;
        long long partial_gain_sum = 0;
        int move_count = 0;
        for (int i = 0; i < gain_record.size(); i++) {
            partial_gain_sum += gain_record[i];
            if (partial_gain_sum >= max_partial_gain_sum) {
                max_partial_gain_sum = partial_gain_sum;
                move_count = i + 1;
            }
        }

        if (max_partial_gain_sum <= 0) break;
        cut_size -= max_partial_gain_sum;
        // 真的移動 cell
        for (int i = 0; i < move_count; i++) {
            Cell* base_cell = cells_to_move[i];
            for (auto net : base_cell->nets) {
                long long weight = net->weight;
                // before move
                if (base_cell->die == "A") {
                    if (net->cells_in_B.size() == 0) {
                        for (auto cell : net->cells_in_A) {
                            // if (cell.second == base_cell) cell.second->gain += weight;
                            cell.second->gain += weight;
                        }
                    }
                    else if (net->cells_in_B.size() == 1) {
                        for (auto cell : net->cells_in_B) {
                            cell.second->gain -= weight;
                        }
                    }
                }
                else {
                    if (net->cells_in_A.size() == 0) {
                        for (auto cell : net->cells_in_B) {
                            // if (cell.second == base_cell) cell.second->gain += weight;
                            cell.second->gain += weight;
                        }
                    }
                    else if (net->cells_in_A.size() == 1) {
                        for (auto cell : net->cells_in_A) {
                            cell.second->gain -= weight;
                        }
                    }
                }

                // update net
                if (base_cell->die == "A") {
                    net->cells_in_A.erase(base_cell->cell_name);
                    net->cells_in_B[base_cell->cell_name] = base_cell;
                }
                else {
                    net->cells_in_B.erase(base_cell->cell_name);
                    net->cells_in_A[base_cell->cell_name] = base_cell;
                }

                // after move
                if (base_cell->die == "A") {
                    if (net->cells_in_A.size() == 0) {
                        for (auto cell : net->cells_in_B) {
                            // if (cell.second == base_cell) cell.second->gain -= weight;
                            cell.second->gain -= weight;
                        }
                    }
                    else if (net->cells_in_A.size() == 1) {
                        for (auto cell : net->cells_in_A) {
                            cell.second->gain += weight;
                        }
                    }
                }
                else {
                    if (net->cells_in_B.size() == 0) {
                        for (auto cell : net->cells_in_A) {
                            // if (cell.second == base_cell) cell.second->gain -= weight;
                            cell.second->gain -= weight;
                        }
                    }
                    else if (net->cells_in_B.size() == 1) {
                        for (auto cell : net->cells_in_B) {
                            cell.second->gain += weight;
                        }
                    }
                }
            }

            // update die
            if (base_cell->die == "A") {
                die[0].cells_map.erase(base_cell->cell_name);
                die[1].cells_map[base_cell->cell_name] = base_cell;
                die[0].area -= base_cell->area_in_A;
                die[1].area += base_cell->area_in_B;
                base_cell->die = "B";
            }
            else {
                die[0].cells_map[base_cell->cell_name] = base_cell;
                die[1].cells_map.erase(base_cell->cell_name);
                die[0].area += base_cell->area_in_A;
                die[1].area -= base_cell->area_in_B;
                base_cell->die = "A";
            }
            base_cell->is_locked = true;
        }
    }

    output_result(die, cut_size, output_file);
    
    
        // output_file << "test: " << max_partial_gain_sum << endl;
        // output_file << "test: " << move_count << endl;
    
    
        // for (auto gain : gain_record) {
        //     cout << gain << endl;
        // }
    
    cout << "Final cut size: " << cut_size << endl;
    input_file.close();
    output_file.close();
    return 0;
}