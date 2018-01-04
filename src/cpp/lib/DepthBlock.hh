//
// Created by O'Connell, Jared on 11/2/17.
//

#ifndef GVCFGENOTYPER_DEPTHBLOCK_HH
#define GVCFGENOTYPER_DEPTHBLOCK_HH

extern "C" {
#include <htslib/vcf.h>
}

#include "ggutils.hh"

//simple class that stores the pertinent values from a GVCF homref block
class DepthBlock
{
public:
    DepthBlock();

    DepthBlock(int rid, int start, int end, int dp, int dpf, int gq,int ploidy);

    inline bool operator == (const DepthBlock& db) const {
        return (_rid==db._rid &&
                _start==db._start &&
                _end==db._end &&
                _dp==db._dp &&
                _dpf==db._dpf &&
                _gq==db._gq
               );
    }

    DepthBlock intersect(const DepthBlock &db);

    DepthBlock intersect(int rid, int start, int end);

    int intersect_size(int rid, int a, int b) const;

    int intersect_size(const DepthBlock &db) const;
    int get_ploidy();
    int size() const;

    void set_missing();//set all values to bcftools missing
    void zero();//zero all values
    void add(const DepthBlock &db);
    int rid() const { return _rid; }
    int start() const { return _start; }
    int end() const { return _end; }
    int dp() const { return _dp; }
    int gq() const { return _gq; }
    int dpf() const { return _dpf; }

    void divide(int n);

private:
    int _rid, _start, _end, _dp, _dpf, _gq, _ploidy;
};

#endif //GVCFGENOTYPER_DEPTHBLOCK_HH

