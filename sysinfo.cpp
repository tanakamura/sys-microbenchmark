#include "features.h"
#include <fstream>
#ifdef X86
#include <cpuid.h>
#endif

#ifdef POSIX
#include <sys/utsname.h>
#endif

#ifdef HAVE_CPUINFO
#include <cpuinfo.h>
#endif

#include "sys-microbenchmark.h"
#include <filesystem>

namespace smbm {

picojson::value get_sysinfo(GlobalState const *g) {
    std::map<std::string, picojson::value> obj;

    obj["ostimer"] = picojson::value(ostimer_value::name());
    obj["userland_timer"] = picojson::value(userland_timer_value::name());
    obj["perf_counter_available"] =
        picojson::value(g->is_hw_perf_counter_available());

    std::cout << "ostimer: " << ostimer_value::name() << '\n';
    std::cout << "userland_timer: " << userland_timer_value::name() << '\n';
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

    obj["cpuid"] = picojson::value((char *)data);
    puts((char *)data);

#elif defined HAVE_CPUINFO
    obj["cpuid"] = picojson::value(cpuinfo_get_package(0)->name);
#else
    obj["cpuid"] = picojson::value("unknown");
#endif

    {
        char buffer[256];
        time_t now = time(NULL);
        static struct tm *t = localtime(&now);
        strftime(buffer, 1024, "%Y-%m-%dT%H:%M:%d%z", t);
        obj["date"] = picojson::value(buffer);
    }

#ifdef __linux__
    std::string path = "/sys/devices/system/cpu/vulnerabilities";
    std::vector<picojson::value> valus;

    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        std::ifstream ifs;
        ifs.open(entry.path());
        if (ifs) {
            std::string line;
            std::getline(ifs, line);

            const char miti[] = "Mitigation:";
            const char na[] = "Not affected";

            if ((line.compare(0, sizeof(na) - 1, na) == 0) ||
                (line.compare(0, sizeof(miti) - 1, miti) == 0)) {
                /* pass */
            } else {
                valus.push_back(
                    picojson::value(std::string(entry.path().filename())));
            }
        }
        // puts(entry.path().c_str());
    }

    obj["vulnerabilities"] = picojson::value(valus);
#endif

#ifdef POSIX
    {
        struct utsname n;
        uname(&n);
        std::string uname = std::string(n.sysname) + " " + n.release + " " +
                            n.version + " " + n.machine;
        obj["os"] = picojson::value(uname);
    }

#elif defined WINDOWS
    obj["os"] = picojson::value("Windows");

#elif defined __wasi__
    obj["os"] = picojson::value("wasi");

#else
    obj["os"] = picojson::value("unknown");

#endif

    return picojson::value(obj);
}

picojson::value insert_result(picojson::value const &root,
                              picojson::value const &insobj_result,
                              picojson::value const &insobj_sysinfo) {
    auto vec = root.get<picojson::value::array>();

    std::string ins_cpuid = insobj_sysinfo.get("cpuid").get<std::string>();
    std::string ins_osname = insobj_sysinfo.get("os").get<std::string>();

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

    std::map<std::string, picojson::value> entry;
    entry["sys_info"] = insobj_sysinfo;
    entry["result"] = insobj_result;

    vec.push_back(picojson::value(entry));

    return picojson::value(vec);
}

} // namespace smbm
