#include <iostream>

#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/random.h>
#include <parlay/sequence.h>
#include <algorithm>    // std::sort

#include <sequential_mergesort.h> 
#include <parallel_mergesort.h>    // (기존 par 버전, 필요 시 유지)
#include <parallel_mergesort_41.h> // 4-1
#include <parallel_mergesort_42.h> // 4-2
#include <parallel_mergesort_43.h> // 4-3

// **************************************************************
// Driver
// **************************************************************
int main(int argc, char* argv[]) {
  auto usage = "Usage: mergesort <n> <algname>";
  if (argc != 3) std::cout << usage << std::endl;
  else {
    long n;
    try { n = std::stol(argv[1]); }
    catch (...) { std::cout << usage << std::endl; return 1; }
    parlay::random_generator gen;
    std::uniform_int_distribution<long> dis(0, n-1);

    // generate random long values
    auto data = parlay::tabulate(n, [&] (long i) {
      auto r = gen[i];
      return dis(r);});

    std::cout << "first 10 elements: " << parlay::to_chars(data.head(10)) << std::endl;

    parlay::internal::timer t("Time");
    parlay::sequence<long> result;
    for (int i=0; i < 5; i++) {
      result = data;
      t.start();
      if(((std::string)argv[2]) == "seq")
        sequential_mergesort::mergesort(result);
      else if(((std::string)argv[2]) == "par")
        parallel_mergesort::mergesort(result);   // 기존 par (있으면 유지)
      else if(((std::string)argv[2]) == "par41")
        parallel_mergesort_41::mergesort(result);
      else if(((std::string)argv[2]) == "par42")
        parallel_mergesort_42::mergesort(result);
      else if(((std::string)argv[2]) == "par43")
        parallel_mergesort_43::mergesort(result);
      else if(((std::string)argv[2]) == "stdsort") {
        std::sort(result.begin(), result.end());
      }
      else {
        std::cout << "invalid algname: " << ((std::string)argv[2]) << std::endl;
        exit(1);
      }
      t.next("mergesort");
    }

    auto first_ten = result.head(10);

    std::cout << "first 10 elements: " << parlay::to_chars(first_ten) << std::endl;
  }
}
