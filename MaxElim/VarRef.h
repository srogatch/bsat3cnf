#pragma once

#include "AVLTree.h"

struct VarRefCommon {
  AVLNodePool _avlNp;
  int64_t _N;
  void Init(const int64_t N) {
    _N = N;
  }
};

struct VarRef {
  FastVector<AVLTree> _trees;

private:
  VarRefCommon *_pVrc;
  int64_t _var;
  std::vector<int64_t> *_pTraversed;

private:
  AVLNode & getNode(const int64_t iNode) {
    return _pVrc->_avlNp._nodes[iNode];
  }

  //// Taken from: https://www.geeksforgeeks.org/avl-tree-set-1-insertion/
  // A utility function to get the height of the tree 
  int64_t height(const int64_t iNode) {
    if (iNode < 0) {
      return 0;
    }
    return getNode(iNode)._height;
  }

  /* Helper function that allocates a new node with the given key and
  NULL left and right pointers. */
  int64_t newNode(const int64_t key) {
    const int64_t index = _pVrc->_avlNp.Acquire();
    AVLNode& node = getNode(index);
    node._key = key;
    node._iLeft = -1;
    node._iRight = -1;
    node._height = 1;  // new node is initially added at leaf 
    return index;
  }

  // A utility function to right rotate subtree rooted with y 
  // See the diagram given above. 
  int64_t rightRotate(const int64_t y) {
    const int64_t x = getNode(y)._iLeft;
    const int64_t T2 = getNode(x)._iRight;

    // Perform rotation
    getNode(x)._iRight = y;
    getNode(y)._iLeft = T2;

    // Update heights 
    getNode(y)._height = std::max(height(getNode(y)._iLeft), height(getNode(y)._iRight)) + 1;
    getNode(x)._height = std::max(height(getNode(x)._iLeft), height(getNode(x)._iRight)) + 1;

    // Return new root 
    return x;
  }

  // A utility function to left rotate subtree rooted with x 
  // See the diagram given above. 
  int64_t leftRotate(const int64_t x) {
    const int64_t y = getNode(x)._iRight;
    const int64_t T2 = getNode(y)._iLeft;

    // Perform rotation 
    getNode(y)._iLeft = x;
    getNode(x)._iRight = T2;

    //  Update heights 
    getNode(x)._height = std::max(height(getNode(x)._iLeft), height(getNode(x)._iRight)) + 1;
    getNode(y)._height = std::max(height(getNode(y)._iLeft), height(getNode(y)._iRight)) + 1;

    // Return new root 
    return y;
  }

  // Get Balance factor of node N 
  int64_t getBalance(const int64_t iNode) {
    if (iNode < 0) {
      return 0;
    }
    return height(getNode(iNode)._iLeft) - height(getNode(iNode)._iRight);
  }

  // Recursive function to insert a key in the subtree rooted 
  // with node and returns the new root of the subtree. 
  int64_t insert(const int64_t iNode, const int64_t key) {
    /* 1.  Perform the normal BST insertion */
    if (iNode < 0)
      return newNode(key);

    if (key < getNode(iNode)._key) {
      const int64_t newLeft = insert(getNode(iNode)._iLeft, key);
      getNode(iNode)._iLeft = newLeft;
    }
    else if (key > getNode(iNode)._key) {
      const int64_t newRight = insert(getNode(iNode)._iRight, key);
      getNode(iNode)._iRight = newRight;
    }
    else {
      fprintf(stderr, "Duplicate entry of variable %lld in clause %lld.\n", _var, key);
      quick_exit(7);
      //return iNode; // we would return this if it wouldn't be an error
    }

    /* 2. Update height of this ancestor node */
    getNode(iNode)._height = 1 + std::max(height(getNode(iNode)._iLeft), height(getNode(iNode)._iRight));

    /* 3. Get the balance factor of this ancestor
    node to check whether this node became
    unbalanced */
    const int64_t balance = getBalance(iNode);

    // If this node becomes unbalanced, then 
    // there are 4 cases 

    if (balance > 1) {
      if (key < getNode(getNode(iNode)._iLeft)._key) {
        // Left Left Case
        return rightRotate(iNode);
      }
      else {
        // Left Right Case
        getNode(iNode)._iLeft = leftRotate(getNode(iNode)._iLeft);
        return rightRotate(iNode);
      }
    }

    if (balance < -1) {
      if (key > getNode(getNode(iNode)._iRight)._key) {
        // Right Right Case
        return leftRotate(iNode);
      }
      else {
        // Right Left Case
        getNode(iNode)._iRight = rightRotate(getNode(iNode)._iRight);
        return leftRotate(iNode);
      }
    }

    /* return the (unchanged) node pointer */
    return iNode;
  }

  void traverse(const int64_t iParent) {
    if (iParent < 0) {
      return;
    }
    traverse(getNode(iParent)._iRight);
    _pTraversed->push_back(getNode(iParent)._key);
    traverse(getNode(iParent)._iLeft);
  }

  int64_t findNode(const int64_t iParent, const int64_t key) {
    if (iParent < 0) {
      return -1;
    }
    if (key < getNode(iParent)._key) {
      return findNode(getNode(iParent)._iLeft, key);
    }
    else if (key > getNode(iParent)._key) {
      return findNode(getNode(iParent)._iRight, key);
    }
    // Found
    return iParent;
  }

