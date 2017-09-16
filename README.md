# smirc - simple* MUD-IRC relay client

Just a little thing that was made during summer to make it easy to connect to MUD from phone or whatever device you want, without constantly reconnecting, provided that you have an IRC client.

It works by launching an IRC server (it's built into it), and then letting user, after they connect to the server, to connect the client to any MUD (or actually a telnet host) in most non-intrusive way that is possible with such setup.

---
Setup instruction:
* Build (`mkdir build && cd build && cmake .. && make`)
* Run it with `./smirc`
* Use your IRC client to connect to the smirc itself at `127.0.0.1:6969` (the default address/port combination)
* Connect to your MUD/MUCK/MOO/MUSH server by using `connect address +port`, the + is optional and is used to connect using SSL.
* Enjoy, mess around, peek into code, report the crashes and send info/stacktrace/dumps/how to reproduce them!

