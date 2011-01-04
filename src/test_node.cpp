#include <eixx/alloc_std.hpp>
#include <eixx/eixx.hpp>
#include <iostream>
#include <boost/bind.hpp>

#define BOOST_REQUIRE

#include <eixx/connect/test_helper.hpp>

void usage(char* exe) {
    printf("Usage: %s -n NODE -r REMOTE_NODE [-c COOKIE] [-v VERBOSE] [-t RECONNECT_SECS]\n"
           "    -v VERBOSE          - verboseness: none|debug|message|wire|trace\n"
           "    -t RECONNECT_SECS   - reconnect timeout between reconnect attempts\n"
           "                          (default: 0 - no reconnecting)\n"
           , exe);
    exit(1);
}

using namespace EIXX_NAMESPACE;

otp_mailbox *g_io_server, *g_main;
static atom g_rem_node;

void on_io_request(otp_mailbox& a_mbox, boost::system::error_code& ec) {
    if (ec)
        std::cerr << "Mailbox " << a_mbox.self() << " got error: "
                  << ec.message() << std::endl;
    else {
        otp_mailbox::queue_type::iterator
            it = a_mbox.queue().begin(), end = a_mbox.queue().end();
        while (it != end) {
            std::cerr << "I/O server got a message: " << *(*it) << std::endl;
            delete *it;
            it = a_mbox.queue().erase(it);
        }
    }
    a_mbox.async_receive(&on_io_request);
}

void on_main_msg(otp_mailbox& a_mbox, boost::system::error_code& ec) {
    if (ec)
        std::cerr << "Mailbox " << a_mbox.self() << " got error: "
                  << ec.message() << std::endl;
    else {
        while (!a_mbox.queue().empty()) {
            boost::scoped_ptr<eixx::transport_msg> l_tmsg( a_mbox.queue().front() );
            a_mbox.queue().pop_front();

            const eterm& l_msg = l_tmsg->msg();

            std::cerr << "Main mailbox got a message: " << l_msg << std::endl;

            static eterm s_add_symbols = eterm::format("{From, {add_symbols, S}}");
            static eterm s_stop        = atom("stop");

            varbind l_binding;

            if (s_add_symbols.match(l_msg, &l_binding)) {
                const epid& l_from = l_binding.find("From")->to_pid();
                a_mbox.node().send_rpc_cast(a_mbox.self(), l_from.node(),
                    "ota_db", "commit_symbols", list::make(atom("add"), 1, *l_binding.find("S")));
                a_mbox.node().send(g_rem_node, l_from, atom("ok"));
            } else if (s_stop.match(l_msg, &l_binding)) {
                a_mbox.node().stop();
            }
        }
    }
    a_mbox.async_receive(&on_main_msg);
}

void on_connect(otp_connection* a_con, const std::string& a_error) {
    otp_node* l_node = a_con->node();

    if (!a_error.empty()) {
        std::cout << a_error << std::endl;
        return;
    }

    // Make sure that remote node has a process registered as "test".
    // Try sending a message to it.
    l_node->send(g_main->self(), a_con->remote_node(),
        "test", tuple::make(g_main->self(), "Hello world!"));

    // Send an rpc request to print a string. The remote 
    l_node->send_rpc(g_main->self(), a_con->remote_node(), atom("io"), atom("put_chars"),
        list::make("This is a test string"), &g_io_server->self());
}

int main(int argc, char* argv[]) {
    if (argc < 2)
        usage(argv[0]);
    
    const char* l_nodename = NULL, *l_remote = NULL, *use_cookie = "";
    connect::verbose_type verbose = connect::verboseness::level();
    int reconnect_secs = 0;

    for (int i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "-n") == 0 && i < argc-1)
            l_nodename = argv[++i];
        else if (strcmp(argv[i], "-r") == 0 && i < argc-1)
            l_remote = argv[++i];
        else if (strcmp(argv[i], "-c") == 0 && i < argc-1)
            use_cookie = argv[++i];
        else if (strcmp(argv[i], "-v") == 0 && i < argc-1)
            verbose = connect::verboseness::parse(argv[++i]);
        else if (strcmp(argv[i], "-t") == 0 && i < argc-1)
            reconnect_secs = atoi(argv[++i]);
        else
            usage(argv[0]);
    }            

    if (!l_nodename || !l_remote)
        usage(argv[0]);

    boost::asio::io_service io_service;
    otp_node l_node(io_service, l_nodename);
    l_node.verbose(verbose);

    g_io_server = l_node.create_mailbox("io_server");
    g_main      = l_node.create_mailbox("main");
    g_rem_node  = atom(l_remote);

    l_node.connect(&on_connect, l_remote, reconnect_secs);

    //otp_connection::connection_type* l_transport = a_con->transport();
    g_io_server->async_receive(&on_io_request);

    //l_node->send_rpc(self, a_con->remote_node(), atom("shell_default"), atom("ls"),
    //    list::make(), &io_server);
    g_main->async_receive(&on_main_msg);

    l_node.run();
    
    return 0;
}
