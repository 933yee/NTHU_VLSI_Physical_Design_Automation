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
#include "lib.h"

using namespace std;
using namespace std::chrono;

mt19937 gen(42); // Seed for random number generation
//start time
std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
int max_displacement = 0;
int cell_num = 0;
unordered_map<string, Cell*> cells;
int blockage_num = 0;
unordered_map<string, Blockage*> blockages;
int row_num = 0;
unordered_map<string, Row*> rows;

vector<SubRow*> subrows;

// 計算總位移的輔助函數
double calculate_total_displacement() {
    double total_disp = 0;
    for (const auto& cell_pair : cells) {
        Cell* cell = cell_pair.second;
        double dx = cell->x - cell->ori_x;
        double dy = cell->y - cell->ori_y;
        total_disp += sqrt(dx*dx + dy*dy);
    }
    return total_disp;
}

double calculate_actual_width(double width, int site_width) {
    return std::ceil(width / site_width) * site_width;
}

// 模擬退火優化函數 - 添加到你的代碼中
void simulated_annealing_optimization(int max_iterations = 10000, double initial_temp = 5000.0, double cooling_rate = 0.9995) {
    cout << "Starting Simulated Annealing optimization..." << endl;
    double current_cost = calculate_total_displacement();
    double best_cost = current_cost;
    
    
    uniform_real_distribution<double> rand_01(0.0, 1.0);
    uniform_int_distribution<int> operation_choice(0, 1);
    
    vector<Cell*> cell_list;
    for (const auto& cell_pair : cells) {
        cell_list.push_back(cell_pair.second);
    }
    uniform_int_distribution<int> cell_selector(0, cell_list.size() - 1);
    
    unsigned long long iter = 0;
    while(true) {
        if (duration_cast<seconds>(high_resolution_clock::now() - start_time).count() > 55) {
            cout << "Time limit exceeded, stopping optimization." << endl;
            break;
        }
        iter++;
        double new_cost = current_cost;
        
        Cell* cell1 = cell_list[cell_selector(gen)];
        Cell* cell2 = cell_list[cell_selector(gen)];
        
        if (cell1 != cell2 && cell1->width == cell2->width && cell1->height == cell2->height) {
            double orig_x1 = cell1->x, orig_y1 = cell1->y;
            double orig_x2 = cell2->x, orig_y2 = cell2->y;
            
            double dx1 = orig_x2 - cell1->ori_x, dy1 = orig_y2 - cell1->ori_y;
            double dx2 = orig_x1 - cell2->ori_x, dy2 = orig_y1 - cell2->ori_y;
            
            if (sqrt(dx1*dx1 + dy1*dy1) <= max_displacement && 
                sqrt(dx2*dx2 + dy2*dy2) <= max_displacement) {
                
                // 執行交換
                cell1->x = orig_x2; cell1->y = orig_y2;
                cell2->x = orig_x1; cell2->y = orig_y1;
                
                new_cost = calculate_total_displacement();
                double delta = new_cost - current_cost;
                
                if (delta < 0) {
                    current_cost = new_cost;
                    if (new_cost < best_cost) {
                        best_cost = new_cost;
                    }
                } else {
                    cell1->x = orig_x1; cell1->y = orig_y1;
                    cell2->x = orig_x2; cell2->y = orig_y2;
                }
            }
        }
        
        if ((iter + 1) % 100000 == 0) {
            cout << "Current cost: " << (int)current_cost 
                << ", Time elapsed: " << duration_cast<seconds>(high_resolution_clock::now() - start_time).count() << " s" << endl;
            iter = 0; 
        }
    }
    
    cout << "SA optimization completed. Final cost: " << (int)best_cost << endl;
}

void divide_row_by_blockage() {
    cout << "Dividing rows by blockages..." << endl;
    for (const auto& row_pair : rows) {
        Row* row = row_pair.second;
        double row_y_top = row->y + row->height;
        double row_y_bottom = row->y;

        // 初始整個 row 是一段
        vector<pair<double, double>> segments = {{row->x, row->x + row->site_num * row->site_width}};

        for (const auto& blk_pair : blockages) {
            Blockage* blk = blk_pair.second;

            double blk_top = blk->y + blk->height;
            double blk_bottom = blk->y;

            // 與這個 row 有交集才需要處理
            if (blk_bottom >= row_y_top || blk_top <= row_y_bottom) continue;

            double blk_left = blk->x;
            blk_left = row->x +  static_cast<int>(std::floor((blk_left - row->x) / (double)row->site_width)) * row->site_width;

            double blk_right = blk->x + blk->width;
            blk_right = row->x +  static_cast<int>(std::ceil((blk_right - row->x) / (double)row->site_width)) * row->site_width;

            vector<pair<double, double>> new_segments;
            for (auto& seg : segments) {
                if (blk_right <= seg.first || blk_left >= seg.second) {
                    new_segments.push_back(seg); // 無交集
                } else {
                    if (blk_left > seg.first)
                        new_segments.emplace_back(seg.first, blk_left);
                    if (blk_right < seg.second)
                        new_segments.emplace_back(blk_right, seg.second);
                }
            }
            segments = new_segments;
        }

        // 每段合法區域建立 subrow
        for (auto& seg : segments) {
            subrows.push_back(new SubRow(seg.first, seg.second, row->y, row->site_width));
        }
    }
}



