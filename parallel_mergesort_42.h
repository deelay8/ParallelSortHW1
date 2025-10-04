#pragma once
#include <parlay/sequence.h>
#include <parlay/primitives.h>
#include <parlay/slice.h>   // for make_slice
#include <algorithm>        // std::copy, std::lower_bound

namespace parallel_mergesort_42 {

// --------------------------------------------------
// 병렬 merge 함수
// --------------------------------------------------
inline void parallel_merge(parlay::slice<long*, long*> A,
                           parlay::slice<long*, long*> B,
                           parlay::slice<long*, long*> C) {
  size_t n = A.size() + B.size();

  // 작은 입력은 시퀀셜 병합 (cutoff = 2000)
  if (n < 2000) {
    size_t ai=0, bi=0, ci=0;
    while (ai < A.size() && bi < B.size()) {
      if (A[ai] < B[bi]) C[ci++] = A[ai++];
      else               C[ci++] = B[bi++];
    }
    while (ai < A.size()) C[ci++] = A[ai++];
    while (bi < B.size()) C[ci++] = B[bi++];
    return;
  }

  // 항상 A가 더 크거나 같게 보장
  if (A.size() < B.size()) {
    parallel_merge(B, A, C);
    return;
  }
  if (A.size() == 0) {
    std::copy(B.begin(), B.end(), C.begin());
    return;
  }

  // divide-and-conquer
  size_t midA = A.size()/2;
  long val = A[midA];
  size_t midB = std::lower_bound(B.begin(), B.end(), val) - B.begin();
  size_t midC = midA + midB;
  C[midC] = val;

  // 병렬 실행
  parlay::par_do(
    [&]{ parallel_merge(A.cut(0, midA), B.cut(0, midB), C.cut(0, midC)); },
    [&]{ parallel_merge(A.cut(midA+1, A.size()),
                        B.cut(midB, B.size()),
                        C.cut(midC+1, n)); }
  );
}

// --------------------------------------------------
// 병렬 mergesort helper
// --------------------------------------------------
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

  parlay::sequence<long> out(n);
  parallel_merge(parlay::make_slice(L),
                 parlay::make_slice(R),
                 parlay::make_slice(out));
  return out;
}

// --------------------------------------------------
// 외부 인터페이스
// --------------------------------------------------
inline void mergesort(parlay::sequence<long>& array) {
  array = mergesort_helper(array, 0, (int)array.size());
}

} // namespace parallel_mergesort_42
