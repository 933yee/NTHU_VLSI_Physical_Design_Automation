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
#include "Block.h"
#include "BStarTree/BStarTree.hpp"

using namespace std;
using namespace std::chrono;

unordered_map<Node<int>*, unordered_map<int, vector<Node<int>*> > > islandPreorderMap;
unordered_map<Node<int>*, unordered_map<int, vector<Node<int>*> > > islandInorderMap;
unordered_map<Node<int>*, unordered_map<int, unordered_map<string, int> > > islandRotatedMap;
mt19937 gen(3); // Seed for random number generation
//start time
std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();


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

void inorderTraversal(Node<int>* node, vector<Node<int>*>& inorder) {
    if (node == nullptr) return;
    inorderTraversal(node->lchild, inorder);
    inorder.push_back(node);
    inorderTraversal(node->rchild, inorder);
}

void preorderTraversal(Node<int>* node, vector<Node<int>*>& preorder) {
    if (node == nullptr) return;
    preorder.push_back(node);
    preorderTraversal(node->lchild, preorder);
    preorderTraversal(node->rchild, preorder);
}

void directionalLift(Node<int>* parent, Node<int>* target) {
    if (!target) return;

    vector<Node<int>*> chain;
    vector<Node<int>*> parentChain;
    vector<bool> isLeftChild;

    Node<int>* curr = target;
    Node<int>* prevParent = parent;

    std::uniform_int_distribution<int> dist(0, 1);
    bool preferLeft = dist(gen) == 0;

    while (curr) {
        chain.push_back(curr);
        parentChain.push_back(prevParent);
        isLeftChild.push_back(prevParent && prevParent->lchild == curr);

        if (preferLeft && curr->lchild) {
            prevParent = curr;
            curr = curr->lchild;
        } else if (!preferLeft && curr->rchild) {
            prevParent = curr;
            curr = curr->rchild;
        } else if (preferLeft && curr->rchild) {
            preferLeft = false;
            prevParent = curr;
            curr = curr->rchild;
        } else if (!preferLeft && curr->lchild) {
            preferLeft = true;
            prevParent = curr;
            curr = curr->lchild;
        } else {
            break;
        }
    }

    if (chain.size() <= 1) {
        if (parent->lchild == target) parent->lchild = nullptr;
        else if (parent->rchild == target) parent->rchild = nullptr;
        return;
    }

    Node<int>* prevChild = nullptr;
    for (int i = chain.size() - 1; i > 0; --i) {
        Node<int>* currNode = chain[i];
        Node<int>* prevNode = chain[i - 1];
        Node<int>* prevParentNode = parentChain[i - 1];

        bool isLeft = isLeftChild[i - 1];
        if (isLeft) {
            prevParentNode->lchild = currNode;
        } else {
            prevParentNode->rchild = currNode;
        }

        isLeft = isLeftChild[i];

        if (isLeft) {
            currNode->rchild = prevNode->rchild;
            currNode->lchild = prevChild;
        } else {
            currNode->lchild = prevNode->lchild;
            currNode->rchild = prevChild;
        }
        prevChild = currNode;
        prevNode->lchild = nullptr;
        prevNode->rchild = nullptr;
    }
}

void swapTwoNode(vector<Node<int>*>& preorder, vector<Node<int>*>& inorder) {
    if (preorder.size() <= 1) return;

    unordered_map<Node<int>*, Node<int>*> parentMap;
    Node<int>* root = preorder[0];
    function<void(Node<int>*, Node<int>*)> buildParentMap = [&](Node<int>* node, Node<int>* parent) {
        if (!node) return;
        parentMap[node] = parent;
        buildParentMap(node->lchild, node);
        buildParentMap(node->rchild, node);
    };
    buildParentMap(root, nullptr);

    std::uniform_int_distribution<int> dist(0, preorder.size() - 1);

    int index1 = dist(gen);
    int index2 = dist(gen);
    while (index1 == index2) {
        index2 = dist(gen);
    }

    Node<int>* node1 = preorder[index1];
    Node<int>* node2 = preorder[index2];

    Node<int>* parent1 = parentMap[node1];
    Node<int>* parent2 = parentMap[node2];
    if(!parent1 || !parent2) return; // No parent for either node
    if (parent1 == parent2) {
        if (parent1->lchild == node1) {
            parent1->lchild = node2;
            parent1->rchild = node1;
        } else {
            parent1->rchild = node2;
            parent1->lchild = node1;
        }
    } else {
        if (parent1->lchild == node1) {
            parent1->lchild = node2;
        } else {
            parent1->rchild = node2;
        }
        if (parent2->lchild == node2) {
            parent2->lchild = node1;
        } else {
            parent2->rchild = node1;
        }
    }

    // Swap the children of node1 and node2
    Node<int>* tempL = node1->lchild;
    Node<int>* tempR = node1->rchild;
    node1->lchild = node2->lchild;
    node1->rchild = node2->rchild;
    node2->lchild = tempL;
    node2->rchild = tempR;
    
    preorder.clear();
    inorder.clear();
    preorderTraversal(root, preorder);
    inorderTraversal(root, inorder);
}

