#include "sys-microbenchmark.h"
#include <fstream>

using namespace smbm;

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("usage: show-result-info src");
        return 1;
    }

    ResultListSet rls;

    std::ifstream ifs;
    ifs.open(argv[1]);
    if (!ifs) {
        perror(argv[1]);
        return 1;
    }
    picojson::value v;
    ifs >> v;
    ResultListSet src;
    deserialize_result(src, v);

    int i = 0;
    for (auto &&r : src.lists) {
        std::cout << "result[" << i << "]::" << "os :" << r.sysinfo.os << "\n";
        std::cout << "result[" << i << "]::"
                  << "date :" << r.sysinfo.date << "\n";
        std::cout << "result[" << i << "]::" << "cpu :" << r.sysinfo.cpuid << "\n";
        i++;
    }

    return 0;
}
