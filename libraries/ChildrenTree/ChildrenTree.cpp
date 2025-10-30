#include "ChildrenTree.hpp"
#include "vector"
#include <sstream>
#include <string>
#include <iostream>
#include "display.hpp"


// Node can't have more than 4 children, 3 at most so 4th can get 

ChildrenTree::~ChildrenTree() {
    if (!head) {
        return;
    }
    remove_children(head->id);
    delete head;
    head = nullptr;
}

void ChildrenTree::edit_head(uint32_t new_id) {
    head->id = new_id;
}

/*
void ChildrenTree::remove_children(Node* node) {
    if (!node) {
        return;
    }
    for (int i = 0; i < node->number_of_children; i++) {
        remove_children(node->children[i]);
        delete node->children[i];
    }
    node->number_of_children = 0;
}*/

// bool ChildrenTree::remove_node(uint32_t id) {
//     Node* node = this->find_node(id, this->head);
//     if (node == nullptr) {
//         return false;
//     }

//     Node **children = node->children;
//     Node* newParent = this->head;
//     int childrenToAdd = node->number_of_children;
//     int index = 0;
//     /*
//     while (childrenToAdd > 0) {
//         // Try adding to deleted nodes parent
//         while (newParent->number_of_children < 4) {
//             int id = children[index]->id;
//             this->add_any_child(id, newParent->id);
//             index++;
//             childrenToAdd--;
//         }
        
//     }
//     */

//     //current method for reshaping tree, delete then add node again
//    for (int i = 0; i < childrenToAdd; i++) {
//         int id = children[i]->id;
//         this->remove_node(id);
//         add_child(id);
//    }
//     return true;
// }

void ChildrenTree::remove_children(int id) {
    Node* node = this->find_node(id, this->head);
    for (int i = 0; i < node->number_of_children; i++) {
        for (int j = 0; j < node->children[i]->number_of_children; j++) {
            int childId = node->children[i]->id;
            add_any_child(node->id, childId);
            delete node->children[i]->children[j];
        }
        remove_children(node->children[i]->id);
        delete node->children[i];
    }
    node->number_of_children = 0;
}


bool ChildrenTree::add_child(uint32_t child_id) {
    if(head == nullptr) {return false;}
    Node* child = new Node(child_id);
    Node* temp;
    std::vector<Node*> nodes; 
    nodes.push_back(head);
    bool found = false;
    while(!found && !nodes.empty()) {
        temp = nodes.back();
        if (temp->number_of_children < 3) {
            found = true;
            child->parent = temp;
            temp->children[temp->number_of_children] = child;
            temp->number_of_children++;
        }
        else {
            for (int i = 0; i < temp->number_of_children; i++) {
                nodes.insert(nodes.begin(), temp->children[i]);
            }
        }
        nodes.pop_back();
    }
    nodes.clear();
    return found;
}

bool ChildrenTree::add_any_child(uint32_t parent_id, uint32_t child_id) {
    if(head == nullptr) {return false;}
    Node *parent = find_node(parent_id, head);
    
    
    if (parent ==  nullptr) {return false;}
    std::cout << "found parent node\n";
    if (parent->number_of_children > 3) {
        std::cout<<"Max number of children already reached for node " << parent->id;
        return false;
    }
    Node* child = new Node(child_id);
    std::cout << "Made new node\n";
    parent->children[parent->number_of_children] = child;
    parent->number_of_children++;
    child->parent = parent;
    return true;
}

ChildrenTree::Node* ChildrenTree::find_node(uint32_t id, Node* head) {
    if (head == nullptr) {return nullptr;}
    if (head->id == id) {return head;}
    if (head->number_of_children == 0) {
        return nullptr;
    }
    for (int i = 0; i < head->number_of_children; i++) {
        if (head->children[i]->id == id) {
            return head->children[i];
        } else {
            Node* ret_val = find_node(id, head->children[i]);
            if (ret_val) {
                return ret_val;
            }
        }
    }
    return nullptr;
}

bool ChildrenTree::node_exists(uint32_t id) {
    if (find_node(id, head) != nullptr) {
        return true;
    }
    return false;
}

/**
 * @brief Returns true or false if path exists, and parent is overwritten as the path's parent
 * 
 * @param id node to find
 * @param parent of the path to node to find
 * @return true 
 * @return false 
 */
 bool ChildrenTree::find_path_parent(uint32_t id, uint32_t *parent) {
    DEBUG_printf("Checking tree for parent of the path to %u\n", id);
    if (!head) {
        // either empty tree or id not in tree
        ERROR_printf("head == nullptr");
        return false;
    }

    DEBUG_printf("Children count of head %d", head->number_of_children);
    for (int i = 0; i < head->number_of_children; i++) {
        
        Node* child = head->children[i];
        if (!child) continue;

        if (child->id == id) {
            *parent = child->id;
            return true;
        }
        else if (find_parent_recursive(child, id)) {
            *parent = child->id;
            return true;
        }
    }

    DEBUG_printf("Failed to find path_parent, parent set to -1");
    *parent = UINT32_MAX;
    return false;
}