void perturbReinsertNode(vector<Node<int>*>& preorder, vector<Node<int>*>& inorder) {
    if (preorder.size() <= 2) return;

    unordered_map<Node<int>*, Node<int>*> parentMap;
    Node<int>* root = preorder[0];
    function<void(Node<int>*, Node<int>*)> buildParentMap = [&](Node<int>* node, Node<int>* parent) {
        if (!node) return;
        parentMap[node] = parent;
        buildParentMap(node->lchild, node);
        buildParentMap(node->rchild, node);
    };
    buildParentMap(root, nullptr);
    std::uniform_int_distribution<int> dist(1, preorder.size() - 1);
    Node<int>* target = preorder[dist(gen)];
    Node<int>* parent = parentMap[target];
    
    if (!parent || target->isSymSelf) return; // No parent or target is a symSelf node
    // remove
    
    directionalLift(parent, target);

    // insert to leaf node
    std::uniform_int_distribution<int> rand(0, preorder.size() - 1);
    Node<int>* newParent = preorder[rand(gen)];
    while (newParent == target || newParent == parent || (newParent->lchild && newParent->rchild)) {
        newParent = preorder[rand(gen)];
    }
    if(newParent->lchild == nullptr && newParent->rchild == nullptr) {
        std::uniform_int_distribution<int> rand(0, 1);
        if (rand(gen) == 0) {
            newParent->lchild = target;
        } else {
            newParent->rchild = target;
        }
    } else if (newParent->lchild == nullptr) {
        newParent->lchild = target;
    } else if (newParent->rchild == nullptr) {
        newParent->rchild = target;
    }
    preorder.clear();
    inorder.clear();
    preorderTraversal(root, preorder);
    inorderTraversal(root, inorder);
}

void buildInitialTree3(vector<Node<int>*>& nodes) {
    for(auto& node: nodes) {
        node->lchild = nullptr;
        node->rchild = nullptr;
    }
    unordered_map<Node<int>*, bool> used;
    Node<int>* root = nodes[0];
    used[root] = true;
    Node<int>* currentHead = root;
    Node<int>* current = root;
    int maxWidth = max(root->width, root->height);
    int currentWidth = 0;
    int targetWidth = maxWidth;
    int baseHeight = 0, currentHeight = 0, baseWidth = 0;
    while(used.size() < nodes.size()) {
        bool found = false;
        for(auto& node: nodes) {
            if(current == root) break;
            if(used.count(node)) continue;
            int nodeWidth = max(node->width, node->height);
            if(nodeWidth <= targetWidth) {
                targetWidth -= nodeWidth;
                baseWidth = nodeWidth;
                current->lchild = node;
                current = node;
                used[node] = true;
                found = true;
                currentHeight += min(node->width, node->height);
                break;
            } 
        }
        if(!found) {
            for(auto& node: nodes) {
                if(used.count(node)) continue;
                int nodeWidth = max(node->width, node->height);
                int nodeHeight = min(node->width, node->height);
                currentHead->rchild = node;
                currentHead = node;
                baseHeight = baseHeight + nodeHeight;
                current = node;
                used[node] = true;
                targetWidth = maxWidth - nodeWidth;
                break;
            }
        }else{
            // 符合 baseWidth 和 baseHeight 的 node
            while(true){
                bool found2 = false;
                for(auto& node: nodes) {
                    if(used.count(node)) continue;
                    int nodeWidth = max(node->width, node->height);
                    int nodeHeight = min(node->width, node->height);
                    if(nodeWidth <= baseWidth && currentHeight + nodeHeight <= baseHeight) {
                        current->rchild = node;
                        current = node;
                        used[node] = true;
                        currentHeight += nodeHeight;
                        found2 = true;
                        break;
                    }
                }
                if(!found2) break;
            }
        }
    }
}

