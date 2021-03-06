//
// Copyright [2020] <inhzus>
//

#ifndef YALDB_SKIP_LIST_H_
#define YALDB_SKIP_LIST_H_

#include <cassert>
#include <ctime>
#include <functional>
#include <random>
#include <memory>
#include <utility>

namespace yaldb {

template<typename T, typename Comp>
class SkipList;

namespace impl {

template<typename T>
struct SkipListNode {
  T value;
  SkipListNode *back;
  size_t level;
//  struct NodeLink {
//    SkipListNode *n;
//    NodeLink() : n(nullptr) {}
//  } links[1];
  SkipListNode **links;
/*
//  static void *operator new(const std::size_t size) {
//    return ::operator new(size);
//  }
//  static void operator delete(void *ptr) {
//
//  }
//  static void *operator new(const std::size_t size, const int level) {
//    assert(level > 0);
//    return ::operator new(size + (level - 1) * sizeof(NodeLink));
//  }
//  static void operator delete(void *ptr, const int level) {
//    return ::operator delete(ptr, sizeof(SkipListNode)
//        + (level - 1) * sizeof(NodeLink));
//  }
//  static void
//  operator delete(void *ptr, const std::size_t size, const int level) {
//    return ::operator delete(ptr, size);
//  }
 */
  SkipListNode(T value, const size_t level, SkipListNode *back) :
      value(std::move(value)), back(back), level(level) {
    links = new SkipListNode *[level];
//    std::uninitialized_fill_n(
//        &links[0], level * sizeof(SkipListNode *), nullptr);
    for (size_t i = 0; i < level; ++i) {
      links[i] = nullptr;
    }
//    printf("construct level: %lu\n", level);
  }
  ~SkipListNode() {
//    printf("destruct level: %lu\n", level);
    delete[](links);
  }
};

template<typename T>
class SkipListIterator {
 private:
  template<typename U, typename Comp> friend
  class ::yaldb::SkipList;
  friend class SkipListNode<std::remove_const_t<T>>;
  SkipListNode<T> *node_;

 public:
//  SkipListIterator(const SkipListIterator &it) : node_(it.node_) {}
//  SkipListIterator(
//      const SkipListIterator<std::remove_const_t<T>> &it) : node_(it.node_) {}
//  template<typename U, typename std::enable_if_t<
//      std::is_same_v<T, U> || std::is_same_v<std::remove_const_t<T>, U>,
//      int> = 0>
//  SkipListIterator(const SkipListIterator<U> &it) : node_(it.node_) {} // NOLINT
  SkipListIterator(const SkipListIterator &it) : node_(it.node_) {}
  explicit SkipListIterator(SkipListNode<T> *node) : node_(node) {}

//  T &operator*() { return node_->value; }
  const T &operator*() const { return node_->value; }
//  T *operator->() { return &node_->value; }
  const T *operator->() const { return &node_->value; }
  SkipListIterator &operator++() {
    node_ = node_->links[0];
    return *this;
  }
  SkipListIterator &operator--() {
    node_ = node_->back;
    return *this;
  }
  SkipListIterator operator++(int) {  // NOLINT
    SkipListIterator it(*this);
    ++(*this);
    return it;
  }
  SkipListIterator operator--(int) {  // NOLINT
    SkipListIterator it(*this);
    --(*this);
    return it;
  }
  bool operator==(const SkipListIterator &it) const {
    return node_ == it.node_;
  }
  bool operator!=(const SkipListIterator &it) const {
    return node_ != it.node_;
  }
//  template<typename U>
//  friend bool operator==(SkipListIterator<T> lhs, SkipListIterator<U> rhs) {
//    return lhs.node_ == rhs.node_;
//  }
};

}  // namespace impl

// template<typename T, bool(*Less)(T, T)>
template<typename T, typename Comp = std::less<T>>
class SkipList {
 public:
  using node_type = impl::SkipListNode<T>;
  using iterator = impl::SkipListIterator<T>;
  using const_iterator = iterator;

 private:
  [[nodiscard]] size_t RandomLevel() const;
  [[nodiscard]] node_type *FindPrev(const T &value) const;
  node_type *FindPrev(const T &value, node_type **prev) const;

  static constexpr double kRandomRatio = 0.5;
  static constexpr size_t kMaxLevel = 32;

  Comp comp_;
  mutable std::mt19937 rand_gen_;
  size_t length_;
  node_type *head_;
  node_type *tail_;

 public:
  explicit SkipList(Comp comp = std::less<T>());  // NOLINT
  ~SkipList();

  size_t Size() const { return length_; }
  bool Empty() const { return length_ == 0; }

  iterator begin() { return iterator(head_->links[0]); }
  iterator end() { return iterator(tail_); }
  [[nodiscard]] const_iterator begin() const {
    return const_iterator(head_->links[0]);
  }
  [[nodiscard]] const_iterator end() const {
    return const_iterator(tail_);
  }