bool ChildrenTree::find_parent_recursive(Node* node, uint32_t target) {
    for (int i = 0; i < node->number_of_children; ++i) {
        Node* child = node->children[i];
        if (!child) continue;

        if (child->id == target) {
            // this node is the parent of the target
            return true;
        }
        // otherwise, recurse into that child’s subtree
        if (find_parent_recursive(child, target)) {
            return true;
        }
    }
    return false;
}

void ChildrenTree::traverse() {
    Node *root = this->head;
    if (root == nullptr) {
        return;
    }    

    std::vector<Node*> queue;
    queue.push_back(root);

    while (!queue.empty()) {
        Node* current = queue.front();
        queue.erase(queue.begin());

        std::cout << "\nNode ID: " << current->id << " | Children: " << (int)current->number_of_children << " | Parent ID: " << (current->parent ? std::to_string(current->parent->id) : "None") <<std::endl;

        if (current->children != nullptr) {
            for (int i = 0; i < current->number_of_children; ++i) {
                queue.push_back(current->children[i]);
            }
        }
    }
}

void ChildrenTree::get_node_details(ChildrenTree* tree) {
    Node* node = tree->head;
    if (node == this->head) {
        std::cout << "Head\n";
    }
    else {
        std::cout << "Not Head\n";
    }
    
    std::cout << "Number of Children: "<<(int)node->number_of_children;
    if(node->number_of_children == 0) std::cout << "No Children";
    std::cout << "\nID: " << node->id;
}

bool ChildrenTree::get_children(uint32_t parent_id, uint32_t children_id[4], uint8_t &number_of_children) {
    Node* node = find_node(parent_id, head);

    if (!node) return false;

    for (int i = 0; i < node->number_of_children; i++) {
        children_id[i] = node->children[i]->id;
    }
    number_of_children = node->number_of_children;

    return true;
}

bool ChildrenTree::update_node(uint32_t id, uint32_t children_id[4], uint8_t &number_of_children) {
    Node* node = find_node(id, head);
    if (!node) { 
        ERROR_printf("Node with id: %u not found", id);
        return false; 
    }

    for (int i = 0; i < number_of_children; i++) {
        bool found = false;
        for (int j = 0; j < node->number_of_children; j++) {
            if (node->children[j]->id == children_id[i]) {
                found = true;
                break;
            }
        }
        if (found) continue;
        if (!add_any_child(node->id, children_id[i])) return false;
    }

    return true;
}

std::string ChildrenTree::serialize_node(Node* node) {
    std::ostringstream ss;
    ss << "{\"id\":" << node->id << ",\"children\":[";
    for (int i = 0; i < node->number_of_children; i++) {
        if (i > 0) ss << ",";
        ss << serialize_node(node->children[i]);
    }
    ss << "]}";
    return ss.str();
}

std::string ChildrenTree::serialize_tree() {
    if (!head) return "{}";
    return serialize_node(head);
}

void ChildrenTree::send_tree_serial() {
    std::string json = serialize_tree();
    printf("%s\n", json.c_str());   // prints JSON over USB serial (Pico default)
}

void ChildrenTree::move_node(uint32_t id, uint32_t newParentID) {
    // ONLY USE IF YOU WANT ORIGINAL PARENT NODE DELETED
    //Find node that moving
    //Find parent of node moving, remove the parent from the tree is it exisis
    // Find grandparent node and remove parent from its children, update its num of children
    // Find new parent node
    // Add current node to new parent nodes children
    // Set parent node of current node to new parent
    // Update parent nodes number of children
    Node* node = find_node(id, head);
    Node* newParent = find_node(newParentID, head);
    Node* parent = node->parent;
    if (parent != nullptr) {
        // Removing parent
        int index = 0;
        Node* grandparent = parent->parent;
        for (int i = 0; i < grandparent->number_of_children; i++) {
            index++;
            if(grandparent->children[i] == parent) {
                delete grandparent->children[i]; // delete parent
            }
        }   
        //need to shift rest of grandparent child nodes back if needed
        for (int i = index; i < grandparent->number_of_children-1; i++) {
            grandparent->children[i] = grandparent->children[i+1];
        }
        grandparent->children[grandparent->number_of_children-1] = nullptr;
        grandparent->number_of_children--;
    }
    node->parent = newParent;
    newParent->children[newParent->number_of_children] = node;
    newParent->number_of_children++; 
}