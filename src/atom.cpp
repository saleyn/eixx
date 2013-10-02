#include <eixx/marshal/eterm.hpp>
#include <eixx/marshal/atom.hpp>
//#include <boost/pool/detail/singleton.hpp>

namespace EIXX_NAMESPACE {

    util::atom_table<>& marshal::atom::atom_table() {
        //return boost::details::pool::singleton_default<detail::atom_table<> >::instance();
        static util::atom_table<> s_atom_table;
        return s_atom_table;
    }

} // namespace EIXX_NAMESPACE

