#ifndef SW_HPP
#define SW_HPP

#include <vector>
#include <utility>
#include <functional>
#include <iostream>
#include <iomanip>
#include <type_traits>
#include "matrix.hpp"
#include "matcher.hpp"

#define BLOCK_SIZE 64

template<typename It>
std::vector<ItAlignment<It>> findAlignments(It known, It unknown, Eigen::MatrixXi& matrix, Eigen::MatrixXi& sources, unsigned int threshold) {
    std::vector<ItAlignment<It>> alignments;

    while(1){
        auto [i, j] = slowMatrixMax(matrix);
        if(i <= 0 || j <= 0){
            return alignments;
        }
        if(matrix(i, j) < threshold){
            return alignments;
        }

        ItAlignment<It> a;

        a.endUnknown = unknown + i;
        a.endKnown = known + j;
        a.score = matrix(i, j);

        do{
            if(matrix(i, j) == 0){
                a.startUnknown = unknown + i;
                a.startKnown = known + j;
                alignments.push_back(a);
                break;
            }

            matrix(i, j) = -1;
            int tempi = i - (bool)sources(i, j);
            j -= sources(i, j) < 2;
            i = tempi;

        } while(matrix(i, j) >= 0);
    }

    return alignments;
}

template<typename It, typename Cmp>
void populateSearchSpace(It known, It unknown, int m, int n, unsigned int gapScore, Cmp comp, Eigen::MatrixXi& matrix, Eigen::MatrixXi& sources) {
    // populate matrix
    for(int i = 1; i <= m; i++){
        for(int j = 1; j <= n; j++){
            // if((i * n + j) % 20 == 0) std::cout << "Inner loop: " << i * n + j << " out of " << m * n << std::endl;
            int max = 0;
            int score = 0;

            // first comparison: west cell (deletion)
            score = matrix(i, j-1) - gapScore;
            if(score > max){
                max = score;
                sources(i, j) = 0;
            }

            // second comparison: north cell (insertion)
            score = matrix(i-1, j) - gapScore;
            if(score > max){
                max = score;
                sources(i, j) = 2;
            }

            // last comparison: north-west cell (alignment)
            if(max < (score = comp(known[j - 1], unknown[i - 1]) + matrix(i - 1, j - 1))){
                max = score;
                sources(i, j) = 1;
            }

            matrix(i, j) = max;
        }
    }
}

template <typename It, typename Cmp>
std::vector<ItAlignment<It>> calculateAlignment(It known, It knownEnd, It unknown, It unknownEnd, Cmp comp, unsigned int threshold, unsigned int gapScore){
    int m = std::distance(unknown, unknownEnd);
    int n = std::distance(known, knownEnd);

    Eigen::MatrixXi matrix(m + 1, n + 1);
    matrix.fill(0);
    // 0 for left, 1 for diagonal, 2 for up
    Eigen::MatrixXi sources(m + 1, n + 1);
    sources.fill(-1);

    populateSearchSpace(known, unknown, m, n, gapScore, comp, matrix, sources);
    //FIXME
#ifdef SW_COUT
    for(auto& a : matrix){
        for(auto& b : a){
            std::cout << std::setw(2) << b << " ";
        }
        std::cout << std::endl;
    }
#endif

    return findAlignments(known, unknown, matrix, sources, threshold);
}

template <typename Range, typename Cmp>
std::vector<Alignment> calculateAlignment(Range&& known, Range&& unknown, Cmp&& comp, int threshold, unsigned int gapScore){
    auto alignments = calculateAlignment(known.begin(), known.end(), unknown.begin(), unknown.end(), comp, threshold, gapScore);
    std::vector<Alignment> ret;
    ret.reserve(alignments.size());

    std::transform(alignments.begin(), alignments.end(), std::back_inserter(ret), [&known, &unknown](auto val) {
        return Alignment{
            static_cast<unsigned int>(std::distance(known.begin(), val.startKnown)),
            static_cast<unsigned int>(std::distance(unknown.begin(), val.startUnknown)),
            static_cast<unsigned int>(std::distance(known.begin(), val.endKnown)),
            static_cast<unsigned int>(std::distance(unknown.begin(), val.endUnknown)),
            val.score
        };
    });

    return ret;
}

#endif