void buildInitialTree2(vector<Node<int>*>& nodes) {
    for(auto& node: nodes) {
        node->lchild = nullptr;
        node->rchild = nullptr;
    }
    unordered_map<Node<int>*, bool> used;
    Node<int>* root = nodes[0];
    used[root] = true;
    Node<int>* currentHead = root;
    Node<int>* current = root;
    // 超過 maxWidth 就往上一層放 (right child)
    // 沒有超過 maxWidth 就往右放 (left child)
    int maxWidth = max(root->width, root->height);
    int currentWidth = 0;
    int targetWidth = maxWidth;
    int baseHeight = 0, currentHeight = 0, baseWidth = 0;
    while(used.size() < nodes.size()) {
        bool found = false;
        for(auto& node: nodes) {
            if(current == root) break;
            if(used.count(node)) continue;
            int nodeWidth = max(node->width, node->height);
            if(nodeWidth <= targetWidth) {
                targetWidth -= nodeWidth;
                baseWidth = nodeWidth;
                current->lchild = node;
                current = node;
                used[node] = true;
                found = true;
                currentHeight += min(node->width, node->height);
                break;
            } 
        }
        if(!found) {
            for(auto& node: nodes) {
                if(used.count(node)) continue;
                int nodeWidth = max(node->width, node->height);
                int nodeHeight = min(node->width, node->height);
                currentHead->rchild = node;
                currentHead = node;
                baseHeight = baseHeight + nodeHeight;
                current = node;
                used[node] = true;
                targetWidth = maxWidth - nodeWidth;
                break;
            }
        }else{
            // 符合 baseWidth 和 baseHeight 的 node
            while(true){
                bool found2 = false;
                for(auto& node: nodes) {
                    if(used.count(node)) continue;
                    int nodeWidth = max(node->width, node->height);
                    int nodeHeight = min(node->width, node->height);
                    if(nodeWidth <= baseWidth && currentHeight + nodeHeight <= baseHeight) {
                        current->rchild = node;
                        current = node;
                        used[node] = true;
                        currentHeight += nodeHeight;
                        found2 = true;
                        break;
                    }
                }
                if(!found2) break;
            }
            // currentHeight = 0;
        }
    }
}


void buildInitialTree(vector<Node<int>*>& nodes) {

    for(auto& node: nodes) {
        node->lchild = nullptr;
        node->rchild = nullptr;
    }
    unordered_map<Node<int>*, bool> used;
    Node<int>* root = nodes[0];
    used[root] = true;
    Node<int>* currentHead = root;
    Node<int>* current = root;
    // 超過 maxWidth 就往上一層放 (right child)
    // 沒有超過 maxWidth 就往右放 (left child)
    int maxWidth = max(root->width, root->height);
    int currentWidth = 0;
    int targetWidth = maxWidth;
    int baseHeight = 0, currentHeight = 0, baseWidth = 0;
    while(used.size() < nodes.size()) {
        bool found = false;
        for(auto& node: nodes) {
            if(current == root) break;
            if(used.count(node)) continue;
            int nodeWidth = max(node->width, node->height);
            if(nodeWidth <= targetWidth) {
                targetWidth -= nodeWidth;
                baseWidth = nodeWidth;
                current->lchild = node;
                current = node;
                used[node] = true;
                found = true;
                currentHeight += min(node->width, node->height);
                break;
            } 
        }
        if(!found) {
            for(auto& node: nodes) {
                if(used.count(node)) continue;
                int nodeWidth = max(node->width, node->height);
                int nodeHeight = min(node->width, node->height);
                currentHead->rchild = node;
                currentHead = node;
                baseHeight = currentHeight + nodeHeight;
                current = node;
                used[node] = true;
                targetWidth = maxWidth - nodeWidth;
                break;
            }
        }else{
            // 符合 baseWidth 和 baseHeight 的 node
            while(true){
                bool found2 = false;
                for(auto& node: nodes) {
                    if(used.count(node)) continue;
                    int nodeWidth = max(node->width, node->height);
                    int nodeHeight = min(node->width, node->height);
                    if(nodeWidth <= baseWidth && currentHeight + nodeHeight <= baseHeight) {
                        current->rchild = node;
                        current = node;
                        used[node] = true;
                        currentHeight += nodeHeight;
                        found2 = true;
                        break;
                    }
                }
                if(!found2) break;
            }
            // currentHeight = 0;
        }
    }
}

