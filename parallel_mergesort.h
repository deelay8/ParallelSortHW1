#pragma once
#include <parlay/sequence.h>
#include <parlay/primitives.h>

// 굳이 sequential_mergesort.h를 또 include 할 필요 없음
// mergesort.cpp에서 이미 불러와 줌

namespace parallel_mergesort {

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
