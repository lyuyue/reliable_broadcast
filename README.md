## Reliable Total Order Multicast Protocol: CS7680 Programming Assignment 2
### Yue Lyu

### Instruction:
Just `make` and run the binary with folloing command:<br />
`./proj2 -p port -h hostfile -c count`<br />

`-p port`            : port number    
`-h hostfile`        : path to file with hosts  
`-c count`           : maximum number of DataMessage expected to be sent by this host

Notice that host **MUST** be started in exact same order they show up in the hostfile.

### Test Cases:
- Typical size of the group is 3, while it is subjected to change.
- Maximum seconds of a message send delay is 2 seconds.
- Allow different number of hosts to send message concurrently.

###Code Architecture:  
**README.md**       
**Makefile**        
**constants.h**     : header file with constants definition   
**tcp_helper.h**    : header file introducing helper function with tcp sockets  
**message.h**       : header file with Message struct definition  
**main.c**          : main program  

### State diagrams:
![Alt](/diagram/reliable.png)


### Design decisions
- If two message are delivered with same **seq**, then seq with a lower id of proposer is considered 'happened before' the other one, in this case, we can ensure the total ordering.
- Multithread programming is applied, main thread only focus on listening from other hosts and deal with the message received.
- All send() are executed by a independent thread, a mutex_lock is implemented to keep key information **seq** safe, in this case we reach a balance between correctness and performance.
- A randomized delay is introduced to any Message send() to make a simulation of network latency/disconnected link. The maximum seconds a thread might wait before send the message is 2 seconds, while it is subjected to change (MAX_DELAY in constants.h).
- To better trace if the SeqMessage is received by target host, we have a new struct SeqAckMessage here to address problem raised by potential SeqMessage lost.
- Since we try to achieve some kind of 'reliable' multicast, a retransimit mechanism is added. The process would periodically check if any Message suffered a lost during transimission. If any, it reset record of AckMessage/SeqAckMessage and resend the original message. Interval length of this check can be customized by RELIABLE_THR in constants.h
- To ensure a total ordering, self_deliver happens after receiving all SeqAckMessage from other processes in the group.

### Implementation issues
- The implementation is some kind of 'tricky' since we are given the total number of message, if the number of message is not given, we need a dynamic mechanism for 'reliable' retransimission.
