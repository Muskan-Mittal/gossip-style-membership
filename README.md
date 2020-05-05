# Gossip style Membership Protocol

This is my solution the programming assignment for the Cloud Computing Concepts course by University of Illinois.

Membership Protocol is a failure detection protocol used in distributed systems that monitors which processes are operating in the system, which have failed or have newly joined the system.
A membership potocol must perform two steps:  
**1. Failure detection**: Detect which processes have joined or left the system.  
**2. Dissemination**: Passes this informataion to other members in the system.

## Properties

**1. Completeness**: Membership Protocol should have 100% completeness i.e. any non-faulty process should be able to detect all the failures that happen it the system.  
**2. Accuracy**: It must also be very accurate close to 100% i.e. the number of false positives should be as low as possible.  
**3. Speed**: Time of first failure detected should be as low as possible.  
**4.Scale**: Each member should have equal load.   

Completeness and accuracy together can’t be achieved in lossy networks. Membership Protocol must ensure 100% completeness but we can afford little less than 100% accuracy as it will only add up additional overhead of false positives but we detect all the failures.

For the purpose of detecting failures, membership protocol can use different methods of heartbeating such as Centralized Heartbeating,  Ring Heartbeating, All to all heartbeating or gossip style heartbeating. I have implemented gossip based heart beating algorithm in this repository.

## Gossip Style Failure Detection

Each node maintains a membership list which contain information of all the nodes currently in the network. Each entry in the list contains the address, heartbeat counter and the local timestamp at which the entry was last updated. Each node gossips its membership list to b random nodes who update their lists.If a node's hertbeat counter is not since the cleanup time from the last entry it is removed from the list. This is how it works.
![alt text](https://github.com/Muskan-Mittal/gossip-style-membership/blob/master/images/GossipStyleFailureDetection.jpg "Membership Protocol")

## Implementation

For implementation purpose the problem since it is infeasible to run a thousand cluster nodes (peers) over a real network, they provided an implementation of an emulated network layer (EmulNet). Membership protocol implementation will sit above EmulNet in a peer- to-peer (P2P) layer, but below an App layer. It is like a three-layer protocol stack with Application, P2P, and EmulNet as the three layers (from top to bottom).

In the assignment I have implemented nodeLoopOps(), and the recvCallBack() functions. Both functions are invoked by nodeLoop() to periodically perform protocol routines. Each new peer contacts a well-known peer (the introducer) to join the group. This is implemented through JOINREQ and JOINREP messages. JOINREQ messages reach the introducer, and sends a JOINREP message to inform all the nodes in the system about new node being added. JOINREP messages specify the
cluster member list.
This implementation of membership protocol satisfies completeness all the
time (for joins and failures), and accuracy when there are no message delays or losses (high accuracy when there are losses or delays).

## Grader Testing 

To run the code on the grader tests use the following commands.

```bash
make
chmod +x Grader.sh
./Grader.sh
```