void findBestInitialSolution(vector<Node<int>*>& islandNodes, vector<Node<int>*>& nodes, mt19937& gen, BStarTree<int>& bst, int& buildMethod) {
    double T = 5;
    double r = 0.85;
    double epsilon = 1;
    int N = nodes.size() * 20, MT = 0, uphill = 0, reject = 0;

    sort(nodes.begin(), nodes.end(), [](Node<int>* a, Node<int>* b) {
        int maxWidthA = a->island ? a->width : max(a->width, a->height);
        int maxWidthB = b->island ? b->width : max(b->width, b->height);
        return maxWidthA > maxWidthB;
    });
    vector<Node<int>*> preorder, inorder;
    buildInitialTree(nodes);
    preorderTraversal(nodes[0], preorder);
    inorderTraversal(nodes[0], inorder);
    int bestSelectionBuildMethod = 0;

    bst.buildTree(preorder, inorder);
    bst.setPosition();
    int bestCost = bst.getArea();
    
    unordered_map<Node<int>*, vector<Node<int>*> > currentIslandPreorderMap;
    unordered_map<Node<int>*, vector<Node<int>*> > currentIlandInorderMap;
    unordered_map<Node<int>*, unordered_map<string, int> > currentIslandRotatedMap;

    for(auto& node: islandNodes){
        currentIslandPreorderMap[node] = node->preorder;
        currentIlandInorderMap[node] = node->inorder;
        for(auto& child: node->preorder){
            currentIslandRotatedMap[node][child->name] = child->rotated;
        }
    }

    unordered_map<Node<int>*, vector<Node<int>*> > bestIslandPreorderMap = currentIslandPreorderMap;
    unordered_map<Node<int>*, vector<Node<int>*> > bestIlandInorderMap = currentIlandInorderMap;
    unordered_map<Node<int>*, unordered_map<string, int> > bestIslandRotatedMap = currentIslandRotatedMap;
    vector<Node<int>*> bestNodes = nodes;

    while(T >= epsilon) {
        MT = uphill = reject = 0;
        while(MT <= 2 * N && uphill <= N) {
            long long prevCost = bst.getArea();

            //preturbInnerNodeCombinations
            auto newIslandPreorderMap = currentIslandPreorderMap;
            auto newIslandInorderMap = currentIlandInorderMap;
            auto newIslandRotatedMap = currentIslandRotatedMap;

            std::uniform_int_distribution<int> rand1(0, islandNodes.size() - 1);
            int i = rand1(gen);
            Node<int>* islandRoot = islandNodes[i];
            vector<int> widths;
            for(auto& width: islandPreorderMap[islandRoot]) widths.push_back(width.first);
            std::uniform_int_distribution<int> rand2(0, widths.size() - 1);
            int width = widths[rand2(gen)];

            auto newPreorder = islandPreorderMap[islandRoot][width];
            auto newInorder = islandInorderMap[islandRoot][width];
            auto currentRotatedStatus = islandRotatedMap[islandRoot][width];

            newIslandPreorderMap[islandRoot] = newPreorder;
            newIslandInorderMap[islandRoot] = newInorder;
            newIslandRotatedMap[islandRoot] = currentRotatedStatus;

            for(auto& node: islandNodes){
                node->preorder = newIslandPreorderMap[node];
                node->inorder = newIslandInorderMap[node];
                for(auto& child: node->preorder){
                    child->rotated = newIslandRotatedMap[node][child->name];
                }
                node->buildIslandTree();
            }
            // islandRoot->buildIslandTree();
            vector<Node<int>*> newNodes = nodes;
            sort(newNodes.begin(), newNodes.end(), [](Node<int>* a, Node<int>* b) {
                int maxWidthA = a->island ? a->width : max(a->width, a->height);
                int maxWidthB = b->island ? b->width : max(b->width, b->height);
                return maxWidthA > maxWidthB;
            });
            
            vector<Node<int>*> preorder, inorder;
            buildInitialTree2(newNodes);
            std::uniform_int_distribution<int> rand3(0, 1);
            int currentSelectionBuildMethod = rand3(gen);
            switch (currentSelectionBuildMethod) {
                case 0:
                    buildInitialTree(newNodes);
                    break;
                case 1:
                    buildInitialTree2(newNodes);
                    break;
                case 2:
                    buildInitialTree3(newNodes);
                    break;
            }
            preorderTraversal(newNodes[0], preorder);
            inorderTraversal(newNodes[0], inorder);
            bst.buildTree(preorder, inorder);
            bst.setPosition();
            int newCost = bst.getArea();
            // cout << "newCost: " << newCost << endl;
            MT++;
            long long deltaCost = newCost - prevCost;
            double delta = -(double)(deltaCost) / T;

            uniform_real_distribution<> dis(0, 1);
            double p = dis(gen);

            if (deltaCost < 0 || exp(delta) > p) {
                if(deltaCost > 0) uphill++;
                currentIslandPreorderMap = newIslandPreorderMap;
                currentIlandInorderMap = newIslandInorderMap;
                currentIslandRotatedMap = newIslandRotatedMap;

                if(newCost < bestCost) {
                    cout << "new bestCost: " << newCost << endl;
                    bestCost = newCost;
                    bestIslandPreorderMap = newIslandPreorderMap;
                    bestIlandInorderMap = newIslandInorderMap;
                    bestIslandRotatedMap = newIslandRotatedMap;
                    bestNodes = newNodes;
                    bestSelectionBuildMethod = currentSelectionBuildMethod;
                }
            } else {
                reject++;
            }
        }
        T *= r;
        // cout << "T: " << T << " bestCost: " << bestCost << endl;
    }
    nodes = bestNodes;
    buildMethod = bestSelectionBuildMethod;
    for(auto& node: islandNodes){
        node->preorder = bestIslandPreorderMap[node];
        node->inorder = bestIlandInorderMap[node];
        for(auto& child: node->preorder){
            child->rotated = bestIslandRotatedMap[node][child->name];
        }
        node->buildIslandTree();
    }
    // bst.buildTree(preorder, inorder);
    // bst.setPosition();
}

