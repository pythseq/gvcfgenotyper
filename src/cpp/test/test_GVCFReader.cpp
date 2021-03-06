#include "test_helpers.hh"

extern "C"
{
#include <htslib/vcf.h>
}

#include "GVCFReader.hh"


TEST(DepthBlock, intersects)
{
    DepthBlock db1(0, 100, 199, 20, 1, 30,2);
    DepthBlock db2(0, 100, 100, 20, 1, 30,2);
    ASSERT_EQ(db1.IntersectSize(db2), 1);
    ASSERT_EQ(db1.IntersectSize(db2), db2.IntersectSize(db1));

    DepthBlock db3(0, 300, 401, 20, 1, 30,2);
    ASSERT_EQ(db1.IntersectSize(db3), 0);

    DepthBlock db4(1, 300, 401, 20, 1, 30,2);
    ASSERT_EQ(db1.IntersectSize(db4), 0);

    DepthBlock db5(0, 190, 401, 20, 1, 30,2);
    ASSERT_EQ(db1.IntersectSize(db5), 10);

    DepthBlock db6(0, 100, 100, 20, 1, 30,2);
    DepthBlock db7(0, 100, 100, 20, 1, 30,2);
    ASSERT_EQ(db6.IntersectSize(db7), 1);

    DepthBlock db8(1, 10000, 10000, 20, 1, 30,2);
    DepthBlock db9(1, 10000, 10000, 20, 1, 30,2);
    ASSERT_EQ(db8.IntersectSize(db9), 1);

}

TEST(DepthBuffer, interpolate)
{
    DepthBuffer buf;
    buf.push_back(DepthBlock(0, 0, 99, 20, 1, 30,2));
    buf.push_back(DepthBlock(0, 100, 109, 30, 1, 30,2));
    buf.push_back(DepthBlock(0, 110, 200, 40, 1, 30,2));
    DepthBlock db;
    buf.Interpolate(0, 90, 95, db);
    ASSERT_EQ(db.dp(), 20);
    buf.Interpolate(0, 99, 99, db);
    ASSERT_EQ(db.dp(), 20);
    buf.Interpolate(0, 100, 100, db);
    ASSERT_EQ(db.dp(), 30);
    buf.Interpolate(0, 95, 104, db);
    ASSERT_EQ(db.dp(), 25);
    buf.Interpolate(0, 95, 114, db);
    ASSERT_EQ(db.dp(), 30);
    buf.Interpolate(0, 95, 154, db);
    ASSERT_EQ(db.dp(), 37);
}

TEST(VariantBuffer, test1)
{
    int rid=1;
    int pos=99;
    auto hdr = get_header();
    auto rec1 = generate_record(hdr,rid,pos,"C,G");

    VariantBuffer v;
    v.PushBack(hdr, rec1);
    v.FlushBuffer(rec1);
    ASSERT_TRUE(v.IsEmpty());

    auto rec2 = generate_record(hdr,rid,pos,"C,G");
    auto rec3 = generate_record(hdr,rid,pos,"C,CG");
    v.PushBack(hdr, bcf_dup(rec2));
    v.PushBack(hdr, bcf_dup(rec3));

    v.FlushBuffer(rec2);
    ASSERT_FALSE(v.IsEmpty());

    v.FlushBuffer(rec3);
    ASSERT_TRUE(v.IsEmpty());
}

//This tests a fairly tricky case of two different insertions starting at the same position within the same sample.
TEST(VariantBuffer, test2)
{
    multiAllele m;
    auto hdr = get_header();
    m.Init(hdr);
    std::string ref_file_name = g_testenv->getBasePath() + "/../test/test2/test2.ref.fa";
    Normaliser norm(ref_file_name);
    auto record1 = generate_record(hdr,"chr1\t7832\trs112070696\tC\tCTAAATAAATAAA,CTAAATAAATAAATAAA\t559\tPASS\t.\t"
            "GT:GQ:GQX:DPI:AD:ADF:ADR:FT:PL\t1/2:150:15:42:0,11,11:0,4,5:0,7,6:PASS:601,226,169,225,0,169");
    m.SetPosition(record1->rid, record1->pos);
    vector<bcf1_t *> buffer;
    norm.Unarise(record1, buffer,hdr);
    VariantBuffer v;
    int count=0;
    for (auto it = buffer.begin(); it != buffer.end(); it++)
    {
        v.PushBack(hdr, *it);
        ASSERT_EQ(m.Allele(*it),++count);
    }
    v.FlushBuffer(m.GetMax());
    ASSERT_TRUE(v.IsEmpty());
}

