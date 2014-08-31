Compiling
---------

    make

Running
-------

    erl -pa ebin
    application:start(manager).
    manager_server:send("Message").
    manager_server:test_cast("Cast").
