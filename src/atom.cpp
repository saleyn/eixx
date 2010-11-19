#include <eixx/marshal/eterm.hpp>
#include <eixx/marshal/atom.hpp>
#include <boost/pool/detail/singleton.hpp>

namespace EIXX_NAMESPACE {
namespace marshal {

    detail::atom_table<>& atom::atom_table() {
        return boost::details::pool::singleton_default<detail::atom_table<> >::instance();
    }

} // namespace marshal
} // namespace EIXX_NAMESPACE