bool simulate_collapse_and_check5(Cell* new_cell, SubRow* sub, Cluster* tail_cluster,
                                 double trial_x, double& out_cluster_x,
                                 std::vector<std::pair<Cell*, std::pair<int, int>>>& cell_positions) {
    int site_width = sub->site_width;
    bool new_cluster = (!tail_cluster || tail_cluster->x + tail_cluster->width <= trial_x);

    int total_width = (new_cluster ? 0 : tail_cluster->width) + calculate_actual_width(new_cell->width, site_width);
    double cluster_x = trial_x;
    std::vector<Cell*> simulated_cells;
    if (new_cluster || (tail_cluster->x + total_width > sub->max_x)) {
        return false;
    }
    out_cluster_x = tail_cluster->x;
    if (!new_cluster) {
        double overlap = abs((tail_cluster->x + tail_cluster->width) - cluster_x);
        double displacement = (calculate_actual_width(new_cell->width, site_width) / (double)total_width) * (overlap);
        out_cluster_x -= displacement;
        // out_cluster_x = floor(out_cluster_x / site_width) * site_width;
        out_cluster_x = sub->min_x + static_cast<int>(std::floor((out_cluster_x - sub->min_x) / (double)site_width)) * site_width;
        if (out_cluster_x < sub->min_x) {
            return false;
        }
    }

    Cluster* curr = tail_cluster;
    cluster_x = out_cluster_x;
    simulated_cells.insert(simulated_cells.begin(), curr->members.begin(), curr->members.end());

    curr = curr->prev;
    while (curr) {
        int overlap = (curr->x + curr->width) - cluster_x;
        if (overlap > 0) {
            simulated_cells.insert(simulated_cells.begin(), curr->members.begin(), curr->members.end());
            cluster_x = curr->x - overlap; 
            if (cluster_x < sub->min_x) {
                return false;
            }
            curr = curr->prev;
        } else {
            break;
        }
    }
    

    if (simulated_cells.size() > 1) {
        std::sort(simulated_cells.begin(), simulated_cells.end(), [](Cell* a, Cell* b) {
            return a->x < b->x;
        });
    }
    simulated_cells.push_back(new_cell);

    // int x = (int)(cluster_x / site_width) * site_width;
    int x = sub->min_x +  static_cast<int>(std::floor((cluster_x - sub->min_x) / (double)site_width)) * site_width;
    
    for (Cell* m : simulated_cells) {
        cell_positions.emplace_back(m, std::make_pair(x, sub->y));
         x += calculate_actual_width(m->width, site_width);
    }

    return true;
}


bool simulate_collapse_and_check4(Cell* new_cell, SubRow* sub, Cluster* tail_cluster,
                                 double trial_x, double& out_cluster_x,
                                 std::vector<std::pair<Cell*, std::pair<int, int>>>& cell_positions) {
    int site_width = sub->site_width;
    bool new_cluster = (!tail_cluster || tail_cluster->x + tail_cluster->width <= trial_x);

    int total_width = (new_cluster ? 0 : tail_cluster->width) + calculate_actual_width(new_cell->width, site_width);
    double cluster_x = trial_x;
    std::vector<Cell*> simulated_cells;
    if (new_cluster || (tail_cluster->x + total_width > sub->max_x)) {
        return false;
        cell_positions.emplace_back(new_cell, std::make_pair((int)out_cluster_x, sub->y));
        return true;
    }
    out_cluster_x = tail_cluster->x;
    if (!new_cluster) {
        double displacement ;
        displacement = (calculate_actual_width(new_cell->width, site_width) / (double)total_width) * calculate_actual_width(new_cell->width, site_width);
        out_cluster_x -= calculate_actual_width(new_cell->width, site_width);
        // out_cluster_x = floor(out_cluster_x / site_width) * site_width;
        out_cluster_x = sub->min_x + static_cast<int>(std::floor((out_cluster_x - sub->min_x) / (double)site_width)) * site_width;
        if (out_cluster_x < sub->min_x) {
            return false;
        }
    }

    Cluster* curr = tail_cluster;
    cluster_x = out_cluster_x;
    simulated_cells.insert(simulated_cells.begin(), curr->members.begin(), curr->members.end());

    curr = curr->prev;
    while (curr) {
        int overlap = (curr->x + curr->width) - cluster_x;
        if (overlap > 0) {
            simulated_cells.insert(simulated_cells.begin(), curr->members.begin(), curr->members.end());
            cluster_x = curr->x - overlap; 
            if (cluster_x < sub->min_x) {
                return false;
            }
            curr = curr->prev;
        } else {
            break;
        }
    }
    

    if (simulated_cells.size() > 1) {
        std::sort(simulated_cells.begin(), simulated_cells.end(), [](Cell* a, Cell* b) {
            return a->x < b->x;
        });
    }
    simulated_cells.push_back(new_cell);

    // int x = (int)(cluster_x / site_width) * site_width;
    int x = sub->min_x +  static_cast<int>(std::floor((cluster_x - sub->min_x) / (double)site_width)) * site_width;

    for (Cell* m : simulated_cells) {
        cell_positions.emplace_back(m, std::make_pair(x, sub->y));
         x += calculate_actual_width(m->width, site_width);
    }

    return true;
}

