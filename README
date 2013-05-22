eixx - Erlang C++ Interface Library
===================================

This library provides a set of classes for convenient marshaling
of Erlang terms between processes as well as connecting to other
distributed Erlang nodes from a C++ application.

The marshaling classes are built on top of ei library included in
{@link http://www.erlang.org/doc/apps/erl_interface erl_interface}.
It is largely inspired by the {@link http://code.google.com/p/epi epi}
project, but is a complete rewrite with many new features and 
optimization enhancements.

This library adds on the following features:
  - Use of reference-counted smart pointers for complex terms and
    by-value copied simple terms (e.i. integers, doubles, bool, atoms).
  - Ability to provide custom memory allocators.
  - Encoding/decoding of nested terms using a single function call
    (eterm::encode() and eterm::eterm() constructor).
  - Global atom table for fast manipulation of atoms.

The library consists of two separate parts:
  - Term marshaling (included by marshal.hpp or eixx.hpp)
  - Distributed node connectivity (included by connect.hpp or eixx.hpp)

The connectivity library implements a richer set of features than
what's available in erl_interface - it fully supports process
linking and monitoring.  The library is fully asynchronous and allows
handling many connections and mailboxes in one OS thread.

Ths library is dependend on {@link http://www.boost.org BOOST}
project and erl_interface, which is a part of the 
{@link www.erlang.org Erlang} distribution.

Downloading
===========
    Repository location: http://github.com/saleyn/eixx

    $ git clone git@github.com:saleyn/eixx.git

Building
========
    Make sure that you have autoconf-archive package installed:
        http://www.gnu.org/software/autoconf-archive

    Run:
    $ ./bootstrap
    $ ./configure --with-boost="/path/to/boost" [--with-erlang="/path/to/erlang"] \
        [--prefix="/target/install/path"]
    $ make
    $ make install      # Default install path is ./install

Author
======
    Serge Aleynikov <saleyn at gmail dot com>

LICENSE
=======
    GNU Lesser General Public License

Example
=======

Here's an example use of the eixx library:

    void on_message(otp_mailbox& a_mbox, boost::system::error_code& ec) {
        // On timeout ec == boost::asio::error::timeout
        if (ec == boost::asio::error::timeout) {
            std::cout << "Mailbox " << a_mbox.self() << " timeout: "
                      << ec.message() << std::endl;
        } else {
            // The mailbox has a queue of transport messages.
            // Dequeue next message from the mailbox.
            boost::scoped_ptr<transport_msg> dist_msg;

            while (dist_msg.reset(a_mbox.receive())) {
                std::cout << "Main mailbox got a distributed transport message:\n  "
                          << *dist_msg << std::endl;

                // Compile the following pattern into the corresponding abstract tree.
                // The expression must be a valid Erlang term
                static const eterm s_pattern = eterm::format("{From, {command, Cmd}}");

                varbind binding;

                // Pattern match the message and bind From and Cmd variables
                if (s_pattern.match(dist_msg->msg(), &binding)) {
                    const eterm* cmd = binding->find("Cmd");
                    std::cout << "Got a command " << *cmd << std::endl;
                    // Process command, e.g.:
                    // process_command(binding["From"]->to_pid(), *cmd);
                }
            }
        }
        
        // Schedule next async receive of a message (can also provide a timeout).
        a_mbox.async_receive(&on_message);
    }

    void on_connect(otp_connection* a_con, const std::string& a_error) {
        if (!a_error.empty()) {
            std::cout << a_error << std::endl;
            return;
        }

        // Illustrate creation of Erlang terms.
        eterm t1 = eterm::format("{ok, ~i}", 10);
        eterm t2 = tuple::make(10, 1.0, atom("test"), "abc");
        eterm t3("This is a string");
        eterm t4(tuple::make(t1, t2, t3));

        otp_node*    node = a_con->node();
        otp_mailbox* mbox = node->get_mailbox("main");

        // Send a message: {{ok, 10}, {10, 1.0, 'test', "abc"}, "This is a string"}
        // to an Erlang process named 'test' running
        // on node "abc@myhost"

        node->send(mbox.self(), a_con->remote_node(), atom("test"), t4);
    }

    int main() {
        use namespace eixx;

        otp_node node("abc");

        otp_mailbox* mbox = node.create_mailbox("main");

        node.connect(&on_connect, "abc@myhost");

        // Asynchronously receive a message with a deadline of 10s:
        mbox->async_receive(&on_message, 10000);

        node.run();
    }



Testing distributed transport
=============================

$ make

Run tests:

    $ LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./test_eterm

Test distributed transport:

    $ cd src

    # In this example we assume the host name of "fc12".

    [Shell A]$ erl -sname abc
    (abc@fc12)1> register(test, self()).

    [Shell B]$ ./test_node -n a@fc12 -r abc@fc12
    Connected to node: abc@fc12
    I/O server got a message:
        #DistMsg{
            type=SEND,
            cntrl={2,'',#Pid<a@fc12.1.0>},
            msg={io_request,#Pid<abc@fc12.273.0>,#Pid<a@fc12.1.0>,
                    {put_chars,<<"This is a test string">>}}}

The message above is a result of the on_connect() handler in test_node.cpp
issuing an rpc call to the abc@fc12 node of `io:put_chars("This is a test 
string")'. This the call selects a locally registered process called 
'io_server' as the group leader for this rpc call, the I/O output is sent 
to that mailbox.

Now you can try to send a message from Erlang to the 'main' mailbox
registered on the C++ node:

    [Shell A]
    (abc@fc12)2> {main, a@fc12} ! "This is a test!".

    [Shell B]
    Main mailbox got a message:
        #DistMsg{
            type=REG_SEND,
            cntrl={6,#Pid<abc@fc12.46.0>,'',main},
            msg="This is a test!"}

