## eixx - Erlang C++ Interface Library ##

This library provides a set of classes for convenient marshaling
of Erlang terms between processes as well as connecting to other
distributed Erlang nodes from a C++ application.

The marshaling classes are built on top of ei library included in
http://www.erlang.org/doc/apps/erl_interface.
It is largely inspired by the http://code.google.com/p/epi
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
  - Term marshaling (included by eterm.hpp or eixx.hpp).
  - Distributed node connectivity (included by connect.hpp or eixx.hpp)

If you are simply doing term marshalling only the eterm.hpp header along
with one of the alloc headers is needed. This allows for header-only
usage of the marshalling capabilities.

The connectivity library implements a richer set of features than
what's available in erl_interface - it fully supports process
linking and monitoring.  The library is fully asynchronous and allows
handling many connections and mailboxes in one OS thread.

Ths library is dependend on http://www.boost.org project and
erl_interface, which is a part of the www.erlang.org distribution.

### Downloading ###

Repository location: http://github.com/saleyn/eixx

    $ git clone git@github.com:saleyn/eixx.git

### Building ###

This library is dependent on BOOST.

If you need to customize location of BOOST or installation prefix, create a file called
`.cmake-args.${HOSTNAME}`. Alternatively if you are doing multi-host build with
identical configuration, create a file call `.cmake-args`. E.g.:

There are three sets of variables present in this file:

1. Build and install locations.

   * `DIR:BUILD=...`   - Build directory
   * `DIR:INSTALL=...` - Install directory

   They may contain macros:
   
      * `@PROJECT@`   - name of current project (from CMakeList.txt)
      * `@VERSION@`   - project version number  (from CMakeList.txt)
      * `@BUILD@`     - build type (from command line)
      * `${...}`      - environment variable

2. `ENV:VAR=...`     - Environment var set before running cmake

3. `VAR=...`         - Variable passed to cmake with -D prefix

Example:
```
$ cat > .cmake-args.${HOSTNAME}
DIR:BUILD=/tmp/@PROJECT@/build
DIR:INSTALL=/opt/pkt/@PROJECT@/@VERSION@
ENV:BOOST_ROOT=/opt/pkg/boost/current
ENV:BOOST_LIBRARYDIR=/opt/pkg/boost/current/gcc/lib
PKG_ROOT_DIR=/opt/pkg
```
Run:
```
$ make bootstrap [toolchain=gcc|clang]  [build=Debug|Release] \
                 [generator=make|ninja] [prefix=/usr/local] [verbose=true]
$ make [verbose=true]
$ make install      # Default install path is /usr/local
```
After running `make bootstrap` two local links are created `build` and `inst`
pointing to build and installation directories.

If you need to do a full cleanup of the current build and rerun bootstrap with
previously chosen options, do:
```
$ make distclean
$ make rebootstrap
```

### Author ###

Serge Aleynikov <saleyn at gmail dot com>

### LICENSE ###

Apache Public License v2.0

### Example ###

Manipulating Erlang terms is quite simple:
```cpp
eterm i = 10;
eterm s = "abc";
eterm a = atom("ok");
eterm t = {20.0, i, s, a};  // Constructs a tuple
eterm e = list{};           // Constructs an empty list
eterm l = list{i, 100.0, s, {a, 30}, list{}};   // Constructs a list
```
A convenient eterm::format() function implements an expression parser:

```cpp
eterm t1 = eterm::format("{ok, 10}");
eterm t2 = eterm::format("[1, 2, ok, [{one, 10}, {two, 20}]]");
eterm t3 = eterm::format("A::int()");            // t3 is a variable that can be matched
```

Pattern matching is done by constructing a pattern, and matching a term
against it. If varbind is provided, it'll store the values of all matched variables:

```cpp
static const eterm s_pattern = eterm::format("{ok, A::string()}");

eterm value = {atom("ok"), "abc"};

varbind binding;

if (value.match(s_pattern, &binding))
    std::cout << "Value of variable A: " << binding["A"].to_string() << std::endl;
```
Aside from providing functionality that allows to manipulate Erlang terms, this
library implements Erlang distributed transport that allows a C++ program to connect
to an Erlang node, exchange messages, make RPC calls, and receive I/O requests.
Here's an example use of the eixx library:

```cpp
void on_message(otp_mailbox& a_mbox, boost::system::error_code& ec) {
    // On timeout ec == boost::asio::error::timeout
    if (ec == boost::asio::error::timeout) {
        std::cout << "Mailbox " << a_mbox.self() << " timeout: "
                  << ec.message() << std::endl;
    } else {
        // The mailbox has a queue of transport messages.
        // Dequeue next message from the mailbox.
        std::unique_ptr<transport_msg> dist_msg;

        while (dist_msg.reset(a_mbox.receive())) {
            std::cout << "Main mailbox got a distributed transport message:\n  "
                      << *dist_msg << std::endl;

            // Compile the following pattern into the corresponding abstract tree.
            // The expression must be a valid Erlang term
            static const eterm s_pattern = eterm::format("{From, {command, Cmd}}");

            varbind binding;

            // Pattern match the message and bind From and Cmd variables
            if (dist_msg->msg().match(s_pattern, &binding)) {
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
```

### Testing distributed transport ###

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