bool simulate_collapse_and_check3(Cell* new_cell, SubRow* sub, Cluster* tail_cluster,
                                 double trial_x, double& out_cluster_x,
                                 std::vector<std::pair<Cell*, std::pair<int, int>>>& cell_positions) {
    int site_width = sub->site_width;
    bool new_cluster = (!tail_cluster || tail_cluster->x + tail_cluster->width <= trial_x);

    int total_width = (new_cluster ? 0 : tail_cluster->width) + calculate_actual_width(new_cell->width, site_width);
    double cluster_x = trial_x;
    std::vector<Cell*> simulated_cells;
    // out_cluster_x = trial_x - tail_cluster->width;
    // 模擬 collapse 舊 cluster
    if (new_cluster || (tail_cluster->x + total_width > sub->max_x)) {
        return false;
        // 不與任何 cluster overlap，直接擺
        cell_positions.emplace_back(new_cell, std::make_pair((int)out_cluster_x, sub->y));
        return true;
    }
    out_cluster_x = tail_cluster->x;
    if (!new_cluster) {
        double displacement ;
        // out_cluster_x = tail_cluster->x + tail_cluster->width;
        displacement = (calculate_actual_width(new_cell->width, site_width) / (double)total_width) * calculate_actual_width(new_cell->width, site_width);
        // displacement = new_cell->width;
        // cout << "displacement: " << displacement << endl;
        out_cluster_x -= displacement;
        // out_cluster_x = floor(out_cluster_x / site_width) * site_width;
        out_cluster_x = sub->min_x + static_cast<int>(std::floor((out_cluster_x - sub->min_x) / (double)site_width)) * site_width;
        if (out_cluster_x < sub->min_x) {
            return false;
        }
    }

    

    // 舊 cluster 模擬 collapse，不修改原本 cluster 結構
    Cluster* curr = tail_cluster;
    cluster_x = out_cluster_x;
    // cell_positions.emplace_back(new_cell, std::make_pair((int)cluster_x, sub->y));
    // total_width += curr->width;
    simulated_cells.insert(simulated_cells.begin(), curr->members.begin(), curr->members.end());

    curr = curr->prev;
    // int overlap_cnt = 0;
    while (curr) {
        int overlap = (curr->x + curr->width) - cluster_x;
        if (overlap > 0) {
            // 將 curr 的 cell 放到最前面
            simulated_cells.insert(simulated_cells.begin(), curr->members.begin(), curr->members.end());
            // total_width += curr->width;
            cluster_x = curr->x - overlap; 
            if (cluster_x < sub->min_x) {
                return false;
            }
            // double cpy_x = cluster_x;
            // for (Cell* m : curr->members) {
            //     cell_positions.emplace_back(m, std::make_pair((int)cpy_x, sub->y));
            //     cpy_x += m->width;
            // }
            curr = curr->prev;
        } else {
            break;
        }
    }
    

    // if(out_cluster_x < sub->min_x ) {
    //     return false;
    // }

    // out_cluster_x = std::max((double)sub->min_x, std::min(cluster_x, (double)(sub->max_x - total_width)));

    // 優化：避免重複排序 - 由於 cells 已經是按 x 排序的，這裡可以更智能地處理
    if (simulated_cells.size() > 1) {
        std::sort(simulated_cells.begin(), simulated_cells.end(), [](Cell* a, Cell* b) {
            return a->x < b->x;
        });
    }
    simulated_cells.push_back(new_cell);

    // sort(cell_positions.begin(), cell_positions.end(), [](const std::pair<Cell*, std::pair<int, int>>& a, const std::pair<Cell*, std::pair<int, int>>& b) {
    //     return a.second.first < b.second.first;
    // });

    // cluster_x += displacement;
    // out_cluster_x += displacement;
    // int x = (int)(cluster_x / site_width) * site_width;
    int x = sub->min_x +  static_cast<int>(std::floor((cluster_x - sub->min_x) / (double)site_width)) * site_width;

    // out_cluster_x = x;
    for (Cell* m : simulated_cells) {
        // if (m->x < sub->min_x || m->x + m->width > sub->max_x) {
        //     return false; 
        // }
        cell_positions.emplace_back(m, std::make_pair(x, sub->y));
         x += calculate_actual_width(m->width, site_width);
    }

    return true;
}

bool simulate_collapse_and_check2(Cell* new_cell, SubRow* sub, Cluster* tail_cluster,
                                 double trial_x, double& out_cluster_x,
                                 std::vector<std::pair<Cell*, std::pair<int, int>>>& cell_positions) {
    bool new_cluster = (!tail_cluster);

    int site_width = sub->site_width;
    int total_width = (new_cluster ? 0 : tail_cluster->width) + calculate_actual_width(new_cell->width, site_width);
    out_cluster_x = std::max((double)sub->min_x, std::min(trial_x, (double)(sub->max_x - total_width)));

    if (new_cluster) {
        // cell_positions.emplace_back(new_cell, std::make_pair((int)out_cluster_x, sub->y));
        return false;
    }else{
        out_cluster_x = tail_cluster->x + tail_cluster->width;
        if (out_cluster_x + calculate_actual_width(new_cell->width, site_width) > sub->max_x) {
            return false;
        }
        cell_positions.emplace_back(new_cell, std::make_pair((int)out_cluster_x, sub->y));
    }

    return true;
}

