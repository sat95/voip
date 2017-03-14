# VOIP
This is a simple implementation of voice communication over IP through sockets.
It makes use of of both BSD sockets and the pulseaudio libraries to achieve this. Dealing with hardware and device drivers of speakers and
microphone is bit difficult, so pulseaudio provides a easier way to handle speaker and microphone with high level APIs. Using those APIs
we can convert the input and output voice signal into stream of bytes. These APIs take cares of the device drivers interface which in turn handles
the actual hardware. And finally these stream of bytes can be transferred over internet using BSD sockets. As sockets are full duplex which makes it
suitable for real time voice transmission and reception concurrently.

- It contains two files USR1.c which is server and USR2.c which is client.                  
- USR1.c initiate the link and USR2.c connects to the link.
- Both USR1.c and USR2.c are capable of transmission and reception of audio bytes concurrently.
- A Makefile is included for the building of necessary executables usr1 and usr2.
