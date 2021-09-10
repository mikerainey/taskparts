#include <vector>
#include <stdio.h>

#include "sum_tree_rollforward_decls.hpp"

int answer;

class node {
public:
  int value;
  node* left;
  node* right;

  node(int value)
    : value(value), left(nullptr), right(nullptr) { }
  node(int value, node* left, node* right)
    : value(value), left(left), right(right) { }
};

void sum_serial(node* n);

int main() {
  auto ns = std::vector<node*>(
    { 
      nullptr,
      new node(123),
      new node(1, new node(2), nullptr),
      new node(1, nullptr, new node(2)),  
      new node(1, new node(200), new node(3)), 
      new node(1, new node(2), new node(3, new node(4), new node(5))),
      new node(1, new node(3, new node(4), new node(5)), new node(2)),
    });
  for (auto n : ns) {
    answer = -1;
    sum_serial(n);
    printf("answer=%d\n",answer);
  }
  return 0;
}