bool simulate_collapse_and_check(Cell* new_cell, SubRow* sub, Cluster* tail_cluster,
                                 double trial_x, double& out_cluster_x,
                                 std::vector<std::pair<Cell*, std::pair<int, int>>>& cell_positions) {
    int site_width = sub->site_width;
    bool new_cluster = (!tail_cluster || tail_cluster->x + tail_cluster->width <= trial_x);

    int total_width = (new_cluster ? 0 : tail_cluster->width) + calculate_actual_width(new_cell->width, site_width);
    double cluster_x = trial_x;
    std::vector<Cell*> simulated_cells;

    // if (!new_cluster) {
    //     float overlap = (tail_cluster->x + tail_cluster->width) - cluster_x;
    //     float displacement = overlap * (new_cell->width / total_width);
    //     cluster_x += displacement; 
    // }

    // if (!new_cluster) {
    //     float overlap = (tail_cluster->x + tail_cluster->width) - cluster_x;
    //     float displacement = overlap * ((double) new_cell->width / (double) total_width);
    //     cluster_x = std::max(cluster_x + displacement, (double)sub->max_x);
    //     cluster_x = (int)(cluster_x / site_width) * site_width;
    // }

    out_cluster_x = trial_x;
    if (!new_cluster) {
        out_cluster_x = std::min(cluster_x - tail_cluster->width, (double)(sub->max_x - total_width));
    }
    if(out_cluster_x < sub->min_x)  return false;


    // 模擬 collapse 舊 cluster
    if (new_cluster) {
        // 不與任何 cluster overlap，直接擺
        cell_positions.emplace_back(new_cell, std::make_pair((int)out_cluster_x, sub->y));
        return true;
    }

    // 舊 cluster 模擬 collapse，不修改原本 cluster 結構
    Cluster* curr = tail_cluster;
    cluster_x = out_cluster_x;
    // cell_positions.emplace_back(new_cell, std::make_pair((int)cluster_x, sub->y));
    // total_width += curr->width;
    simulated_cells.insert(simulated_cells.begin(), curr->members.begin(), curr->members.end());

    curr = curr->prev;
    // int overlap_cnt = 0;
    while (curr) {
        int overlap = (curr->x + curr->width) - cluster_x;
        if (overlap > 0) {
            // 將 curr 的 cell 放到最前面
            simulated_cells.insert(simulated_cells.begin(), curr->members.begin(), curr->members.end());
            // total_width += curr->width;
            cluster_x = curr->x - overlap; 
            if (cluster_x < sub->min_x) {
                return false;
            }
            // double cpy_x = cluster_x;
            // for (Cell* m : curr->members) {
            //     cell_positions.emplace_back(m, std::make_pair((int)cpy_x, sub->y));
            //     cpy_x += m->width;
            // }
            curr = curr->prev;
        } else {
            break;
        }
    }
    

    // if(out_cluster_x < sub->min_x ) {
    //     return false;
    // }

    // out_cluster_x = std::max((double)sub->min_x, std::min(cluster_x, (double)(sub->max_x - total_width)));

    // 優化：避免重複排序 - 由於 cells 已經是按 x 排序的，這裡可以更智能地處理
    if (simulated_cells.size() > 1) {
        std::sort(simulated_cells.begin(), simulated_cells.end(), [](Cell* a, Cell* b) {
            return a->x < b->x;
        });
    }
    simulated_cells.push_back(new_cell);

    // sort(cell_positions.begin(), cell_positions.end(), [](const std::pair<Cell*, std::pair<int, int>>& a, const std::pair<Cell*, std::pair<int, int>>& b) {
    //     return a.second.first < b.second.first;
    // });
    // int x = (int)(cluster_x / site_width) * site_width;
    int x = sub->min_x +  static_cast<int>(std::floor((cluster_x - sub->min_x) / (double)site_width)) * site_width;

    // out_cluster_x = x;
    for (Cell* m : simulated_cells) {
        cell_positions.emplace_back(m, std::make_pair(x, sub->y));
        x += calculate_actual_width(m->width, site_width);
    }

    return true;
}


