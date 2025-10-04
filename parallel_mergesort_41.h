#pragma once
#include <parlay/sequence.h>
#include <parlay/primitives.h>

namespace parallel_mergesort_41 {

  inline parlay::sequence<long>
  mergesort_helper(parlay::sequence<long>& a, int s, int e) {
    int n = e - s;
    if (n <= 1) return a.subseq(s, e);

    int m = s + n/2;
    parlay::sequence<long> L, R;

    parlay::par_do(
      [&]{ L = mergesort_helper(a, s, m); },
      [&]{ R = mergesort_helper(a, m, e); }
    );

    // 여기서 sequential_mergesort::merge 사용
    return sequential_mergesort::merge(L, R);
  }

  inline void mergesort(parlay::sequence<long>& array) {
    array = mergesort_helper(array, 0, (int)array.size());
  }

}
