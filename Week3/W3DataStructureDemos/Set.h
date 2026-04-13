template<typename T>
class Set {
private:
    struct Node {
        T value;
        Node* left = nullptr;
        Node* right = nullptr;

        Node(const T& newValue) 
            : value(newValue), left(nullptr), right(nullptr) {}

        // Copy constructor
        Node(const Node& other): value(other.value) {
            if (other.left) {
                left = new Node(*other.left);
            }
            if (other.right) {
                right = new Node(*other.right);
            }
        }

        T& operator*() {
            return value;
        }
    };
    Node* root = nullptr;
    int count = 0;
public:
Set() : root(nullptr), count(0) {}

// Destructor
~Set() {
    clear(root);
}

// Copy constructor
Set(const Set& other) : root(nullptr), count(other.count) {
    if (other.root) {
        root = new Node(*other.root);
    }
}

// Copy assignment
Set& operator=(const Set& other) {
    if (this != &other) {
        clear(root);
        if (other.root) {
            root = new Node(*other.root);
            count = other.count;
        } else {
            root = nullptr;
            count = 0;
        }
    }
    return *this;
}

// Move Constructor
Set(Set&& other) 
    : root(other.root), count(other.count) {
    other.root = nullptr;
    other.count = 0;
}

// Move Assignment Operator
Set& operator=(Set&& other) {
    if (this != &other) {
        clear(root);
        root = other.root;
        count = other.count;
        other.root = nullptr;
        other.count = 0;
    }
    return *this;
}

void insert(const T& x) {
    if (root == nullptr) {
        root = new Node(x);
        count++;
        return ;
    }
    insert(root, x);
}

void erase(const T& x) {
    root = erase(root, x);
}

Node* find(const T& x) const {
    return find(root, x);
}

Node* begin() const {
    if (!root) {
        return nullptr;
    }
    Node* it = root;
    while (it->left) {
        it = it->left;
    }
    return it;
}

Node* lower_bound(const T& x) const {
    if (!root) {
        return nullptr;
    }
    Node* ret = nullptr;
    Node* it = root;
    while (it) {
        if (x <= it->value) {
            ret = it;
            it = it->left;
        }
        else {
            it = it->right;
        }
    }
    return ret;
}

int size() const {
    return count;
}

private:
void clear(Node* node) {
    if (node) {
        clear(node->left);
        clear(node->right);
        delete node;
    }
}

Node* insert(Node* node, const T& x) {
    if (!node) {
        count++;
        return new Node(x);
    }
    if (x < node->value) {
        node->left = insert(node->left, x);
    }
    else if (x > node->value) {
        node->right = insert(node->right, x);
    }
    return node;
}

Node* erase(Node* node, const T& x) {
    if (!node) {
        return nullptr;
    }
    if (x == node->value) {
        if (!node->right) {
            Node* temp = node->left;
            count--;
            delete node;
            return temp;
        }
        else if (!node->left) {
            Node* temp = node->right;
            count--;
            delete node;
            return temp;
        }
        Node* successor = node->right;
        while (successor->left) {
            successor = successor->left;
        }
        node->value = successor->value;
        node->right = erase(node->right, successor->value);
        return node;
    }
    if (x < node->value) {
        node->left = erase(node->left, x);
    }
    else {
        node->right = erase(node->right, x);
    }
    return node;
}

Node* find(Node* node, const T& x) const {
    if (!node) {
        return nullptr;
    }
    if (x == node->value) {
        return node;
    }
    if (x < node->value) {
        return find(node->left, x);
    }
    return find(node->right, x);
}
};