void legalize_cells() {
    
    std::vector<Cell*> cell_list;
    cell_list.reserve(cells.size()); // 預分配記憶體
    for (auto& [_, cell] : cells)
        cell_list.emplace_back(cell);
    std::sort(cell_list.begin(), cell_list.end(), [](Cell* a, Cell* b) {
        return a->x < b->x;
    });

    // 優化：預先為每個 row 分組 subrows
    std::unordered_map<Row*, std::vector<SubRow*>> row_subrows;
    std::vector<std::pair<int, Row*>> row_y_sorted;
    for (auto& [_, row] : rows) {
        row_y_sorted.emplace_back(row->y, row);
        row_subrows[row];  // 確保每個 row 都有 entry
    }

    // 將 subrow 按照 y 對應回各 row
    for (SubRow* sub : subrows) {
        for (auto& [y, row] : row_y_sorted) {
            if (sub->y == row->y) {
                row_subrows[row].push_back(sub);
                break;
            }
        }
    }

    std::sort(row_y_sorted.begin(), row_y_sorted.end());

    int max_displacement_sq = max_displacement * max_displacement;
    int cnt = 0;

    cout << "Legalizing cells..." << endl;
    for (Cell* cell : cell_list) {
        SubRow* best_sub = nullptr;
        double best_cost = 1e9;
        double best_x = -1;
        int last_cluster_x = -1;

        // 使用 binary search 找最靠近 cell->y 的 row
        auto it = std::lower_bound(row_y_sorted.begin(), row_y_sorted.end(),
                                   std::make_pair((int)cell->y, nullptr),
                                   [](const auto& a, const auto& b) {
                                       return a.first < b.first;
                                   });
                                   
                             
        
        bool second_strategy = false;
        int it_index = std::distance(row_y_sorted.begin(), it);
        int max_row_range = rows.size() / 2; // 設定最大 row 偏移範圍
        int try_cnt = 0;
        for (int d = 0; d <= max_row_range; ++d) {
            bool out_of_bounds = false;
            bool end_search = false;
            for (int sign : {1, -1}) {
                int offset = d * sign;
                if (offset == 0 && sign == -1) continue; 
                int idx = it_index + offset;
                if (idx < 0 || idx >= (int)row_y_sorted.size()) continue;

                Row* row = row_y_sorted[idx].second;
                if (std::abs(row->y - cell->y) > max_displacement) {
                    out_of_bounds = true;
                    break; 
                }

                // 優化：提前檢查 displacement 限制
                // if (std::abs(row->y - cell->y) > max_displacement) break;
                
                std::vector<SubRow*>& candidate_subrows = row_subrows[row];
                
                // 優化：過濾合適的 subrows，避免不必要的計算
                std::vector<SubRow*> valid_subrows;
                valid_subrows.reserve(candidate_subrows.size());
                for (SubRow* sub : candidate_subrows) {
                    if (calculate_actual_width(cell->width, sub->site_width) <= sub->free_space && sub->max_x - sub->min_x >= calculate_actual_width(cell->width, sub->site_width)) {
                        valid_subrows.push_back(sub);
                    }
                }
                
                if (valid_subrows.empty()) continue;

                for (SubRow* sub : valid_subrows) {
                    // double trial_x = std::max(sub->min_x, std::min(cell->x, sub->max_x - cell->width));
                    // trial_x = std::floor(trial_x / site_width) * site_width;
                    double trial_x = cell->x;
                    if (trial_x < sub->min_x) trial_x = sub->min_x;
                    if (trial_x > sub->max_x - calculate_actual_width(cell->width, sub->site_width)) trial_x = sub->max_x - calculate_actual_width(cell->width, sub->site_width);
                    // trial_x = std::floor(trial_x / sub->site_width) * sub->site_width;
                    trial_x = sub->min_x +  static_cast<int>(std::round((trial_x - sub->min_x) / (double)sub->site_width)) * sub->site_width;

                    Cluster* cluster = sub->last_cluster;
                    std::vector<std::pair<Cell*, std::pair<int, int>>> sim_positions;
                    double sim_cluster_x;
                    bool legal = simulate_collapse_and_check5(cell, sub, cluster, trial_x, sim_cluster_x, sim_positions);
                    if (!legal) continue;
                    int new_x, new_y;
                    bool displacement_ok = true;
                    double dist = 0;

                    for (auto& [c, pos] : sim_positions) {
                        double dx = pos.first - c->ori_x;
                        double dy = pos.second - c->ori_y;
                        double dist_sq = dx * dx + dy * dy; // 避免 sqrt 計算
                        dist += dist_sq;
                        if (dist_sq > max_displacement_sq) {
                            displacement_ok = false;
                            break;
                        }
                        if(c->name == cell->name) {
                            new_x = pos.first;
                            new_y = pos.second;
                        }
                    }
                    if (!displacement_ok) continue;
                    

                    if (dist < best_cost) {
                        second_strategy = false;
                        best_cost = dist;
                        best_sub = sub;
                        best_x = new_x;
                        last_cluster_x = sim_cluster_x;
                        // break;
                    }
                } 

                for (SubRow* sub : valid_subrows) {
                    // double trial_x = std::max(sub->min_x, std::min(cell->x, sub->max_x - cell->width));
                    // trial_x = std::floor(trial_x / site_width) * site_width;
                    double trial_x = cell->x;
                    if (trial_x < sub->min_x) trial_x = sub->min_x;
                    if (trial_x > sub->max_x - calculate_actual_width(cell->width, sub->site_width)) trial_x = sub->max_x - calculate_actual_width(cell->width, sub->site_width);
                    trial_x = sub->min_x +  static_cast<int>(std::round((trial_x - sub->min_x) / (double)sub->site_width)) * sub->site_width;

                    Cluster* cluster = sub->last_cluster;
                    std::vector<std::pair<Cell*, std::pair<int, int>>> sim_positions;
                    double sim_cluster_x;
                    bool legal = simulate_collapse_and_check4(cell, sub, cluster, trial_x, sim_cluster_x, sim_positions);
                    if (!legal) continue;
                    int new_x, new_y;
                    bool displacement_ok = true;
                    double dist = 0;

                    for (auto& [c, pos] : sim_positions) {
                        double dx = pos.first - c->ori_x;
                        double dy = pos.second - c->ori_y;
                        double dist_sq = dx * dx + dy * dy; // 避免 sqrt 計算
                        dist += dist_sq;
                        if (dist_sq > max_displacement_sq) {
                            displacement_ok = false;
                            break;
                        }
                        if(c->name == cell->name) {
                            new_x = pos.first;
                            new_y = pos.second;
                        }
                    }
                    if (!displacement_ok) continue;
                    

                    if (dist < best_cost) {
                        second_strategy = false;
                        best_cost = dist;
                        best_sub = sub;
                        best_x = new_x;
                        last_cluster_x = sim_cluster_x;
                        // break;
                    }
                }   

                for (SubRow* sub : valid_subrows) {
                    // double trial_x = std::max(sub->min_x, std::min(cell->x, sub->max_x - cell->width));
                    // trial_x = std::floor(trial_x / site_width) * site_width;
                    double trial_x = cell->x;
                    if (trial_x < sub->min_x) trial_x = sub->min_x;
                    if (trial_x > sub->max_x - calculate_actual_width(cell->width, sub->site_width)) trial_x = sub->max_x - calculate_actual_width(cell->width, sub->site_width);
                    trial_x = sub->min_x +  static_cast<int>(std::round((trial_x - sub->min_x) / (double)sub->site_width)) * sub->site_width;

                    Cluster* cluster = sub->last_cluster;
                    std::vector<std::pair<Cell*, std::pair<int, int>>> sim_positions;
                    double sim_cluster_x;
                    bool legal = simulate_collapse_and_check3(cell, sub, cluster, trial_x, sim_cluster_x, sim_positions);
                    if (!legal) continue;
                    int new_x, new_y;
                    bool displacement_ok = true;
                    double dist = 0;

                    for (auto& [c, pos] : sim_positions) {
                        double dx = pos.first - c->ori_x;
                        double dy = pos.second - c->ori_y;
                        double dist_sq = dx * dx + dy * dy; // 避免 sqrt 計算
                        dist += dist_sq;
                        if (dist_sq > max_displacement_sq) {
                            displacement_ok = false;
                            break;
                        }
                        if(c->name == cell->name) {
                            new_x = pos.first;
                            new_y = pos.second;
                        }
                    }
                    if (!displacement_ok) continue;
                    

                    if (dist < best_cost) {
                        second_strategy = false;
                        best_cost = dist;
                        best_sub = sub;
                        best_x = new_x;
                        last_cluster_x = sim_cluster_x;
                        // break;
                    }
                }   

                for (SubRow* sub : valid_subrows) {
                    // double trial_x = std::max(sub->min_x, std::min(cell->x, sub->max_x - cell->width));
                    // trial_x = std::floor(trial_x / site_width) * site_width;
                    double trial_x = cell->x;
                    if (trial_x < sub->min_x) trial_x = sub->min_x;
                    if (trial_x > sub->max_x - calculate_actual_width(cell->width, sub->site_width)) trial_x = sub->max_x - calculate_actual_width(cell->width, sub->site_width);
                    trial_x = sub->min_x +  static_cast<int>(std::round((trial_x - sub->min_x) / (double)sub->site_width)) * sub->site_width;

                    Cluster* cluster = sub->last_cluster;
                    std::vector<std::pair<Cell*, std::pair<int, int>>> sim_positions;
                    sim_positions.reserve(10); // 預分配記憶體，避免頻繁 reallocation
                    double sim_cluster_x;
                    bool legal = simulate_collapse_and_check2(cell, sub, cluster, trial_x, sim_cluster_x, sim_positions);
                    if (!legal) continue;
                    
                    int new_x, new_y;
                    bool displacement_ok = true;
                    double dist = 0;

                    for (auto& [c, pos] : sim_positions) {
                        double dx = pos.first - c->ori_x;
                        double dy = pos.second - c->ori_y;
                        double dist_sq = dx * dx + dy * dy; // 避免 sqrt 計算
                        dist += dist_sq;

                        if (dist_sq > max_displacement_sq) {
                            displacement_ok = false;
                            break;
                        }
                        if(c->name == cell->name) {
                            new_x = pos.first;
                            new_y = pos.second;
                        }
                    }
                    if (!displacement_ok) continue;

                    if (dist < best_cost) {
                        second_strategy = true;
                        best_cost = dist;
                        best_sub = sub;
                        best_x = new_x;
                        last_cluster_x = sim_cluster_x;
                        // break;  
                    }
                }
                // if (best_sub) break;
                
                for (SubRow* sub : valid_subrows) {
                    // double trial_x = std::max(sub->min_x, std::min(cell->x, sub->max_x - cell->width));
                    // trial_x = std::floor(trial_x / site_width) * site_width;
                    double trial_x = cell->x;
                    if (trial_x < sub->min_x) trial_x = sub->min_x;
                    if (trial_x > sub->max_x - calculate_actual_width(cell->width, sub->site_width)) trial_x = sub->max_x - calculate_actual_width(cell->width, sub->site_width);
                    // trial_x = std::floor(trial_x / sub->site_width) * sub->site_width;
                    trial_x = sub->min_x +  static_cast<int>(std::round((trial_x - sub->min_x) / (double)sub->site_width)) * sub->site_width;
                    Cluster* cluster = sub->last_cluster;
                    std::vector<std::pair<Cell*, std::pair<int, int>>> sim_positions;
                    double sim_cluster_x;
                    bool legal = simulate_collapse_and_check(cell, sub, cluster, trial_x, sim_cluster_x, sim_positions);
                    if (!legal) continue;
                    int new_x, new_y;
                    bool displacement_ok = true;
                    double dist = 0;
                    for (auto& [c, pos] : sim_positions) {
                        double dx = pos.first - c->ori_x;
                        double dy = pos.second - c->ori_y;
                        double dist_sq = dx * dx + dy * dy; // 避免 sqrt 計算
                        dist += dist_sq;
                        if (dist_sq > max_displacement_sq) {
                            displacement_ok = false;
                            break;
                        }
                        if(c->name == cell->name) {
                            new_x = pos.first;
                            new_y = pos.second;
                        }
                    }
                    if (!displacement_ok) continue;
                    
                    if (dist < best_cost) {
                        second_strategy = false;
                        best_cost = dist;
                        best_sub = sub;
                        best_x = trial_x;
                        last_cluster_x = sim_cluster_x;
                        // break;
                    }
                }
                if (best_sub) {
                    end_search = true;
                    // break; 
                }
            }
            if (out_of_bounds || (end_search && try_cnt++ > 2)) break;
        }

        if (!best_sub) {
            cnt++;
            continue;
        }
        best_sub->free_space -= calculate_actual_width(cell->width, best_sub->site_width);

        Cluster* cluster = best_sub->last_cluster;

        if(second_strategy){
            cluster->members.push_back(cell);
            cluster->width += calculate_actual_width(cell->width, best_sub->site_width);
            cell->x = best_x;
            cell->y = best_sub->y;
            continue;
        }

        bool new_cluster = (!cluster || cluster->x + cluster->width <= best_x);

        if (new_cluster) {
            cluster = new Cluster(best_x, best_sub->last_cluster);
            best_sub->last_cluster = cluster;
            cluster->members.push_back(cell);
            cluster->x = best_x;
            cluster->width = calculate_actual_width(cell->width, best_sub->site_width);
            cell->x = best_x;
            cell->y = best_sub->y;
            continue;
        }

        cluster->width += calculate_actual_width(cell->width, best_sub->site_width);
        cluster->x = last_cluster_x;

        Cluster* head = cluster;

        while (head && head->prev) {
            Cluster* prev = head->prev;
            int overlap = (prev->x + prev->width) - head->x;
            if (overlap > 0) {
                prev->members.insert(prev->members.end(), head->members.begin(), head->members.end());
                // prev->weight += head->weight;
                // prev->q += head->q - head->weight * prev->width;
                prev->width += head->width;
                prev->x -= overlap;
                // delete head;
                head = prev;
            } else {
                break;
            }
        }

        best_sub->last_cluster = head;

        // 優化：減少不必要的排序
        std::vector<Cell*>& all_cells = head->members;
        if (all_cells.size() > 1) {
            std::sort(all_cells.begin(), all_cells.end(), [](Cell* a, Cell* b) {
                return a->x < b->x;
            });
        }
        head->members.push_back(cell);
        
        // int x = (int)(head->x / best_sub->site_width) * best_sub->site_width;
        int x = best_sub->min_x +  static_cast<int>(std::floor((head->x - best_sub->min_x) / (double)best_sub->site_width)) * best_sub->site_width;
        for (Cell* m : all_cells) {
            m->x = x;
            m->y = best_sub->y;
            x += calculate_actual_width(m->width, best_sub->site_width);
        }
    }
    cout << "ERROR: " << cnt << endl;
}