void perturbOuterNode(vector<Node<int>*>& preorder, vector<Node<int>*>& inorder, bool isIsland = false) {
    if (preorder.size() <= 1) return;
    std::uniform_int_distribution<int> rand(0, preorder.size() - 1);
    int i = rand(gen);
    std::uniform_int_distribution<int> rand2(0, 1);
    int op = rand2(gen);
    switch (op) {
        case 0:
            if(preorder[i]->symSelf.size() > 0) 
                preorder[i]->rotated = 0;    
            else 
                preorder[i]->rotated ^= 1;
            break;
        case 1:
            perturbReinsertNode(preorder, inorder);
            break;
        case 2:
            swapTwoNode(preorder, inorder);
            break;
    }
}

void postNodesStatus(vector<Node<int>*>& preorder, unordered_map<string, int>& rotated) {
    for (int i = 0; i < preorder.size(); ++i)
        preorder[i]->rotated = rotated[preorder[i]->name];
}

void getNodesStatus(vector<Node<int>*>& preorder, unordered_map<string, int>& rotated) {
    for (int i = 0; i < preorder.size(); ++i)
        rotated[preorder[i]->name] = preorder[i]->rotated;
}

void findMultipleCases(BStarTree<int>& root, vector<Node<int>*>& preorder, vector<Node<int>*>& inorder, mt19937& gen, int recordedBestCost, Node<int>* islandRoot) {
    double T = 10000;
    double r = 0.95;
    double epsilon = 1;
    int N = preorder.size() * 50, MT = 0, uphill = 0, reject = 0;

    vector<Node<int>*> bestPreorder = preorder;
    vector<Node<int>*> bestInorder = inorder;
    unordered_map<string, int> bestRotatedStatus;
    getNodesStatus(bestPreorder, bestRotatedStatus);

    vector<Node<int>*> currentPreorder = bestPreorder;
    vector<Node<int>*> currentInorder = bestInorder;
    unordered_map<string, int> currentRotatedStatus = bestRotatedStatus;
    while(T >= epsilon) {
        MT = uphill = reject = 0;
        while(MT <= 2 * N && uphill <= N) {
            long long prevCost = root.getArea();
            auto newPreorder = currentPreorder;
            auto newInorder = currentInorder;
            postNodesStatus(newPreorder, currentRotatedStatus);
            root.buildTree(newPreorder, newInorder);
            
            perturbOuterNode(newPreorder, newInorder, true);
            root.buildTree(newPreorder, newInorder);
            root.setPosition();
            int newCost = root.getArea();

            MT++;
            long long deltaCost = newCost - prevCost;
            double delta = -(double)(deltaCost) / T;

            uniform_real_distribution<> dis(0, 1);
            double p = dis(gen);

            // if (deltaCost <= 0 || exp(delta) > p) {
                if(deltaCost > 0) uphill++;
                currentPreorder = newPreorder;
                currentInorder = newInorder;
                getNodesStatus(currentPreorder, currentRotatedStatus);

                if(newCost <= recordedBestCost) {
                    int width = root.getWidthHeight(root.root).first;
                    islandPreorderMap[islandRoot][width] = newPreorder;
                    islandInorderMap[islandRoot][width] = newInorder;
                    islandRotatedMap[islandRoot][width] = currentRotatedStatus;
                }

            // } else {
                // reject++;
            // }
        }
        T *= r;
    }

    postNodesStatus(bestPreorder, bestRotatedStatus);
    root.buildTree(bestPreorder, bestInorder);
    root.setPosition();

    preorder = bestPreorder;
    inorder = bestInorder;
    root.buildTree(preorder, inorder);
    root.setPosition();
}

