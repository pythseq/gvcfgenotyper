//
// Created by O'Connell, Jared on 11/2/17.
//

#ifndef GVCFGENOTYPER_DEPTHBLOCK_HH
#define GVCFGENOTYPER_DEPTHBLOCK_HH

extern "C" {
#include <htslib/vcf.h>
}

#include "utils.hh"

//simple class that stores the pertinent values from a GVCF homref block
class DepthBlock
{
public:
    DepthBlock();

    DepthBlock(int rid, int start, int end, int dp, int dpf, int gq);

    DepthBlock intersect(const DepthBlock &db);

    DepthBlock intersect(int rid, int start, int end);

    int intersect_size(int rid, int a, int b) const;

    int intersect_size(const DepthBlock &db) const;

    int size() const;

    void set_missing();//set all values to bcftools missing
    void zero();//zero all values
    void add(const DepthBlock &db);

    void divide(int n);

    int _rid, _start, _end, _dp, _gq, _dpf;
};

#endif //GVCFGENOTYPER_DEPTHBLOCK_HH