  /* Given a non-empty binary search tree, return the
  node with minimum key value found in that tree.
  Note that the entire tree does not need to be
  searched. */
  int64_t minValueNode(const int64_t iNode) {
    int64_t current = iNode;

    /* loop down to find the leftmost leaf */
    while (getNode(current)._iLeft >= 0) {
      current = getNode(current)._iLeft;
    }

    return current;
  }

  //// Taken from https://www.geeksforgeeks.org/avl-tree-set-2-deletion/
  // Recursive function to delete a node with given key 
  // from subtree with given root. It returns root of 
  // the modified subtree. 
  int64_t deleteNode(int64_t root, const int64_t key) {
    // STEP 1: PERFORM STANDARD BST DELETE 

    if (root < 0) {
      fprintf(stderr, "Cannot find to delete variable %lld in clause %lld.\n", _var, key);
      quick_exit(8);
      //return root; // we would return this if it wouldn't be an error
    }

    // If the key to be deleted is smaller than the 
    // root's key, then it lies in left subtree 
    if (key < getNode(root)._key) {
      getNode(root)._iLeft = deleteNode(getNode(root)._iLeft, key);
    }
    // If the key to be deleted is greater than the 
    // root's key, then it lies in right subtree 
    else if (key > getNode(root)._key) {
      getNode(root)._iRight = deleteNode(getNode(root)._iRight, key);
    }
    // if key is same as root's key, then This is 
    // the node to be deleted 
    else {
      // node with only one child or no child 
      if ((getNode(root)._iLeft < 0) || (getNode(root)._iRight < 0)) {
        int64_t temp = (getNode(root)._iLeft >= 0) ? getNode(root)._iLeft : getNode(root)._iRight;

        // No child case 
        if (temp < 0) {
          temp = root;
          root = -1;
        }
        else { // One child case 
          // Copy the contents of the non-empty child 
          getNode(root) = getNode(temp);
        }
        _pVrc->_avlNp.Release(temp);
      }
      else {
        // node with two children: Get the inorder 
        // successor (smallest in the right subtree) 
        const int64_t temp = minValueNode(getNode(root)._iRight);

        // Copy the inorder successor's data to this node 
        getNode(root)._key = getNode(temp)._key;

        // Delete the inorder successor 
        getNode(root)._iRight = deleteNode(getNode(root)._iRight, getNode(temp)._key);
      }
    }

    // If the tree had only one node then return 
    if (root < 0)
      return root;

    // STEP 2: UPDATE HEIGHT OF THE CURRENT NODE 
    getNode(root)._height = 1 + std::max(height(getNode(root)._iLeft), height(getNode(root)._iRight));

    // STEP 3: GET THE BALANCE FACTOR OF THIS NODE (to 
    // check whether this node became unbalanced) 
    const int64_t balance = getBalance(root);

    // If this node becomes unbalanced, then there are 4 cases 

    if (balance > 1) {
      if (getBalance(getNode(root)._iLeft) >= 0) {
        // Left Left Case 
        return rightRotate(root);
      }
      else {
        // Left Right Case 
        getNode(root)._iLeft = leftRotate(getNode(root)._iLeft);
        return rightRotate(root);
      }
    }

    if (balance < -1) {
      if (getBalance(getNode(root)._iRight) <= 0) {
        // Right Right Case 
        return leftRotate(root);
      }
      else {
        // Right Left Case 
        getNode(root)._iRight = rightRotate(getNode(root)._iRight);
        return leftRotate(root);
      }
    }

    return root;
  }

public:
  void Init(const VarRefCommon& vrc) {
    for (int64_t i = 0; i < 2 * vrc._N + 1; i++) {
      _trees.emplace_back();
      _trees.back()._iRoot = -1;
      _trees.back()._size = 0;
    }
  }

  void Add(const int64_t var, const int64_t iClause, VarRefCommon &vrc) {
    _var = var;
    _pVrc = &vrc;
    const int64_t iTree = vrc._N + var;
    _trees[iTree]._iRoot = insert(_trees[iTree]._iRoot, iClause);
    _trees[iTree]._size++;
    _pVrc = nullptr;
  }

  void Del(const int64_t var, const int64_t iClause, VarRefCommon &vrc) {
    _pVrc = &vrc;
    _var = var;
    const int64_t iTree = vrc._N + var;
    _trees[iTree]._iRoot = deleteNode(_trees[iTree]._iRoot, iClause);
    _trees[iTree]._size--;
    _pVrc = nullptr;
  }

  int64_t Size(const int64_t var, VarRefCommon &vrc) {
    return _trees[vrc._N + var]._size;
  }

  std::vector<int64_t> Clauses(const int64_t var, VarRefCommon &vrc) {
    std::vector<int64_t> ans;
    _pTraversed = &ans;
    _pVrc = &vrc;
    const int64_t iTree = vrc._N + var;
    ans.reserve(_trees[iTree]._size);
    traverse(_trees[iTree]._iRoot);
    _pTraversed = nullptr;
    _pVrc = nullptr;
    return std::move(ans);
  }

  bool Contains(const int64_t var, const int64_t iClause, VarRefCommon &vrc) {
    _pVrc = &vrc;
    const int64_t iTree = vrc._N + var;
    bool ans = (findNode(_trees[iTree]._iRoot, iClause) >= 0);
    _pVrc = nullptr;
    return ans;
  }
};