void write_output(ofstream &output_file) {
    double total_disp = 0;
    double max_disp = 0;
    string name = "result";
    for (const auto& cell_pair : cells) {
        Cell* cell = cell_pair.second;
        double dx = cell->x - cell->ori_x;
        double dy = cell->y - cell->ori_y;
        double dist = sqrt(dx*dx + dy*dy);
        total_disp += dist;
        if (dist > max_disp) {
            max_disp = dist;
            name = cell->name; 
        }
    }
    cout << "Name of cell with max displacement: " << name << endl;

    output_file << "TotalDisplacement " << (int)ceil(total_disp) << endl;
    output_file << "MaxDisplacement " << (int)ceil(max_disp) << endl;
    cout << "Total Displacement: " << (int)ceil(total_disp) << endl;
    cout << "Max Displacement: " << (int)ceil(max_disp) << endl;
    output_file << "NumCells " << cell_num << endl;
    for (const auto& cell_pair : cells) {
        const Cell* cell = cell_pair.second;
        output_file << cell->name << " " << (int)cell->x << " " << (int)cell->y << endl;
    }
}

void write_svg(const string& filename, double scale = 0.5) {
    ofstream svg(filename);
    if (!svg.is_open()) {
        cerr << "Error: cannot open SVG file\n";
        return;
    }

    // 掃描所有 cell/blockage，找出 bounding box
    double min_x = 1e9, min_y = 1e9;
    double max_x = -1e9, max_y = -1e9;

    for (auto& [_, cell] : cells) {
        min_x = min(min_x, cell->x);
        min_y = min(min_y, cell->y);
        max_x = max(max_x, cell->x + cell->width);
        max_y = max(max_y, cell->y + cell->height);
    }

    for (auto& [_, blk] : blockages) {
        min_x = min(min_x, blk->x);
        min_y = min(min_y, blk->y);
        max_x = max(max_x, blk->x + blk->width);
        max_y = max(max_y, blk->y + blk->height);
    }

    for (auto& [_, row] : rows) {
        min_x = min(min_x, (double)row->x);
        min_y = min(min_y, (double)row->y);
        max_x = max(max_x, (double) row->x + row->site_num * row->site_width);
        max_y = max(max_y, (double) row->y + row->height);
    }
    // 偏移修正
    double shift_x = -min_x + 10; // +10 for padding
    double shift_y = -min_y + 10;

    double canvas_width = (max_x - min_x) + 20;
    double canvas_height = (max_y - min_y) + 20;

    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << canvas_width * scale
        << "\" height=\"" << canvas_height * scale
        << "\" viewBox=\"0 0 " << canvas_width << " " << canvas_height << "\">\n";
    svg << "<rect width=\"100%\" height=\"100%\" fill=\"white\" />\n";
    svg << "<g transform=\"scale(" << scale << ")\">\n";

    // --- row & site ---
    for (const auto& [_, row] : rows) {
        double row_x = row->x + shift_x;
        double row_y = row->y + shift_y;
        double row_w = row->site_num * row->site_width;
        double row_h = row->height;

        // row 背景
        svg << "<rect x=\"" << row_x << "\" y=\"" << canvas_height - row_y - row_h
            << "\" width=\"" << row_w << "\" height=\"" << row_h
            << "\" fill=\"lightgray\" fill-opacity=\"0.2\" stroke=\"gray\" stroke-width=\"0.5\" />\n";
        // site 線
        for (int i = 0; i < row->site_num; ++i) {
            double site_x = row_x + i * row->site_width;
            svg << "<line x1=\"" << site_x << "\" y1=\"" << canvas_height - row_y
                << "\" x2=\"" << site_x << "\" y2=\"" << canvas_height - row_y - row_h
                << "\" stroke=\"black\" stroke-width=\"0.5\" />\n";
        }
    }

    // --- blockages ---
    for (auto& [_, blk] : blockages) {
        double x = blk->x + shift_x;
        double y = blk->y + shift_y;
        svg << "<rect x=\"" << x << "\" y=\"" << canvas_height - y - blk->height
            << "\" width=\"" << blk->width << "\" height=\"" << blk->height
            << "\" fill=\"rgba(255, 0, 0, 0.5)\" stroke=\"black\" stroke-width=\"0.5\" />\n";

        double center_x = x + blk->width / 2.0;
        double center_y = canvas_height - y - blk->height + blk->height / 2.0 + 3;
        svg << "<text x=\"" << center_x << "\" y=\"" << center_y
            << "\" font-size=\"48\" text-anchor=\"middle\" fill=\"black\">"
            << blk->name << "</text>\n";
    }

    // --- cells ---
    for (auto& [_, cell] : cells) {
        double x = cell->x + shift_x;
        double y = cell->y + shift_y;
        double draw_y = canvas_height - y - cell->height;

        svg << "<rect x=\"" << x << "\" y=\"" << draw_y
            << "\" width=\"" << cell->width << "\" height=\"" << cell->height
            << "\" fill=\"rgba(0, 0, 255, 0.5)\" stroke=\"black\" stroke-width=\"1\" />\n";

        // cell name 中心顯示
        double center_x = x + cell->width / 2.0;
        double center_y = draw_y + cell->height / 2.0 + 3;
        svg << "<text x=\"" << center_x << "\" y=\"" << center_y
            << "\" font-size=\"48\" text-anchor=\"middle\" fill=\"black\">"
            << cell->name << "</text>\n";
    }

    svg << "</g>\n</svg>\n";
    svg.close();
}