int simulatedAnnealing(BStarTree<int>& root, vector<Node<int>*>& preorder, vector<Node<int>*>& inorder, mt19937& gen, bool isIsland = false) {
    double T = 10;
    double r = 0.95;
    double epsilon = 1;
    int N = preorder.size() * 40, MT = 0, uphill = 0, reject = 0;
    if(isIsland){
        
    }else{
        T = 10000000;
        r = 0.999;
        int sz = preorder.size();
        N = min(15, sz);
        epsilon = 1;
    }
    vector<Node<int>*> bestPreorder = preorder;
    vector<Node<int>*> bestInorder = inorder;
    unordered_map<string, int> bestRotatedStatus;
    int bestCost = root.getArea();
    getNodesStatus(bestPreorder, bestRotatedStatus);

    vector<Node<int>*> currentPreorder = bestPreorder;
    vector<Node<int>*> currentInorder = bestInorder;
    unordered_map<string, int> currentRotatedStatus = bestRotatedStatus;
    while(T >= epsilon) {
        auto now = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(now - start_time);
        if (duration.count() > 280) {
            cout << "Time limit exceeded" << endl;
            break;
        }
        MT = uphill = reject = 0;
        while(MT <= 2 * N && uphill <= N) {
            long long prevCost = root.getArea();
            auto newPreorder = currentPreorder;
            auto newInorder = currentInorder;
            postNodesStatus(newPreorder, currentRotatedStatus);
            root.buildTree(newPreorder, newInorder);
            
            perturbOuterNode(newPreorder, newInorder, isIsland);
            root.buildTree(newPreorder, newInorder);
            root.setPosition();
            int newCost = root.getArea();

            MT++;
            long long deltaCost = newCost - prevCost;
            double delta = -(double)(deltaCost) / T;

            uniform_real_distribution<> dis(0, 1);
            double p = dis(gen);
            if (deltaCost <= 0 || exp(delta) > p) {
                if(deltaCost > 0) uphill++;
                currentPreorder = newPreorder;
                currentInorder = newInorder;
                getNodesStatus(currentPreorder, currentRotatedStatus);

                if(newCost <= bestCost) {
                    bestCost = newCost;
                    bestPreorder = newPreorder;
                    bestInorder = newInorder;
                    bestRotatedStatus = currentRotatedStatus;
                }
            } else {
                reject++;
            }
        }
        // cout << "T: " << T << " best_cost: " << bestCost << " prev_cost: " << root.getArea() << endl;
        if(!isIsland) {
            currentPreorder = bestPreorder;
            currentInorder = bestInorder;
            currentRotatedStatus = bestRotatedStatus;
            postNodesStatus(currentPreorder, currentRotatedStatus);
            root.buildTree(currentPreorder, currentInorder);
            root.setPosition();
        }
        T *= r;
    }

    postNodesStatus(bestPreorder, bestRotatedStatus);
    root.buildTree(bestPreorder, bestInorder);
    root.setPosition();

    preorder = bestPreorder;
    inorder = bestInorder;
    root.buildTree(preorder, inorder);
    root.setPosition();
    return bestCost;
}

