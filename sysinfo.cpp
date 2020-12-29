#include "sys-microbenchmark.h"
#include "json.h"

#include <fstream>
#ifdef X86
#include <cpuid.h>
#endif

#include <dirent.h>

#ifdef POSIX
#include <sys/utsname.h>
#endif

#ifdef HAVE_CPUINFO
#include <cpuinfo.h>
#endif

namespace smbm {

SysInfo get_sysinfo(GlobalState const *g) {
    SysInfo ret;

    ret.ostimer = ostimer_value::name();
    ret.userland_timer = userland_timer_value::name();

    std::map<std::string, picojson::value> obj;

    ret.perf_counter_available = g->is_hw_perf_counter_available();
    ret.ooo_ratio = g->ooo_ratio;

    std::cout << "perf_counter: "
              << (g->is_hw_perf_counter_available() ? "yes" : "no") << '\n';

#ifdef X86

#define x_cpuid(p, eax) __get_cpuid(eax, &(p)[0], &(p)[1], &(p)[2], &(p)[3]);
    typedef unsigned int cpuid_t;

    cpuid_t data[4 * 3 + 1];

    x_cpuid(data + 4 * 0, 0x80000002);
    x_cpuid(data + 4 * 1, 0x80000003);
    x_cpuid(data + 4 * 2, 0x80000004);
    data[12] = 0;

    ret.cpuid = (char *)data;
    puts((char *)data);

#elif defined HAVE_CPUINFO
    ret.cpuid = cpuinfo_get_package(0)->name;
#else
    obj["cpuid"] = picojson::value("unknown");
#endif

    puts(cpuinfo_get_package(0)->name);

    {
        char buffer[256];
        time_t now = time(NULL);
        static struct tm *t = localtime(&now);
        strftime(buffer, 1024, "%Y-%m-%dT%H:%M:%d%z", t);
        ret.date = buffer;
    }

#ifdef __linux__
    std::string path = "/sys/devices/system/cpu/vulnerabilities";
    std::vector<picojson::value> valus;
    DIR *dir = opendir(path.c_str());

    // for (const auto &entry : std::filesystem::directory_iterator(path)) {
    if (dir) {
        while (1) {
            struct dirent *de = readdir(dir);
            if (de == nullptr) {
                break;
            }

            if (de->d_type != DT_REG) {
                continue;
            }

            std::ifstream ifs;
            ifs.open(path + "/" + de->d_name);
            if (ifs) {
                std::string line;
                std::getline(ifs, line);

                const char miti[] = "Mitigation:";
                const char na[] = "Not affected";

                if ((line.compare(0, sizeof(na) - 1, na) == 0) ||
                    (line.compare(0, sizeof(miti) - 1, miti) == 0)) {
                    /* pass */
                } else {
                    ret.vulnerabilities.push_back(de->d_name);
                }
            }
        }
        closedir(dir);
    } else {
        ret.vulnerabilities.push_back("unknow");
    }
#else
    ret.vulnerabilities.push_back("unknow");
#endif

#ifdef POSIX
    {
        struct utsname n;
        uname(&n);
        std::string uname = std::string(n.sysname) + " " + n.release + " " +
                            n.version + " " + n.machine;
        ret.os = uname;
    }

#elif defined WINDOWS
    ret.os = "Windows";

#elif defined __wasi__
    ret.os = "wasi";

#else
    ret.os = "unknown";

#endif

    return ret;
}

picojson::value insert_result(picojson::value const &root,
                              picojson::value const &insobj_result,
                              SysInfo const &insobj_sysinfo)
{
    using namespace json;

    auto vec = root.get<picojson::value::array>();

    const std::string &ins_cpuid = insobj_sysinfo.cpuid;
    const std::string &ins_osname = insobj_sysinfo.os;

    for (auto it = vec.begin(); it != vec.end();) {
        auto &si = it->get("sys_info");

        std::string cpuid0 = si.get("cpuid").get<std::string>();
        std::string osname0 = si.get("os").get<std::string>();

        if (cpuid0 == ins_cpuid && osname0 == ins_osname) {
            it = vec.erase(it);
        } else {
            ++it;
        }
    }

    std::map<std::string, picojson::value> json_sysinfo_map;
    json_sysinfo_map["cpuid"] = to_jv(insobj_sysinfo.cpuid);
    json_sysinfo_map["os"] = to_jv(insobj_sysinfo.os);
    json_sysinfo_map["vulnerabilities"] = to_jv(insobj_sysinfo.vulnerabilities);

    std::map<std::string, picojson::value> entry;
    entry["sys_info"] = picojson::value(json_sysinfo_map);
    entry["result"] = insobj_result;

    vec.push_back(picojson::value(entry));

    return picojson::value(vec);
}

} // namespace smbm
