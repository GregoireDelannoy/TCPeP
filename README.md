TCPeP
=====
TCPeP is inteded to be a Performance-Enhancing proxy for TCP over lossy links, using Network Coding methods. This is currently a work in progress...

Network :
--------
UDP tunnel in Python, using examples from secdev.org .
client => Creates a tun interface, read from it and pass it along to a proxy via UDP.
Proxy => Creates a tun interface and a UDP socket ; transfer things between them.

You will need some iptables magic to make it work ; basically to tell the kernel to use the tun interfaces as we intended.
On the client :
  - Set interface IP, in point-to-point mode :
    ifconfig tun0 <Client tun addr> pointopoint <Proxy tun addr>
  - Set the routing table to use the tun interface
    For example : ip route add default via <proxy tun addr>
  - Define a route to the proxy that could use a real interface
    ip route add <proxy public ip> via <local gateway> dev <output interface>

On the Proxy :
  - IP stuff :
    ifconfig tun0 <Proxy tun addr> pointopoint <client tun addr>
  - Enable IP forwarding
  - Set up a NAT to your output interface
    iptables -t nat -A POSTROUTING -o <output interface> -j MASQUERADE

and voilà, a nice little UDP tunneling, that enables you to do whatever you want with the transmitted data !

Encoding :
----------
My own humble try to implement Network Coding, as defined in "Network coding meets tcp : Theory and implementation."
You might want to read the README.pdf in the encoding directory to get an overview of the method.

The directory contains a quite straightforward C implementation of the network coding principles, but has no practical use for the moment.



Goal :
======
At some point, the idea is to mix the network tunneling prototype and the Network Coding scheme, to get a real-world implementation (in userland)  of the TCP/NC protocol, described in the previously cited paper.
