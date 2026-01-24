#ifndef TREE_H 
#define TREE_H

#include <string>
#include <vector>
#include <utility>

using namespace std;

struct TreeNode;
struct Shape;
struct ShapeKey;


struct Shape{
    Shape(){}
    Shape(Block* block, int left_child_index, int right_child_index) {
        this->left_child_index = left_child_index;
        this->right_child_index = right_child_index;
        this->block = block;
    }
    Block* block;
    int left_child_index = -1, right_child_index = -1;
};

struct ShapeKey {
    int width, height, li, ri;
    bool operator<(const ShapeKey& other) const {
        return tie(width, height, li, ri) < tie(other.width, other.height, other.li, other.ri);
    }
};

struct TreeNode{
    TreeNode(){}
    TreeNode(string name, TreeNode* left, TreeNode* right, TreeNode* parent) {
        this->name = name;
        this->left = left;
        this->right = right;
        this->parent = parent;
    }
    string name;
    TreeNode* left = nullptr;
    TreeNode* right = nullptr;
    TreeNode* parent = nullptr;
    Block* block = nullptr; // for leaf node
    // set<pair<int, int>> shape; // (width, height) 
    vector<Shape*> shapes; 
};

#endif // TREE_H
