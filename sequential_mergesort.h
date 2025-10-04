
#include <parlay/sequence.h>

namespace sequential_mergesort {

    parlay::sequence<long> merge(parlay::sequence<long> a, parlay::sequence<long> b) {
        parlay::sequence<long> c(a.size() + b.size());
        int ai = 0, ci = 0, bi = 0;
        while(ai < a.size() && bi < b.size()){
            if(a[ai] < b[bi]) {
                c[ci] = a[ai]; ai++; ci++;
            } else {
                c[ci] = b[bi]; bi++; ci++;
            }
        }
        while(ai < a.size()) {
            c[ci] = a[ai]; ai++; ci++;
        }
        while(bi < b.size()) {
            c[ci] = b[bi]; bi++; ci++;
        }
        return c;
    }

    parlay::sequence<long> mergesort_helper(parlay::sequence<long> &array, int start, int end){
        int n = end-start;
        int m = start + n/2;
        if(n <= 1) return array.subseq(start, end);
        else {
            return merge(mergesort_helper(array, start, m), mergesort_helper(array, m, end));
        }
    }

    void mergesort(parlay::sequence<long> &array) {
        array = mergesort_helper(array, 0, array.size());
        // std::cout << "first 10 elements: " << parlay::to_chars(array.head(10)) << std::endl;

    }
}

