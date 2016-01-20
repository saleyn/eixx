#include <eixx/alloc_std.hpp>
//#include "test_alloc.hpp"   // Uses boost::pool_alloc, which does much worse
#include <eixx/eixx.hpp>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

/// Prevent variable optimization by the compiler
#ifdef _MSC_VER
#pragma optimize("", off)
template <class T> void dont_optimize_var(T&& v) { v = v; }
#pragma optimize("", on)
#else
template <class T> void dont_optimize_var(T&& v) { asm volatile("":"+r" (v)); }
#endif

using namespace eixx;

int iterations=1000000;
size_t g_size = 0;
class timer {
    struct rusage start, end;
    void begin() { getrusage(RUSAGE_THREAD, &start); }
public:
    timer() { begin(); }

    void restart() { begin(); }

    void sample(const char* title, bool restart = true, size_t out = 0) {
        getrusage(RUSAGE_THREAD, &end);
        double diff = (double)(end.ru_utime.tv_sec  - start.ru_utime.tv_sec +
                               end.ru_stime.tv_sec  - start.ru_stime.tv_sec) +
                      (double)(end.ru_utime.tv_usec - start.ru_utime.tv_usec +
                               end.ru_stime.tv_usec - start.ru_stime.tv_usec)/1000000.0;
        g_size += out;

        // out is used merely to trick the optimizer
        printf("%30s | latency: %5ldns, speed: %9ld/s%s", title,
               long(1000000000.0*diff/iterations),
               diff > 0 ? long((double)iterations / diff) : 0,
               out == 0 ? "\n" : " \n");
        if (restart)
            begin();
    }
};


int main(int argc, char* argv[]) {
    for(int i=1; i < argc-1 && argv[i][0] == '-' && argv[i][1] != '\0'; i++) {
        switch (argv[i][1]) {
            case 'i': iterations    = atoi(argv[i+1]); break;
        }
    }

    printf("%30d iterations\n", iterations);

    timer tot;
    timer t;

    size_t size = 0;
    for (int j=0; j < iterations; j++)
        { size += eterm(j).encode_size(); }
    t.sample("Integer", true, size);
    for (int j=0; j < iterations; j++)
        { size += eterm((double)j).encode_size(); }
    t.sample("Double", true, size);
    for (int j=0; j < iterations; j++)
        { size += eterm(j&1).encode_size(); }
    t.sample("Bool", true, size);
    for (int j=0; j < iterations; j++)
        { size += eterm("test").encode_size(); }
    t.sample("String", true, size);
    for (int j=0; j < iterations; j++)
        { size += atom("test").encode_size(); }
    t.sample("Atom1", true, size);
    {
        atom a("test");
        int k = 0;
        for (int j=0; j < iterations; j++)
            { atom t("test"); if (t==a) k++; size += eterm(t).encode_size(); }
        t.sample("Atom2", true, size);
    }
    static const char* ss="This is a test string. This is a test string. This is a test string."; 
    for (int j=0; j < iterations; j++) { 
        binary b(ss, sizeof(ss)); size += eterm(b).encode_size();
    }
    t.sample("Binary1", true, size);
    {
        binary b(ss, sizeof(ss));
        for (int j=0; j < iterations; j++) { 
            eterm t(b);
            size += t.encode_size();
        }
        t.sample("Binary2", true, size);
    }

    eterm l[] = {
        eterm(10), eterm("atom_value"), eterm(true) };
    for (int j=0; j < iterations; j++)
        { tuple t(l); size += eterm(t).encode_size(); }
    t.sample("Tuple1", true, size);
    {
        tuple tup(l);
        for (int j=0; j < iterations; j++)
            { eterm et(tup); size += et.encode_size(); }
        t.sample("Tuple2", true, size);
    }
    for (int j=0; j < iterations; j++)
        { list t(l); size += eterm(t).encode_size(); }
    t.sample("List1", true, size);
    {
        list ll(l);
        for (int j=0; j < iterations; j++)
            { eterm et(ll); size += et.encode_size(); }
        t.sample("List2", true, size);
    }

    static const eterm s_md1 =
        eterm::format("{md, Xchg, Instr, [{q, [{BPx,BQty}], [{APx, AQty}]}]}");
    static const eterm s_md2 =
        eterm::format("{md, Xchg, Instr, [{q, L1, L2}]}");
    static const atom  am_Xchg ("Xchg");
    static const atom  am_Instr("Instr");
    static const atom  am_BPx  ("BPx");
    static const atom  am_AQty ("AQty");
    static const atom  am_APx  ("APx");
    static const atom  am_BQty ("BQty");
    static const atom  am_L1   ("L1");
    static const atom  am_L2   ("L2");
    atom   xchg ("CNX");
    atom   instr("EUR/USD");
    t.restart();
    {
        for (int j=0; j < iterations; j++) {
            auto x = s_md1.apply({{am_Xchg, xchg},  {am_Instr, instr},
                                  {am_BPx, 1.2345}, {am_BQty, 100000},
                                  {am_APx, 1.2355}, {am_AQty, 200000}});
            size += x.encode_size();
        }
        t.sample("Apply speed", true, size);
    }
    {
        for (int j=0; j < iterations; j++) {
            auto x = s_md2.apply({{am_Xchg, xchg},  {am_Instr, instr},
                                  {am_L1, list::make(tuple::make(1.2345, 100000))},
                                  {am_L2, list::make(tuple::make(1.2355, 200000))}});
            size += x.encode_size();
        }
        t.sample("Apply/Create speed", true, size);
    }
    atom am_md("md");
    atom am_q ("q");
    {
        iterations /= 10;
        for (int j=0, e = iterations; j < e; j++) {
            auto x = tuple::make(am_md, xchg, instr,
                        list::make(tuple::make(am_q,
                                    list::make(tuple::make(1.2345, 100000)),
                                    list::make(tuple::make(1.2355, 200000)))));
            size += x.encode_size();
        }
        t.sample("Nested lists/tuples (1) speed", true, size);
        iterations *= 10;
    }

    {
        iterations /= 10;
        for (int j=0, e = iterations; j < e; j++) {
            auto x = tuple{am_md, xchg, instr,
                        list{tuple{am_q,
                                   list{tuple{1.2345, 100000}},
                                   list{tuple{1.2355, 200000}}}}};
            size += x.encode_size();
        }
        t.sample("Nested lists/tuples (2) speed", true, size);
        iterations *= 10;
    }

    if (g_size == 0)
        std::cerr << "No iterations performed!" << std::endl;

    return 0;
}

