#pragma once
#include <parlay/sequence.h>
#include <parlay/primitives.h>
#include <parlay/slice.h>   // make_slice, slice
#include <algorithm>        // std::copy, std::lower_bound, std::sort
#include <utility>          // std::pair
#include <cstddef>

namespace parallel_mergesort_43 {

// ---- 튜닝 파라미터 ----
static constexpr size_t SORT_CUTOFF  = 4000;  // 작은 정렬은 순차(std::sort)
static constexpr size_t MERGE_CUTOFF = 4000;  // 작은 머지는 순차
// 두 하위 문제가 모두 이 임계 이상일 때만 par_do 사용 (그라뉼러리티 제어)
static inline bool should_spawn(size_t left_size, size_t right_size) {
  return (left_size  >= MERGE_CUTOFF) && (right_size >= MERGE_CUTOFF);
}

// ---- 균형 분할: 결과의 k 위치를 기준으로 (i,j) 찾기 ----
// A[0..i)와 B[0..j)를 C[0..k)로 채우면 항상 거의 반반으로 분할됨.
static inline std::pair<size_t,size_t>
split_by_rank(parlay::slice<long*, long*> A,
              parlay::slice<long*, long*> B,
              size_t k) {
  const size_t aN = A.size(), bN = B.size();
  // i in [max(0, k-bN), min(k, aN)]
  size_t lo = (k > bN ? k - bN : 0);
  size_t hi = std::min(k, aN);

  while (lo < hi) {
    size_t i = (lo + hi) >> 1;
    size_t j = k - i;
    // 경계 체크와 비교 (A[i]와 B[j-1], A[i-1]와 B[j])
    bool too_small = (i < aN && j > 0 && B[j-1] > A[i]);              // i ↑
    bool too_big   = (i > 0  && j < bN && A[i-1] > B[j]);              // i ↓
    if (too_small)      lo = i + 1;
    else if (too_big)   hi = i - 1;
    else { lo = hi = i; }
  }
  size_t i = lo;
  size_t j = k - i;
  return {i, j};
}

// ---- 순차 머지 (작은 입력용) ----
static inline void sequential_merge(parlay::slice<long*, long*> A,
                                    parlay::slice<long*, long*> B,
                                    parlay::slice<long*, long*> C) {
  size_t ai=0, bi=0, ci=0;
  while (ai < A.size() && bi < B.size()) {
    C[ci++] = (A[ai] < B[bi]) ? A[ai++] : B[bi++];
  }
  while (ai < A.size()) C[ci++] = A[ai++];
  while (bi < B.size()) C[ci++] = B[bi++];
}

// ---- 병렬 머지 (Balanced split + Granularity control) ----
static inline void parallel_merge(parlay::slice<long*, long*> A,
                                  parlay::slice<long*, long*> B,
                                  parlay::slice<long*, long*> C) {
  const size_t n = A.size() + B.size();

  if (n < MERGE_CUTOFF) {  // 작은 입력은 순차
    sequential_merge(A, B, C);
    return;
  }

  // 항상 A가 더 크거나 같게 유지 (분기 수/깊이 안정화)
  if (A.size() < B.size()) {
    parallel_merge(B, A, C);
    return;
  }
  if (A.size() == 0) {
    std::copy(B.begin(), B.end(), C.begin());
    return;
  }

  // 결과 중앙에서 균형 분할
  const size_t k = n >> 1;
  auto [i, j] = split_by_rank(A, B, k);

  auto A_L = A.cut(0, i),         B_L = B.cut(0, j),         C_L = C.cut(0, k);
  auto A_R = A.cut(i, A.size()),  B_R = B.cut(j, B.size()),  C_R = C.cut(k, n);

  if (should_spawn(C_L.size(), C_R.size())) {
    parlay::par_do(
      [&]{ parallel_merge(A_L, B_L, C_L); },
      [&]{ parallel_merge(A_R, B_R, C_R); }
    );
  } else {
    parallel_merge(A_L, B_L, C_L);
    parallel_merge(A_R, B_R, C_R);
  }
}

// ---- 병렬 mergesort (작은 입력은 std::sort, 큰 입력은 par_do + 병렬 머지) ----
static inline parlay::sequence<long>
mergesort_helper(parlay::sequence<long>& a, int s, int e) {
  int n = e - s;
  if (n <= 1) return a.subseq(s, e);

  if ((size_t)n < SORT_CUTOFF) {
    parlay::sequence<long> tmp = a.subseq(s, e);
    std::sort(tmp.begin(), tmp.end());
    return tmp;
  }

  int m = s + (n >> 1);
  parlay::sequence<long> L, R;

  // 작은 한쪽은 현재 스레드에서 처리, 큰 쪽만 스폰하는 비대칭 전략(오버헤드↓)
  if ((size_t)(m - s) >= SORT_CUTOFF && (size_t)(e - m) >= SORT_CUTOFF) {
    parlay::par_do(
      [&]{ L = mergesort_helper(a, s, m); },
      [&]{ R = mergesort_helper(a, m, e); }
    );
  } else {
    L = mergesort_helper(a, s, m);
    R = mergesort_helper(a, m, e);
  }

  parlay::sequence<long> out(n);
  parallel_merge(parlay::make_slice(L),
                 parlay::make_slice(R),
                 parlay::make_slice(out));
  return out;
}

// ---- 외부 인터페이스 ----
static inline void mergesort(parlay::sequence<long>& array) {
  array = mergesort_helper(array, 0, (int)array.size());
}

} // namespace parallel_mergesort_43
