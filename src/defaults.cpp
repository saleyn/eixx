#include <eixx/marshal/defaults.hpp>

namespace EIXX_NAMESPACE {

    const char* type_to_string(eterm_type a_type) {
        switch (a_type) {
            case LONG  : return "LONG";
            case DOUBLE: return "DOUBLE";
            case BOOL  : return "BOOL";
            case ATOM  : return "ATOM";
            case STRING: return "STRING";
            case BINARY: return "BINARY";
            case PID   : return "PID";
            case PORT  : return "PORT";
            case REF   : return "REF";
            case VAR   : return "VAR";
            case TUPLE : return "TUPLE";
            case LIST  : return "LIST";
            case TRACE : return "TRACE";
            default    : return "UNDEFINED";
        }
    }

    const char* type_to_type_string(eterm_type a_type, bool a_prefix) {
        switch (a_type) {
            case LONG  : return a_prefix ? "::int()"    : "int()";
            case DOUBLE: return a_prefix ? "::float()"  : "float()";
            case BOOL  : return a_prefix ? "::bool()"   : "bool()";
            case ATOM  : return a_prefix ? "::atom()"   : "atom()";
            case STRING: return a_prefix ? "::string()" : "string()";
            case BINARY: return a_prefix ? "::binary()" : "binary()";
            case PID   : return a_prefix ? "::pid()"    : "pid()";
            case PORT  : return a_prefix ? "::port()"   : "port()";
            case REF   : return a_prefix ? "::ref()"    : "ref()";
            case VAR   : return a_prefix ? "::var()"    : "var()";
            case TUPLE : return a_prefix ? "::tuple()"  : "tuple()";
            case LIST  : return a_prefix ? "::list()"   : "list()";
            case TRACE : return a_prefix ? "::trace()"  : "trace()";
            default    : return "";
        }
    }

    eterm_type type_string_to_type(const char* s, size_t n) {
        eterm_type r = UNDEFINED;

        if (n < 3) return r;

        int m = n - 1;
        const char* p = s+1;

        switch (s[0]) {
            case 'i':
                if (strncmp(p,"nt",m) == 0)         r = LONG;
                if (strncmp(p,"nteger",m) == 0)     r = LONG;
                break;
            case 'd':
                if (strncmp(p,"ouble",m) == 0)      r = DOUBLE;
                break;
            case 'f':
                if (strncmp(p,"loat",m) == 0)       r = DOUBLE;
                break;
            case 'b':
                if      (strncmp(p,"ool",m) == 0)   r = BOOL;
                else if (strncmp(p,"inary",m) == 0) r = BINARY;
                else if (strncmp(p,"oolean",m)== 0) r = BOOL;
                else if (strncmp(p,"yte",m) == 0)   r = LONG;
                break;
            case 'c':
                if (strncmp(p,"har",m) == 0)        r = LONG;
                break;
            case 'a':
                if (strncmp(p,"tom",m) == 0)        r = ATOM;
                break;
            case 's':
                if (strncmp(p,"tring",m) == 0)      r = STRING;
                break;
            case 'p':
                if      (strncmp(p,"id",m) == 0)    r = PID;
                else if (strncmp(p,"ort",m) == 0)   r = PORT;
                break;
            case 'r':
                if      (strncmp(p,"ef",m) == 0)       r = REF;
                else if (strncmp(p,"eference",m) == 0) r = REF;
                break;
            case 'v':
                if (strncmp(p,"ar",m) == 0)         r = VAR;
                break;
            case 't':
                if      (strncmp(p,"uple",m) == 0)  r = TUPLE;
                else if (strncmp(p,"race",m) == 0)  r = TRACE;
                break;
            case 'l':
                if (strncmp(p,"ist",m) == 0)        r = LIST;
                break;
            default:
                break;
        }
        return r;
    }

} // namespace EIXX_NAMESPACE

