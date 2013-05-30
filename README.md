TCPeP
=====
TCPeP is a Performance-Enhancing proxy for TCP over lossy links, using Network Coding principles. It is partly based on the paper "Network coded tcp - CTCP".
Standard TCP algorithm were designed with wired networks in mind, assuming that all losses resulted from congestion events, when a router queue was overflowing. Coded-TCP is based on the assumption that losses can be completely random, as it occured in some wireless links.

Where does it work ?
====================
This system is supposed to improve throughput and latency of a communication, compared to standard TCP. The problem it addresses is the problem of random losses, hence you *should* be able to see an improvment if you experience this over your link.
Indeed, most modern wireless networks effectively hides random packet losses to the upper layers, so there might not be any random loss over your particular network.

I'm highly interested in any test cases or success story, as I do not have access to a wide range of networks to test the system for now.


How does it work ?
==================
The CTCP protocol works on two different levels :
    - The congestion control is based on RTT measurements, à la TCP-Vegas
    - The loss correction is based on Network Coding (see the Network_Coding.pdf file for more details)

How can I use it ?
==================
The system consists of a client software on the mobile host and a proxy side, on a ground server. The architecture is represented in the diagram below:
![This image represents the client/proxy architecture](./overview.png "Architecture Diagram")

Requirements:
1. Both hosts must run Linux (it also might work on other Unix system.)
2. You must have root access to the client
3. The Proxy must be connected to the Internet through a reliable link
4. You should be able to communicate via UDP between the Client and the Proxy

In order to use the system, you must:
1. Get the code:
`git clone "https://github.com/GregoireDelannoy/TCPeP.git"`
2. Compile:
`make all`
3. Run the Proxy:
`./tcpep -P -u <UDP PORT TO LISTEN ON> (-d)`
4. Run the Client:
`./tcpep -C <PROXY IP ADDRESS> -u <PROXY UDP PORT> -t <TCP PORT TO LISTEN ON> (-d)`
