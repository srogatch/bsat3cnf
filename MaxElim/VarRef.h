#pragma once

#include "AVLTree.h"

struct Problem;

struct VarRefCommon {
  AVLNodePool _avlNp;
  int64_t _N = -1;

  void Init(const int64_t N) {
    _N = N;
  }
};

template<int8_t taClauseSz> struct VarRef {
  FastVector<AVLTree> _trees;

private:
  Problem *_pProb = nullptr;
  int64_t _var;
  FastVector<int64_t> *_pTraversed;

private:
  AVLNode & getNode(const int64_t iNode);
  const AVLNode & getNode(const int64_t iNode) const;

  //// Taken from: https://www.geeksforgeeks.org/avl-tree-set-1-insertion/
  // A utility function to get the height of the tree 
  int64_t height(const int64_t iNode);

  /* Helper function that allocates a new node with the given key and
  NULL left and right pointers. */
  int64_t newNode(const int64_t key);

  // A utility function to right rotate subtree rooted with y 
  // See the diagram given above. 
  int64_t rightRotate(const int64_t y);

  // A utility function to left rotate subtree rooted with x 
  // See the diagram given above. 
  int64_t leftRotate(const int64_t x);

  // Get Balance factor of node N 
  int64_t getBalance(const int64_t iNode);

  // Recursive function to insert a key in the subtree rooted 
  // with node and returns the new root of the subtree. 
  int64_t insert(const int64_t iNode, const int64_t key);

  void traverse(const int64_t iParent);

  int64_t findNode(const int64_t iParent, const int64_t key);

  /* Given a non-empty binary search tree, return the
  node with minimum key value found in that tree.
  Note that the entire tree does not need to be
  searched. */
  int64_t minValueNode(const int64_t iNode);

  //// Taken from https://www.geeksforgeeks.org/avl-tree-set-2-deletion/
  // Recursive function to delete a node with given key 
  // from subtree with given root. It returns root of 
  // the modified subtree. 
  int64_t deleteNode(int64_t root, const int64_t key);

public:
  void Init(const Problem &prob);

  void Add(const int64_t var, const int64_t iClause, Problem &prob);

  void Del(const int64_t var, const int64_t iClause, Problem &prob);

  int64_t Size(const int64_t var, const Problem &prob) const;

  FastVector<int64_t> Clauses(const int64_t var, Problem &prob);

  bool Contains(const int64_t var, const int64_t iClause, Problem &prob);
};
