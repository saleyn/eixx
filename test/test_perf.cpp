#include "test_alloc.hpp"
#include <eixx/eixx.hpp>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

using namespace EIXX_NAMESPACE;

int iterations=500000;

class timer {
    struct rusage start, end;
    void begin() { getrusage(RUSAGE_THREAD, &start); }
public:
    timer() { begin(); }

    void sample(const char* title, bool restart = true) {
        getrusage(RUSAGE_THREAD, &end);
        double diff = (double)(end.ru_utime.tv_sec  - start.ru_utime.tv_sec +
                               end.ru_stime.tv_sec  - start.ru_stime.tv_sec) +
                      (double)(end.ru_utime.tv_usec - start.ru_utime.tv_usec +
                               end.ru_stime.tv_usec - start.ru_stime.tv_usec)/1000000.0;

        printf("%30s | %.6f (speed: %5.3fus)\n", title, diff, 1000000.0*diff/iterations);
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

    for (int j=0; j < iterations; j++)
        { eterm(1); }
    t.sample("Integer");
    for (int j=0; j < iterations; j++)
        { eterm(1.0); }
    t.sample("Double");
    for (int j=0; j < iterations; j++)
        { eterm(true); }
    t.sample("Bool");
    for (int j=0; j < iterations; j++)
        { eterm("test"); }
    t.sample("String");
    for (int j=0; j < iterations; j++)
        { atom("test"); }
    t.sample("Atom1");
    {
        atom a("test");
        int k = 0;
        for (int j=0; j < iterations; j++)
            { atom t("test"); if (t==a) k++; }
        t.sample("Atom2");
    }
    static const char* ss="This is a test string. This is a test string. This is a test string."; 
    for (int j=0; j < iterations; j++) { 
        binary b(ss, sizeof(ss));
    }
    t.sample("Binary1");
    {
        binary b(ss, sizeof(ss));
        for (int j=0; j < iterations; j++) { 
            eterm t(b);
        }
        t.sample("Binary2");
    }

    eterm l[] = {
        eterm(10), eterm("atom_value"), eterm(true) };
    for (int j=0; j < iterations; j++)
        { tuple t(l); }
    t.sample("Tuple1");
    {
        tuple tup(l);
        for (int j=0; j < iterations; j++)
            { eterm et(tup); }
        t.sample("Tuple2");
    }
    for (int j=0; j < iterations; j++)
        { list t(l); }
    t.sample("List1");
    {
        list ll(l);
        for (int j=0; j < iterations; j++)
            { eterm et(ll); }
        t.sample("List2");
    }
    tot.sample("Total time");

    return 0;
}