TEST(GVCFReader, readMNP)
{
    std::string gvcf_file_name = g_testenv->getBasePath() + "/../test/mnp.genome.vcf";
    std::string ref_file_name = g_testenv->getBasePath() + "/../test/tiny.ref.fa";
    Normaliser normaliser(ref_file_name);

    GVCFReader reader(gvcf_file_name, &normaliser, 1000);
    const bcf_hdr_t *hdr = reader.GetHeader();
    bcf1_t *line = reader.Pop();
    int32_t *dp = nullptr, nval = 0;
    while (line != nullptr)
    {
        if (ggutils::is_snp(line))
        {
            ASSERT_EQ(bcf_get_format_int32(hdr, line, "DP", &dp, &nval),-3);
        }
        bcf_destroy(line);
        line = reader.Pop();
    }
    free(dp);
}

// //GVCFReader should match this VID output
// //bcftools norm -m -any data/NA12877.tiny.vcf.gz |
// bcftools norm -f data/tiny.ref.fa |
// bcftools query -i 'ALT!="."' -f '%CHROM:%POS:%REF:%ALT\n' > data/NA12877.tiny.vcf.gz.expected
TEST(GVCFReader, readAGVCF)
{
    std::string expected_output_file = g_testenv->getBasePath() + "/../test/NA12877.tiny.vcf.gz.expected";
    std::string gvcf_file_name = g_testenv->getBasePath() + "/../test/NA12877.tiny.vcf.gz";
    std::string ref_file_name = g_testenv->getBasePath() + "/../test/tiny.ref.fa";
    char tn[] = "/tmp/tmpvcf-XXXXXXX";
    int fd = mkstemp(tn);
    if (fd < 1)
    {
        error("Cannot create temp file: %s", strerror(errno));
    }
    close(fd);

    std::ofstream ofs(tn, std::ofstream::out);
    int buffer_size = 200;
    Normaliser normaliser(ref_file_name);

    GVCFReader reader(gvcf_file_name, &normaliser, buffer_size);
    const bcf_hdr_t *hdr = reader.GetHeader();
    bcf1_t *line = reader.Pop();
    int32_t *dp = nullptr, nval = 0;
    DepthBlock db;
    while (line != nullptr)
    {
//        print_variant(hdr,line);
        ofs << bcf_hdr_id2name(hdr, line->rid) << ":" << line->pos + 1 << ":" << line->d.allele[0] << ":" << line->d.allele[1] << std::endl;
        if (ggutils::is_snp(line))
        {
            reader.GetDepth(line->rid, line->pos, line->pos, db);
            if (bcf_get_format_int32(hdr, line, "DP", &dp, &nval) == 1)
            {
                ASSERT_EQ(db.dp(), *dp);
            }
        }
        if(line->n_allele>1)
        {
            float gq;
            int ngq=0;
            if(bcf_get_format_int32(reader.GetHeader(),line,"GQ",&gq,&ngq)==1)
            {
                Genotype g(reader.GetHeader(),line);
                ASSERT_EQ(g.gq(),(int)gq);
            }
        }
        bcf_destroy(line);
        line = reader.Pop();
    }
    free(dp);
    ofs.close();
    ASSERT_TRUE(reader.IsEmpty());
    const std::string diffcmd = std::string("diff -I '^#' ") + tn + " " + expected_output_file;
    int r = system(diffcmd.c_str());
    if (r != 0)
    {
        error("Difference detected in test case %s: %s\n\n", gvcf_file_name.c_str(), diffcmd.c_str());
    }
    else
    {
        remove(tn);
    }
}