  iterator Insert(T value);
  iterator Erase(const T &value);
  iterator Erase(iterator it);
  iterator Find(const T &value) const;
  std::pair<iterator, iterator> EqualRange(const T &value) const;
};

template<typename T, typename Comp>
size_t SkipList<T, Comp>::RandomLevel() const {
  size_t level = 1;
  std::uniform_real_distribution<double> dis(0, 1);
  while (dis(rand_gen_) < kRandomRatio && level < kMaxLevel) {
    ++level;
  }
  return level;
}
template<typename T, typename Comp>
typename SkipList<T, Comp>::node_type *
SkipList<T, Comp>::FindPrev(const T &value) const {
  node_type *cur = head_;
  for (size_t i = kMaxLevel - 1; i != size_t() - 1; --i) {
    while (cur->links[i] != tail_ && comp_(cur->links[i]->value, value)) {
      cur = cur->links[i];
    }
  }
  return cur;
}
template<typename T, typename Comp>
typename SkipList<T, Comp>::node_type *
SkipList<T, Comp>::FindPrev(const T &value, node_type **prev) const {
  node_type *cur = head_;
  for (size_t i = kMaxLevel - 1; i != size_t() - 1; --i) {
    while (cur->links[i] != tail_ && comp_(cur->links[i]->value, value)) {
      cur = cur->links[i];
    }
    prev[i] = cur;
  }
  return cur;
}

template<typename T, typename Comp>
SkipList<T, Comp>::SkipList(Comp comp) :
    comp_(std::move(comp)), length_(0) {
  static_assert(std::is_invocable_v<Comp, const T &, const T &>);
  static_assert(std::is_same_v<
      bool, std::invoke_result_t<Comp, const T &, const T &>>);
  std::random_device rd;
  rand_gen_ = std::mt19937(rd());
  head_ = new node_type(T(), kMaxLevel, nullptr);
  tail_ = new node_type(T(), kMaxLevel, head_);
  for (size_t i = 0; i < kMaxLevel; ++i) {
    head_->links[i] = tail_;
  }
}
template<typename T, typename Comp>
SkipList<T, Comp>::~SkipList() {
  while (tail_ != nullptr) {
    node_type *node = tail_->back;
    delete (tail_);
    tail_ = node;
  }
//  ::operator delete(head_, head_->level);
//  ::operator delete(tail_, tail_->level);
}
template<typename T, typename Comp>
typename SkipList<T, Comp>::iterator
SkipList<T, Comp>::Insert(T value) {
  auto prev = std::make_unique<node_type *[]>(kMaxLevel);;
  node_type *cur = FindPrev(value, prev.get());
//  if (next != tail_ &&
//      !comp_(next->value, value) &&
//      !comp_(value, next->value)) {
//    return end();
//  }
  size_t level = RandomLevel();
  auto *node = new node_type(std::move(value), level, cur);
  for (size_t i = 0; i < level; ++i) {
    node->links[i] = prev[i]->links[i];
    prev[i]->links[i] = node;
  }
  node->links[0]->back = node;
  ++length_;
  return iterator(node);
}
template<typename T, typename Comp>
typename SkipList<T, Comp>::iterator
SkipList<T, Comp>::Erase(const T &value) {
  auto prev = std::make_unique<node_type *[]>(kMaxLevel);
  FindPrev(value, prev.get());
  node_type *first = tail_, *last = first;
  for (size_t i = kMaxLevel - 1; i != size_t() - 1; --i) {
    for (node_type *node = prev[i]->links[i];
         node != tail_ &&
             !comp_(node->value, value) &&
             !comp_(value, node->value);
         node = prev[i]->links[i]) {
      prev[i]->links[i] = node->links[i];
      if (i == 0) {
        if (first == tail_) {
          first = node;
        }
        last = node->links[i];
      }
    }
  }
  if (first != last) {
    last->back = first->back;
    while (first != last) {
      node_type *tmp = first;
      first = first->links[0];
//      ::operator delete(tmp, tmp->level);
      delete tmp;
      --length_;
    }
    return iterator(prev[0]);
  } else {
    return end();
  }
}
template<typename T, typename Comp>
typename SkipList<T, Comp>::iterator
SkipList<T, Comp>::Erase(iterator it) {
  auto prev = std::make_unique<node_type *[]>(kMaxLevel);
  FindPrev(*it, prev.get());
  bool is_contained = false;
  for (size_t i = kMaxLevel - 1; i != size_t() - 1; --i) {
    for (node_type *cur = prev[i], *next = cur->links[i];
         next != tail_ &&
             !comp_(next->value, *it) &&
             !comp_(*it, next->value);
         cur = next, next = cur->links[i]) {
      if (next == it.node_) {
        is_contained = true;
        cur->links[i] = next->links[i];
        break;
      }
    }
  }
  if (is_contained) {
    node_type *back = it.node_->back;
    it.node_->links[0]->back = back;
    delete it.node_;
//    ::operator delete(it.node_, it.node_->level);
    --length_;
    return iterator(back);
  } else {
    return end();
  }
}
template<typename T, typename Comp>
typename SkipList<T, Comp>::iterator
SkipList<T, Comp>::Find(const T &value) const {
  node_type *cur = FindPrev(value), *next = cur->links[0];
  if (next != tail_ &&
      !comp_(next->value, value) &&
      !comp_(value, next->value)) {
    return iterator(next);
  } else {
    return end();
  }
}
template<typename T, typename Comp>
std::pair<typename SkipList<T, Comp>::iterator,
          typename SkipList<T, Comp>::iterator>
SkipList<T, Comp>::EqualRange(const T &value) const {
  node_type *cur = FindPrev(value), *first = cur->links[0], *last = first;
  while (last != tail_ &&
      !comp_(last->value, value) &&
      !comp_(value, last->value)) {
    last = last->links[0];
  }
  return std::make_pair(iterator(first), iterator(last));
}

}  // namespace yaldb

namespace std {

template<typename T>
struct iterator_traits<yaldb::impl::SkipListIterator<T>> {
  typedef bidirectional_iterator_tag iterator_category;
  typedef T value_type;
  typedef ptrdiff_t difference_type;
  typedef T *pointer;
  typedef T &reference;
};

}  // namespace std

#endif  // YALDB_SKIP_LIST_H_
