#include "ChildrenTree.hpp"

ChildrenTree::~ChildrenTree() {
    if (!head) {
        return;
    }
    remove_children(head);
    delete head;
    head = nullptr;
}

void ChildrenTree::remove_children(Node* node) {
    if (!node) {
        return;
    }
    for (int i = 0; i < node->number_of_children; i++) {
        remove_children(node->children[i]);
        delete node->children[i];
    }
    node->number_of_children = 0;
}

bool ChildrenTree::add_child(uint32_t child_id) {
    Node* child = new Node(child_id);
    head->children[head->number_of_children++] = child;
    return true;
}

bool ChildrenTree::add_any_child(uint32_t parent_id, uint32_t child_id) {
    bool success = false;
    if (parent_id == id) {
        Node* child = new Node(child_id);
        head->children[head->number_of_children++] = child;
        success = true;
    } else {
        Node* parent = find_node(parent_id, head);
        if (!parent) {
            success = false;
        } else {
            Node* child = new Node(child_id);
            parent->children[parent->number_of_children++] = child;
        }

    }
    return success;
}

ChildrenTree::Node* ChildrenTree::find_node(uint32_t id, Node* head) {
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
bool ChildrenTree::find_path_parent(uint32_t id, uint32_t* parent) {
    if (!node_exists(id)) {
        return false;
    } 
    for (int i = 0; i < head->number_of_children; i++) {
        if (find_node(id, head->children[i]) != nullptr) {
            *parent = head->children[i]->id;
            return true;
        } else if (head->children[i]->id == id) {
            *parent = head->children[i]->id;
            return true;
        }
    }
    return false;
}