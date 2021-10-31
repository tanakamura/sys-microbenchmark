#include <fstream>
#include "sys-microbenchmark.h"

using namespace smbm;

int main(int argc, char **argv) {
    if (argc < 3) {
        puts("usage: merge-result output src0 src1.. srcN");
        return 1;
    }

    ResultListSet rls;

    for (int ai=2; ai<argc; ai++) {
        std::ifstream ifs;
        ifs.open(argv[ai]);
        if (!ifs) {
            perror(argv[ai]);
            return 1;
        }
        picojson::value v;
        ifs >> v;
        ResultListSet src;
        deserialize_result(src, v);

        for (auto && r : src.lists) {
            merge_result(rls, r.results, r.sysinfo);
        }
    }

    picojson::value v = serialize_result(rls);
    {
        std::ofstream ofs;
        ofs.open(argv[1]);
        if (!ofs) {
            perror(argv[1]);
            exit(1);
        }
        ofs << v;
    }

    return 0;
}