unordered_map<string, Block*> block_map;
unordered_map<string, SymGroup*> sym_group_map;
unordered_map<string, string> pair_map;

vector<Node<int>*> islandNodes;

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
        if (object_name == "NumHardBlocks") {
            int num_blocks = stoi(object[1]);
            for (int i = 0; i < num_blocks; i++) {
                object = split(input_objects[index++]);
                string blcok_name = object[1];
                int width = stoi(object[2]);
                int height = stoi(object[3]);
                Block* block = new Block(blcok_name, width, height);
                block_map[blcok_name] = block;
            }
        }
        else if(object_name == "NumSymGroups") {
            int num_groups = stoi(object[1]);
            for (int i = 0; i < num_groups; i++) {
                object = split(input_objects[index++]);
                string group_name = object[1];
                int num_blocks = stoi(object[2]);

                SymGroup* group = new SymGroup(group_name, num_blocks);
                sym_group_map[group_name] = group;
                
                for (int j = 0; j < num_blocks; j++) {
                    object = split(input_objects[index++]);
                    string object_name = object[0];
                    if(object_name == "SymPair"){
                        string block_name_1 = object[1];
                        string block_name_2 = object[2];
                        Block* block_1 = block_map[block_name_1];
                        Block* block_2 = block_map[block_name_2];
                        block_1->isOrphan = false;
                        block_2->isOrphan = false;
                        SymPair* pair = new SymPair(block_1, block_2);
                        group->symPairs.push_back(pair);
                        pair_map[block_name_1] = block_name_2;
                    } else if (object_name == "SymSelf"){
                        string block_name = object[1];
                        Block* block = block_map[block_name];
                        block->isOrphan = false;
                        group->symSelf.push_back(block);
                    }
                }
            }
        }
        else index++;
    }
    // test output
    // for (auto& block_pair : block_map) {
    //     Block* block = block_pair.second;
    //     output_file << "Block Name: " << block->name << ", Width: " << block->width << ", Height: " << block->height << endl;
    // }

    // for (auto& group_pair : sym_group_map) {
    //     SymGroup* group = group_pair.second;
    //     output_file << "SymGroup Name: " << group->name << ", Size: " << group->size << endl;
    //     for (auto& pair : group->symPairs) {
    //         output_file << "SymPair: (" << pair->block1->name << ", " << pair->block2->name << ")" << endl;
    //     }
    //     for (auto& block : group->symSelf) {
    //         output_file << "SymSelf: " << block->name << endl;
    //     }
    // }

    using BNode = Node<int>;
    vector<BNode*> nodes;
    
    // 把孤兒 block 變成 node
    for (auto& [name, block] : block_map) {
        BNode* node = new BNode();
        if (block->isOrphan) {
            node->setShape(block->width, block->height);
            node->name = block->name;  
            node->rotated = block->width > block->height ? 0 : 1;
            nodes.push_back(node);
        } 
    }

    // 把 symGroup 變成 node
    for (auto& [name, group] : sym_group_map) {
        BNode* node = new BNode();
        node->name = group->name;
        nodes.push_back(node);
        node->initIsland(group);
        islandNodes.push_back(node);
        // node->rotated = node->width > node->height ? 0 : 1;
    }

    for (auto& node : nodes) {
        int innerBestCost;
        if(node->island){
            innerBestCost = simulatedAnnealing(*node->islandTree, node->preorder, node->inorder, gen, true);
            findMultipleCases(*node->islandTree, node->preorder, node->inorder, gen, innerBestCost, node);
            node->buildIslandTree();
        }
    }

    BStarTree<int> bst;
    int buildMethod = 0;
    findBestInitialSolution(islandNodes, nodes, gen, bst, buildMethod);

    vector<Node<int>*> preorder, inorder;
    switch (buildMethod) {
        case 0:
            buildInitialTree(nodes);
            break;
        case 1:
            buildInitialTree2(nodes);
            break;
        case 2:
            buildInitialTree3(nodes);
            break;
    }
    preorderTraversal(nodes[0], preorder);
    inorderTraversal(nodes[0], inorder);
    bst.buildTree(preorder, inorder);
    bst.setPosition();
    simulatedAnnealing(bst, preorder, inorder, gen);

    bst.buildTree(preorder, inorder);
    bst.setPosition();
    
    int area = bst.getArea();
    cout << "Area: " << area << endl;
    output_file << "Area " << area << endl << endl;
    output_file << "NumHardBlocks " << block_map.size() << endl;
    for (BNode* node : nodes) {
        if(node->island) {
            for(auto& innerNode : node->symPairs){
                int innerWidth = (innerNode->rotated == 0) ? innerNode->width : innerNode->height;
                int innerHeight = (innerNode->rotated == 0) ? innerNode->height : innerNode->width;
                if(node->rotated == 0) {
                    int x = node->x + node->width / 2 + innerNode->x;
                    int y = innerNode->y + node->y;
                    output_file << innerNode->name << " " << x << " " << y << " " << innerNode->rotated << endl;
                    int T = x - (node->x + node->width / 2);
                    int x2 = x - 2 * T - innerWidth;
                    int rotated;
                    if(innerNode->rotated == 0) {
                        if(block_map[innerNode->name]->width == block_map[pair_map[innerNode->name]]->width) {
                            rotated = 0;
                        } else {
                            rotated = 1;
                        }
                    } else {
                        if(block_map[innerNode->name]->height == block_map[pair_map[innerNode->name]]->width) {
                            rotated = 0;
                        } else {
                            rotated = 1;
                        }
                    }
                    output_file << pair_map[innerNode->name] << " " << x2 << " " << y << " " << rotated << endl;
                } else {
                    int x = innerNode->x + node->x;
                    int y = node->y + node->height / 2 + innerNode->y;
                    output_file << innerNode->name << " " << x << " " << y << " " << innerNode->rotated << endl;
                    int T = y - (node->y + node->height / 2);
                    int y2 = y - 2 * T - innerHeight;
                    int rotated;
                    if(innerNode->rotated == 0) {
                        if(block_map[innerNode->name]->width == block_map[pair_map[innerNode->name]]->width) {
                            rotated = 0;
                        } else {
                            rotated = 1;
                        }
                    } else {
                        if(block_map[innerNode->name]->height == block_map[pair_map[innerNode->name]]->width) {
                            rotated = 0;
                        } else {
                            rotated = 1;
                        }
                    }
                    output_file << pair_map[innerNode->name] << " " << x << " " << y2 << " " << rotated << endl;
                }
            }
            for(auto& innerNode : node->symSelf) {
                // cout << "innerNode: " << innerNode->name << endl;
                // cout << "innerNode->x: " << innerNode->x << endl;
                // cout << "node->x: " << node->x << endl;
                // cout << "node->width: " << node->width << endl;
                // cout << "innerNode->width: " << innerNode->width << endl;
                // cout << "innerNode->rotated: " << innerNode->rotated << endl;
                if(node->rotated == 0 && innerNode->rotated == 0) {
                    output_file << innerNode->name << " " << innerNode->x + (node->x + node->width / 2) - innerNode->symSelfWidth / 2 << " " << innerNode->y + node->y << " " << innerNode->rotated << endl;
                } else if(node->rotated == 0 && innerNode->rotated == 1) {
                    output_file << innerNode->name << " " << innerNode->x + (node->x + node->width / 2) - innerNode->symSelfHeight / 2 << " " << innerNode->y + node->y << " " << innerNode->rotated << endl;
                    // output_file << innerNode->name << " " << innerNode->x + node->x << " " << innerNode->y + (node->y + node->height / 2) - innerNode->height << " " << innerNode->rotated << endl;
                } else if(node->rotated == 1 && innerNode->rotated == 0) {
                    output_file << innerNode->name << " " << innerNode->x + node->x << " " << innerNode->y + (node->y + node->height / 2) - innerNode->symSelfHeight / 2 << " " << innerNode->rotated << endl;
                } else {
                    output_file << innerNode->name << " " << innerNode->x + node->x << " " << innerNode->y + (node->y + node->height / 2) - innerNode->symSelfWidth / 2 << " " << innerNode->rotated << endl;
                }
            }
        }
        else 
            output_file << node->name << " " << node->x << " " << node->y << " " << node->rotated << endl;
    }
    

    input_file.close();
    output_file.close();
    return 0;
}