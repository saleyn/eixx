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

} // namespace EIXX_NAMESPACE