vector<string> split(const string& str) {
    vector<string> result;
    result.reserve(8); 
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

int main(int argc, char *argv[]) {
    // Open the input file
    ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        cout << "Error: could not open file " << argv[1] << endl;
        return 1;
    }

    ofstream output_file(argv[2]);
    if (!output_file.is_open()) {
        cout << "Error: could not open file " << argv[2] << endl;
        return 1;
    }

    vector<string> input_objects = input_preprocess(input_file);
    int object_size = input_objects.size();

    for (int index = 0; index < object_size;) {
        vector<string> object = split(input_objects[index++]);
        if (object.size() == 0)  continue;
        string object_name = object[0];
        if (object_name == "MaxDisplacementConstraint") {
            max_displacement = stoi(object[1]);
        }
        else if(object_name == "NumCells") {
            cell_num = stoi(object[1]);
            for (int i = 0; i < cell_num; i++) {
                vector<string> cell_info = split(input_objects[index++]);
                string name = cell_info[1];
                int width = stoi(cell_info[2]);
                int height = stoi(cell_info[3]);
                double x = stod(cell_info[4]);
                double y = stod(cell_info[5]);
                cells[name] = new Cell(name, width, height, x, y);
            }
        }
        else if(object_name == "NumBlockages") {
            blockage_num = stoi(object[1]);
            for (int i = 0; i < blockage_num; i++) {
                vector<string> blockage_info = split(input_objects[index++]);
                string name = blockage_info[1];
                int width = stoi(blockage_info[2]);
                int height = stoi(blockage_info[3]);
                double x = stod(blockage_info[4]);
                double y = stod(blockage_info[5]);
                blockages[name] = new Blockage(name, x, y, width, height);
            }
        }
        else if(object_name == "NumRows") {
            row_num = stoi(object[1]);
            for (int i = 0; i < row_num; i++) {
                vector<string> row_info = split(input_objects[index++]);
                string name = row_info[1];
                int site_width = stoi(row_info[2]);
                int height = stoi(row_info[3]);
                double x = stod(row_info[4]);
                double y = stod(row_info[5]);
                int site_num = stoi(row_info[6]);
                rows[name] = new Row(name, site_width, height, x, y, site_num);
            }
        }
        else index++;
    }

    divide_row_by_blockage();
    legalize_cells();
    cout << "Time taken in seconds: "
         << duration_cast<seconds>(high_resolution_clock::now() - start_time).count() << endl;
    // write_svg("result.svg", 0.5);

    //sa
    cout << "Starting SA optimization..." << endl;
   simulated_annealing_optimization();

    write_output(output_file);
    cout << "Time taken in seconds: "
         << duration_cast<seconds>(high_resolution_clock::now() - start_time).count() << endl;

    input_file.close();
    output_file.close();
    return 0;
}