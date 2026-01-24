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
#include <utility>
#include <cmath>
#include <chrono>
#include <stack>
#include "BlockLib.h"
#include "Tree.h"

using namespace std;
using namespace std::chrono;

vector<string> split(string& str) {
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

void select_best_branch(TreeNode* root, int& index) {
    if (root == nullptr || index == -1) return;

    stack<tuple<TreeNode*, int>> stk;
    stk.push(make_tuple(root, index));

    while (!stk.empty()) {
        TreeNode* node;
        int shape_index;
        tie(node, shape_index) = stk.top();
        stk.pop();

        if (node == nullptr || shape_index == -1) continue;

        Shape* best_shape = node->shapes[shape_index];
        node->block = best_shape->block;

        if (node->left != nullptr && node->right != nullptr) {
            node->block->is_leaf = false;
            stk.push(make_tuple(node->right, best_shape->right_child_index));
            stk.push(make_tuple(node->left, best_shape->left_child_index));
        } else {
            node->block->is_leaf = true;
        }
    }
}

void select_best_path(TreeNode* root, int& floorplan_width) {
    long long min_cost = LLONG_MAX, valid_min_cost= LLONG_MAX;
    Shape *best_shape = nullptr, *valid_best_shape = nullptr;
    for (auto &shape : root->shapes) {
        long long cost = 0;
        if(shape->block->width > floorplan_width) cost += (shape->block->width - floorplan_width);
        if(shape->block->height > floorplan_width) cost += (shape->block->height - floorplan_width);
        if (min_cost > cost) {
            min_cost = cost;
            best_shape = shape;
        }
        if(shape->block->width <= floorplan_width && shape->block->height <= floorplan_width) {
            if (valid_min_cost > cost) {
                valid_min_cost = cost;
                valid_best_shape = shape;
            }
        }
    }
    if(valid_best_shape != nullptr) {
        best_shape = valid_best_shape;
    }

    root->block = best_shape->block;
    root->block->is_leaf = false;
    select_best_branch(root->left, best_shape->left_child_index);
    select_best_branch(root->right, best_shape->right_child_index);
}

void select_random_path(TreeNode* root, int floorplan_width, mt19937 &gen) {
    uniform_int_distribution<> dis(0, root->shapes.size() - 1);
    int random_index = dis(gen);
    Shape *random_shape = root->shapes[random_index];

    root->block = random_shape->block;
    root->block->is_leaf = false;
    select_best_branch(root->left, random_shape->left_child_index);
    select_best_branch(root->right, random_shape->right_child_index);
}

TreeNode* build_slicing_tree(vector<string> &solution, unordered_map<string, Block*> &block_map) {
    stack<TreeNode*> st;

    for (const auto& node_name : solution) {
        TreeNode* node = new TreeNode(node_name, nullptr, nullptr, nullptr);

        if (node_name == "V" || node_name == "H") {
            TreeNode* right = st.top(); st.pop();
            TreeNode* left = st.top(); st.pop();

            node->left = left;
            node->right = right;
            left->parent = node;
            right->parent = node;

            vector<ShapeKey> candidates;
            for (int i = 0; i < left->shapes.size(); ++i) {
                for (int j = 0; j < right->shapes.size(); ++j) {
                    auto l_shape = left->shapes[i];
                    auto r_shape = right->shapes[j];

                    pair<int, int> new_shape;
                    if (node_name == "V") {
                        new_shape = {
                            l_shape->block->width + r_shape->block->width,
                            max(l_shape->block->height, r_shape->block->height)
                        };
                    } else {
                        new_shape = {
                            max(l_shape->block->width, r_shape->block->width),
                            l_shape->block->height + r_shape->block->height
                        };
                    }

                    candidates.push_back({new_shape.first, new_shape.second, i, j});
                }
            }
            
            sort(candidates.begin(), candidates.end(), [](const ShapeKey& a, const ShapeKey& b) {
                return a.width < b.width || (a.width == b.width && a.height < b.height);
            });

            long long min_h = LLONG_MAX;
            long long last_w = LLONG_MAX;
            for (int i = 0; i < candidates.size(); ++i) {
                if (i == 0 || (candidates[i].width != last_w && candidates[i].height < min_h)) {
                    Block* block = new Block(node_name, candidates[i].width, candidates[i].height, 0);
                    node->shapes.push_back(new Shape(block, candidates[i].li, candidates[i].ri));
                    last_w = candidates[i].width;
                    min_h = candidates[i].height;
                }
            }

        } else {
            Block* b0 = new Block(node_name, block_map[node_name]->width, block_map[node_name]->height, 0);
            Block* b1 = new Block(node_name, block_map[node_name]->height, block_map[node_name]->width, 1);
            node->shapes.push_back(new Shape(b0, -1, -1));
            node->shapes.push_back(new Shape(b1, -1, -1));
        }

        st.push(node);
    }

    return st.top();
}


void release_tree(TreeNode* node) {
    if (node == nullptr) return;
    release_tree(node->left);
    release_tree(node->right);
    for (auto shape : node->shapes) {
        delete shape->block;
        delete shape;
    }
    delete node;
}

void calculate_coordinates(TreeNode* node, bool is_left, unordered_map<string, Block*>& block_map) {
    if (node == nullptr) return;
    TreeNode* parent = node->parent;
    if (parent != nullptr) {
        if(is_left){
            node->block->x = parent->block->x;
            node->block->y = parent->block->y;
        }else if(parent->name == "V"){
            node->block->x = parent->block->x + parent->block->width - node->block->width;
            node->block->y = parent->block->y;   
        }else if(parent->name == "H"){
            node->block->x = parent->block->x;
            node->block->y = parent->block->y + parent->block->height - node->block->height;
        }
    }else {
        node->block->x = 0;
        node->block->y = 0;
    }
    if (node->name == "V" || node->name == "H") {
        calculate_coordinates(node->left, true, block_map);
        calculate_coordinates(node->right, false, block_map);
    } else{
        block_map[node->block->name]->x = node->block->x;
        block_map[node->block->name]->y = node->block->y;
        block_map[node->block->name]->orientation = node->block->orientation;
    }
}

bool is_polish_expression(const vector<string> &solution) {
    int operand_count = 0;
    for (const string &s : solution) {
        if (s == "V" || s == "H") {
            operand_count--;
            if (operand_count <= 0) 
                return false;
        } else 
            operand_count++;
    }
    return true;
}

// M1
vector<string> operand_swap(vector<string> &solution, mt19937 &gen) {
    vector<string> new_solution = solution;
    vector<int> adjacent_operands_index;
    for (int i = 0; i < new_solution.size(); i++) {
        if (new_solution[i] != "V" && new_solution[i] != "H") {
            adjacent_operands_index.push_back(i);
        }
    }

    uniform_int_distribution<> dis(0, adjacent_operands_index.size() - 1);
    int index1 = dis(gen);
    int index2 = dis(gen);
    while (index1 == index2) index2 = dis(gen);
    swap(new_solution[adjacent_operands_index[index1]], new_solution[adjacent_operands_index[index2]]);
    return new_solution;
}

// M2
vector<string> chain_invert(vector<string> &solution, mt19937 &gen) {
    vector<string> new_solution = solution;
    vector<vector<int>> chain;
    bool first = true;
    for (int i = 0; i < new_solution.size(); i++) {
        if (new_solution[i] == "V" || new_solution[i] == "H") {
            if(first) {
                chain.push_back({i});
                first = false;
            } else {
                chain.back().push_back(i);
            }
        } else first = true;
    }

    uniform_int_distribution<> dis(0, chain.size() - 1);
    int index = dis(gen);
    for (int i = 0; i < chain[index].size(); i++) {
        new_solution[chain[index][i]] = (new_solution[chain[index][i]] == "V") ? "H" : "V";
    }
    return new_solution;
}

// M3
vector<string> operator_operand_swap(vector<string> &solution, mt19937 &gen) {
    vector<string> new_solution = solution;
    vector<pair<int, int>> operator_index;
    for (int i = 0; i < new_solution.size(); i++) {
        if ((new_solution[i] == "V" || new_solution[i] == "H")) {
            if (i + 1 < new_solution.size() && (new_solution[i + 1] != "V" && new_solution[i + 1] != "H")) { 
                if(i + 2 < new_solution.size() && new_solution[i + 2] == new_solution[i]) continue;
                operator_index.push_back({i, i + 1});
            } else if (i - 1 >= 0 && (new_solution[i - 1] != "V" && new_solution[i - 1] != "H")) {
                if(i - 2 >= 0 && new_solution[i - 2] == new_solution[i]) continue;
                swap(new_solution[i], new_solution[i - 1]);
                if(is_polish_expression(new_solution)) operator_index.push_back({i, i - 1});
                swap(new_solution[i], new_solution[i - 1]);
            }
        }
    }
    if (operator_index.size() == 0) return new_solution;
    uniform_int_distribution<> dis(0, operator_index.size() - 1);
    int index = dis(gen);
    swap(new_solution[operator_index[index].first], new_solution[operator_index[index].second]);
    return new_solution;
}

long long calculate_total_wire_length(unordered_map<string, Net*>& net_map) {
    long long total_wire_length = 0;
    for (auto &net : net_map) {
        long long min_x = INT_MAX, max_x = INT_MIN, min_y = INT_MAX, max_y = INT_MIN;
        for (auto &pin : net.second->pin_to_pad) {
            Pad* pad = pin.second;
            min_x = min(min_x, pad->x);
            max_x = max(max_x, pad->x);
            min_y = min(min_y, pad->y);
            max_y = max(max_y, pad->y);
        }
        for (auto &pin : net.second->pin_to_block) {
            Block* block = pin.second;
            long long x = block->x + (block->orientation == 0 ? block->width / 2 : block->height / 2);
            long long y = block->y + (block->orientation == 0 ? block->height / 2 : block->width / 2);
            min_x = min(min_x, x);
            max_x = max(max_x, x);
            min_y = min(min_y, y);
            max_y = max(max_y, y);
        }
        total_wire_length += (max_x - min_x) + (max_y - min_y);
    }
    return total_wire_length;
}

pair<double, long long> calculate_cost(TreeNode* node, unordered_map<string, Net*>& net_map, int& floorplan_width) {
    long long wire_length = calculate_total_wire_length(net_map);
    long long cost = 0;
    if(node->block->width > floorplan_width) cost += (node->block->width - floorplan_width);
    if(node->block->height > floorplan_width) cost += (node->block->height - floorplan_width);
    return {cost, wire_length};
}

vector<string> build_row_wise_polish_expr2(const vector<string>& blocks, unordered_map<string, Block*>& block_map, int floorplan_width, int stacking_iter) {
    vector<string> result;
    vector<vector<string>> rows;
    unordered_set<Block*> block_used;
    // stacking mode
    while (block_used.size() < blocks.size() && stacking_iter--) {
        vector<string> current_row;
        string base_block;
        for (int i = 0; i < blocks.size(); i++) {
            if (block_used.count(block_map[blocks[i]])) continue;
            base_block = blocks[i];
            break;
        }
        Block* base_blk = block_map[base_block];
        block_used.insert(base_blk);
        long long base_height = min(base_blk->width, base_blk->height);
        long long current_width = max(base_blk->width, base_blk->height);

        current_row.push_back(base_block);

        while (current_width <= floorplan_width && block_used.size() < blocks.size()) {
            long long max_width = 0;
            long long current_height = 0;
            while (block_used.size() < blocks.size()) {
                long long target_width = max_width == 0 ? floorplan_width - current_width : max_width;
                long long width_loss = LLONG_MAX;
                long long min_area = LLONG_MAX;
                Block* blk = nullptr;
                int found = 0;
                for (int i = 0; i < blocks.size(); i++) {
                    const string& candidate = blocks[i];
                    Block* cur_blk = block_map[candidate];
                    if (block_used.count(cur_blk)) continue;
                    long long area = (cur_blk->width * cur_blk->height);
                    if(cur_blk->width <= target_width && current_height + cur_blk->height <= base_height) {
                        long long cur_width_loss = (target_width - cur_blk->width);
                        if (cur_width_loss <= width_loss && area < min_area) {
                            min_area = area;
                            width_loss = cur_width_loss;
                            blk = cur_blk;
                            found = 1;
                        }
                    }
                    if(cur_blk->height <= target_width && current_height + cur_blk->width <= base_height) {
                        long long cur_width_loss = (target_width - cur_blk->height);
                        if (cur_width_loss <= width_loss && area < min_area) {
                            min_area = area;
                            width_loss = cur_width_loss;
                            blk = cur_blk;
                            found = 2;
                        }
                    }
                }
                if (found == 0) break;
                long long blk_width = found == 1 ? blk->width : blk->height;
                long long blk_height = found == 1 ? blk->height : blk->width;
                if(current_width + blk_width > floorplan_width || current_height + blk_height > base_height) break;
                current_height += blk_height;
                block_used.insert(blk);
                current_row.push_back(blk->name);
                if(max_width != 0) current_row.push_back("H");
                max_width = max(max_width, blk_width);
            }
            if(current_row.size() > 1 && max_width > 0) current_row.push_back("V");
            current_width += max_width;
            if (max_width == 0) break;
        }
        rows.push_back(current_row);
    }

    // row-wise mode
    for(int i = 0; i < blocks.size();) {
        if (block_used.count(block_map[blocks[i]])) {
            i++;
            continue;
        }
        vector<string> current_row;
        const string& base_block = blocks[i++];
        Block* base_blk = block_map[base_block];
        block_used.insert(base_blk);
        long long current_width = min(base_blk->width, base_blk->height);
        long long base_height = max(base_blk->width, base_blk->height);

        current_row.push_back(base_block);

        while (current_width <= floorplan_width) {
            int j = i;
            while (j < blocks.size() && block_used.count(block_map[blocks[j]])) j++;
            if (j >= blocks.size()) break;
            const string& candidate = blocks[j];
            Block* blk = block_map[candidate];
            long long blk_width = min(blk->width, blk->height);
            long long blk_height = max(blk->width, blk->height);
            if(current_width + blk_width > floorplan_width || blk_height > base_height) break;
            current_row.push_back(candidate);
            current_width += blk_width;
            if (current_row.size() > 1) current_row.push_back("V");
            block_used.insert(blk);
        }
        rows.push_back(current_row);
    }

    for (int i = 0; i < rows.size(); ++i) {
        auto& row = rows[i];
        result.insert(result.end(), row.begin(), row.end());
        if (i > 0) result.push_back("H");
    }
    return result;
}


vector<string> build_row_wise_polish_expr(const vector<string>& blocks, unordered_map<string, Block*>& block_map, int floorplan_width, int stacking_iter) {
    vector<string> result;
    vector<vector<string>> rows;
    unordered_set<Block*> block_used;
    // stacking mode
    while (block_used.size() < blocks.size() && stacking_iter--) {
        vector<string> current_row;
        string base_block;
        for (int i = 0; i < blocks.size(); i++) {
            if (block_used.count(block_map[blocks[i]])) continue;
            base_block = blocks[i];
            break;
        }
        Block* base_blk = block_map[base_block];
        block_used.insert(base_blk);
        long long base_height = max(base_blk->width, base_blk->height);
        long long current_width = min(base_blk->width, base_blk->height);

        current_row.push_back(base_block);

        while (current_width <= floorplan_width && block_used.size() < blocks.size()) {
            long long max_width = 0;
            long long current_height = 0;
            while (block_used.size() < blocks.size()) {
                long long target_width = max_width == 0 ? floorplan_width - current_width : max_width;
                long long width_loss = LLONG_MAX;
                long long min_area = LLONG_MAX;
                Block* blk = nullptr;
                int found = 0;
                for (int i = 0; i < blocks.size(); i++) {
                    const string& candidate = blocks[i];
                    Block* cur_blk = block_map[candidate];
                    if (block_used.count(cur_blk)) continue;
                    long long area = (cur_blk->width * cur_blk->height);
                    if(cur_blk->width <= target_width && current_height + cur_blk->height <= base_height) {
                        long long cur_width_loss = (target_width - cur_blk->width);
                        if (cur_width_loss <= width_loss && area < min_area) {
                            min_area = area;
                            width_loss = cur_width_loss;
                            blk = cur_blk;
                            found = 1;
                        }
                    }
                    if(cur_blk->height <= target_width && current_height + cur_blk->width <= base_height) {
                        long long cur_width_loss = (target_width - cur_blk->height);
                        if (cur_width_loss <= width_loss && area < min_area) {
                            min_area = area;
                            width_loss = cur_width_loss;
                            blk = cur_blk;
                            found = 2;
                        }
                    }
                }
                if (found == 0) break;
                long long blk_width = found == 1 ? blk->width : blk->height;
                long long blk_height = found == 1 ? blk->height : blk->width;
                if(current_width + blk_width > floorplan_width || current_height + blk_height > base_height) break;
                current_height += blk_height;
                block_used.insert(blk);
                current_row.push_back(blk->name);
                if(max_width != 0) current_row.push_back("H");
                max_width = max(max_width, blk_width);
            }
            if(current_row.size() > 1 && max_width > 0) current_row.push_back("V");
            current_width += max_width;
            if (max_width == 0) break;
        }
        rows.push_back(current_row);
    }

    // row-wise mode
    for(int i = 0; i < blocks.size();) {
        if (block_used.count(block_map[blocks[i]])) {
            i++;
            continue;
        }
        vector<string> current_row;
        const string& base_block = blocks[i++];
        Block* base_blk = block_map[base_block];
        block_used.insert(base_blk);
        long long current_width = min(base_blk->width, base_blk->height);
        long long base_height = max(base_blk->width, base_blk->height);

        current_row.push_back(base_block);

        while (current_width <= floorplan_width) {
            int j = i;
            while (j < blocks.size() && block_used.count(block_map[blocks[j]])) j++;
            if (j >= blocks.size()) break;
            const string& candidate = blocks[j];
            Block* blk = block_map[candidate];
            long long blk_width = min(blk->width, blk->height);
            long long blk_height = max(blk->width, blk->height);
            if(current_width + blk_width > floorplan_width || blk_height > base_height) break;
            current_row.push_back(candidate);
            current_width += blk_width;
            if (current_row.size() > 1) current_row.push_back("V");
            block_used.insert(blk);
        }
        rows.push_back(current_row);
    }

    for (int i = 0; i < rows.size(); ++i) {
        auto& row = rows[i];
        result.insert(result.end(), row.begin(), row.end());
        if (i > 0) result.push_back("H");
    }
    return result;
}

bool is_valid_floorplan(TreeNode* root, int& floorplan_width) {
    return (root->block->width <= floorplan_width && root->block->height <= floorplan_width);
}

int num_hard_blocks = 0, num_pads = 0, num_nets = 0;
long long total_block_area = 0;
double floorplan_area = 0;
int floorplan_width = 0, floorplan_height = 0;

vector<string> best_solution, prev_solution;
unordered_map<string, Block*> block_map;
vector<Block*> block_vec;
unordered_map<string, Pad*> pad_map;
unordered_map<string, Net*> net_map;

mt19937 gen(random_device{}());

// hyperparameters
int k = 15;
double r = 0.98;

// variables for simulated annealing
double T = 2000;
double epsilon = 1;
int N, MT = 0, uphill = 0, reject = 0;
TreeNode *best_root = nullptr, *prev_root = nullptr;
long long best_cost = LLONG_MAX, prev_cost = LLONG_MAX, best_area_cost = LLONG_MAX;
long long total_wire_length = 0;

int main(int argc, char *argv[]) {
    auto start_time = high_resolution_clock::now();
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

    // dead space ratio
    double dead_space_ratio = stod(argv[3]);
    

    vector<string> input_objects = input_preprocess(input_file);
    int object_size = input_objects.size();

    for (int index = 0; index < object_size;) {
        vector<string> object = split(input_objects[index++]);
        if (object.size() == 0)  continue;
        string object_name = object[0];
        if (object_name == "NumHardBlocks") {
            num_hard_blocks = stoi(object[1]);
            N = num_hard_blocks * 10;
            for (int i = 0; i < num_hard_blocks; i++) {
                object = split(input_objects[index++]);
                string hard_block_name = object[1];
                long long width = stoll(object[2]);
                long long height = stoll(object[3]);
                block_map[hard_block_name] = new Block(hard_block_name, width, height);
                block_vec.push_back(block_map[hard_block_name]);
                // Add to total block area
                long long area = width * height;
                total_block_area += area;
            }
        }
        else if(object_name == "NumPads") {
            num_pads = stoi(object[1]);
            for (int i = 0; i < num_pads; i++) {
                object = split(input_objects[index++]);
                string pad_name = object[1];
                int x = stoi(object[2]);
                int y = stoi(object[3]);
                pad_map[pad_name] = new Pad(pad_name, x, y);
            }
        }
        else if(object_name == "NumNets") {
            num_nets = stoi(object[1]);
            for (int i = 0; i < num_nets; i++) {
                object = split(input_objects[index++]);
                string net_name = object[1];
                int num_pins = stoi(object[2]);
                Net* net = new Net(net_name, num_pins);

                for (int j = 0; j < num_pins; j++) {
                    object = split(input_objects[index++]);
                    string pin_name = object[1];
                    if (pin_name[0] == 'p'){
                        net->pin_to_pad[pin_name] = pad_map[pin_name];
                    } else if (pin_name[0] == 'h' && pin_name[1] == 'b'){
                        net->pin_to_block[pin_name] = block_map[pin_name];
                    } else {
                        cout << "Error: invalid pin name " << pin_name << endl;
                        return 1;
                    }
                }
                net_map[net_name] = net;
            }
        }else index++;
    }
    floorplan_area = (double)total_block_area * (1 + dead_space_ratio);
    floorplan_width = floorplan_height = floor(sqrt(floorplan_area));
    cout << "Target Area: " << floorplan_area << endl;

    sort(block_vec.begin(), block_vec.end(), [](Block* a, Block* b) {
        return max(a->width, a->height) > max(b->width, b->height);
    });
    long long current_width = 0;
    int operantors = block_vec.size() - 1, cnt = 0;
    for (auto &block : block_vec) {
        prev_solution.push_back(block->name);
    }
    pair<long long, long long> min_cost = {LLONG_MAX, LLONG_MAX};
    for(int iter = 0; iter < 20; iter++){
        vector<string> new_solution = build_row_wise_polish_expr(prev_solution, block_map, floorplan_width, iter);
        int index = new_solution.size() - 1;
        TreeNode* new_root = build_slicing_tree(new_solution, block_map);
        select_best_path(new_root, floorplan_width);
        calculate_coordinates(new_root, false, block_map);
        pair<long long, long long> cost = calculate_cost(new_root, net_map, floorplan_width);
        cout << "cost: " << cost.first << " wire_length: " << cost.second << endl;
        // if (cost.first <= min_cost.first && num_hard_blocks <= 100 || cost.first < min_cost.first && num_hard_blocks > 100) {
        if (cost.first < min_cost.first) {
            if (best_root != nullptr) release_tree(best_root);
            min_cost = cost;    
            best_area_cost = cost.first;
            best_root = prev_root = new_root;
            best_solution = new_solution;
        } else {
            release_tree(new_root);
        }
    }

    for(int iter = 0; iter < 20; iter++){
        vector<string> new_solution = build_row_wise_polish_expr2(prev_solution, block_map, floorplan_width, iter);
        int index = new_solution.size() - 1;
        TreeNode* new_root = build_slicing_tree(new_solution, block_map);
        select_best_path(new_root, floorplan_width);
        calculate_coordinates(new_root, false, block_map);
        pair<long long, long long> cost = calculate_cost(new_root, net_map, floorplan_width);
        cout << "cost: " << cost.first << " wire_length: " << cost.second << endl;
        // if (cost.first <= min_cost.first && num_hard_blocks <= 100 || cost.first < min_cost.first && num_hard_blocks > 100) {
        if (cost.first < min_cost.first) {
            if (best_root != nullptr) release_tree(best_root);
            min_cost = cost;    
            best_area_cost = cost.first;
            best_root = prev_root = new_root;
            best_solution = new_solution;
        } else {
            release_tree(new_root);
        }
    }


    prev_solution = best_solution;
    calculate_coordinates(best_root, false, block_map);
    min_cost = calculate_cost(best_root, net_map, floorplan_width);
    best_cost = prev_cost = total_wire_length = min_cost.second;
    cout << min_cost.first << endl;
    cout << "Total Wire Length: " << total_wire_length << endl;
    
    while(T >= epsilon) {
        auto now = high_resolution_clock::now();
        auto elapsed = duration_cast<seconds>(now - start_time).count();
        if (elapsed >= 590) {
            cout << "Timeout reached: 590 seconds" << endl;
            break;
        }else cout << "Elapsed time: " << elapsed << " seconds" << endl;
        
        MT = uphill = reject = 0;
        while(MT <= 2 * N && uphill <= N) {
            if (elapsed >= 590) {
                cout << "Timeout reached: 590 seconds" << endl;
                break;
            }
            uniform_int_distribution<> dis(1, 1);
            int method = dis(gen);
            vector<string> new_solution = prev_solution;
            switch(method) {
                case 1:
                    new_solution = operand_swap(new_solution, gen);
                    break;
                case 2:
                    new_solution = chain_invert(new_solution, gen);
                    break;
                case 3:
                    new_solution = operator_operand_swap(new_solution, gen);
                    break;
            }
            MT++;
            TreeNode* new_root = build_slicing_tree(new_solution, block_map);
            method == 4 ? select_random_path(new_root, floorplan_width, gen) : select_best_path(new_root, floorplan_width);
            if(new_root->block->width > floorplan_width || new_root->block->height > floorplan_width) {
                release_tree(new_root);
                reject++;
                continue;
            }
            calculate_coordinates(new_root, false, block_map);
            pair<long long, long long> cost = calculate_cost(new_root, net_map, floorplan_width);
            long long new_wire_length = cost.second;
            long long new_area_cost = cost.first;
            long long delta_cost = new_wire_length - prev_cost;
            double delta = -(double)(delta_cost) / T;
            uniform_real_distribution<> dis3(0, 1);
            double p = dis3(gen);
            if (delta_cost < 0 || exp(delta) > p || (new_area_cost < best_area_cost)) {
                if(delta_cost > 0 && new_area_cost >= best_area_cost) 
                    uphill++;
                if(prev_root != best_root) release_tree(prev_root);
                prev_cost = new_wire_length;
                prev_root = new_root;
                prev_solution = new_solution;
                if(new_wire_length < best_cost || new_area_cost < best_area_cost) {
                    release_tree(best_root);
                    best_cost = new_wire_length;
                    best_root = new_root;
                    best_area_cost = new_area_cost;
                    best_solution = new_solution;
                    total_wire_length = cost.second;
                } 
            } else {
                release_tree(new_root);
                reject++;
            }
        }
        cout << "T: " << T << " best_cost: " << best_cost << " prev_cost: " << prev_cost << endl;
        T *= r;
        if (elapsed >= 590) {
            cout << "Timeout reached: 590 seconds" << endl;
            break;
        }
    }

    output_file << "Wirelength " << total_wire_length << endl;
    output_file << "NumHardBlocks " << num_hard_blocks << endl;

    calculate_coordinates(best_root, false, block_map);
    for (auto &block : block_vec) {
        output_file << block->name << " " << block->x << " " << block->y << " " << block->orientation << endl;
    }
    input_file.close();
    output_file.close();
    return 0;
}