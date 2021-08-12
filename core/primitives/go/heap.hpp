/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::primitives::go {

  /**
   * Go Heap interfase to be controlled by HeapController,
   * where T is an heap element type.
   *
   * If you need to use Go heap, then inherit your heap from IHeap and pass it
   * to HeapController
   *
   * The Interface type describes the requirements for a type using the routines
   * in this package. Any type that implements it may be used as a min-heap with
   * the following invariants
   * (established after Init() has been called or if the data is empty or
   * sorted):
   *
   * !h.Less(j, i) for 0 <= i < h.Len() and 2*i+1 <= j <= 2*i+2 and j < h.Len()
   *
   * NOTE: that push() and pop() in this interface are for package heap's
   * implementation to call. To add and remove things from the heap, use
   * HeapController::push() and HeapController::pop().
   */
  template <typename T>
  class IHeap {
   public:
    virtual ~IHeap() = default;

    virtual int length() const = 0;
    virtual bool less(int i, int j) const = 0;
    virtual void swap(int i, int j) = 0;
    virtual void push(const T &element) = 0;
    virtual T pop() = 0;
  };

  /**
   * HeapController provides Go heap package functionality, see here:
   * https://pkg.go.dev/container/heap
   * https://cs.opensource.google/go/go/+/refs/tags/go1.15.3:src/container/heap/heap.go
   *
   * Use HeapController to manipulate your heap as a Go heap
   */
  template <typename T>
  class HeapController {
   public:
    explicit HeapController(IHeap<T> &controlled_heap)
        : heap{controlled_heap} {}

    ~HeapController() = default;
    HeapController(const HeapController &) = delete;
    HeapController(HeapController &&) = delete;
    HeapController &operator=(const HeapController &) = delete;
    HeapController &operator=(HeapController &&) = delete;

    /**
     * Init establishes the heap invariants required by the other routines in
     * this package. Init is idempotent with respect to the heap invariants and
     * may be called whenever the heap invariants may have been invalidated.
     * The complexity is O(n) where n = heap.length().
     */
    void init() {
      // heapify
      const auto n = heap.length();
      for (int i = n / 2 - 1; i >= 0; i--) {
        down(i, n);
      }
    }

    /**
     * Push pushes the element x onto the heap.
     * The complexity is O(log n) where n = heap.length().
     */
    void push(const T &element) {
      heap.push(element);
      up(heap.length() - 1);
    }

    /**
     * Pop removes and returns the minimum element (according to Less) from the
     * heap.
     * The complexity is O(log n) where n = heap.length().
     * Pop is equivalent to Remove(0).
     */
    T pop() {
      const auto n = heap.length() - 1;
      heap.swap(0, n);
      down(0, n);
      return heap.pop();
    }

    /**
     * Remove removes and returns the element at index i from the heap.
     * The complexity is O(log n) where n = heap.length().
     */
    T remove(int i) {
      const auto n = heap.length() - 1;
      if (n != i) {
        heap.swap(i, n);
        if (!down(i, n)) {
          up(i);
        }
      }
      return heap.pop();
    }

    /**
     * Fix re-establishes the heap ordering after the element at index i has
     * changed its value. Changing the value of the element at index i and then
     * calling Fix is equivalent to, but less expensive than, calling
     * Remove(h, i) followed by a Push of the new value.
     * The complexity is O(log n) where n = heap.length().
     */
    void fix(int i) {
      if (!down(i, heap.length())) {
        up(i);
      }
    }

   private:
    void up(int j) {
      while (true) {
        const int i = (j - 1) / 2;  // parent

        if (i == j || !heap.less(j, i)) {
          break;
        }

        heap.swap(i, j);
        j = i;
      }
    }

    bool down(int i0, int n) {
      int i = i0;
      while (true) {
        const int j1 = 2 * i + 1;

        if (j1 >= n || j1 < 0) {  // j1 < 0 after int overflow
          break;
        }

        int j = j1;  // left child
        if (const int j2 = j1 + 1; j2 < n && heap.less(j2, j1)) {
          j = j2;  // = 2 * i + 2  // right child
        }

        if (!heap.less(j, i)) {
          break;
        }

        heap.swap(i, j);
        i = j;
      }
      return i > i0;
    }

    IHeap<T> &heap;
  };

}  // namespace fc::primitives::go
