#include "ChildrenTree.hpp"

bool ChildrenTree::add_child(uint32_t parent_id, uint32_t child_id) {
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