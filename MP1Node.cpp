/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"
#include <sstream>
/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	//int id = *(int*)(&memberNode->addr.addr);
	//int port = *(short*)(&memberNode->addr.addr[4]);

    if( memberNode == nullptr ) {
        return -1;
    }
	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
    return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
    MessageHdr *msg = (MessageHdr *) data;
    Address *addr = (Address *)(msg+1);
    
    int id = 0;
    short port;
    string address = addr->getAddress();
    memcpy(&id, &address[0], sizeof(int));
    memcpy(&port, &addr[5], sizeof(short));
    long heartbeat=0;
    memcpy(&heartbeat, ((msg+1) + 1 + sizeof(addr)), sizeof(long));

    if(msg->msgType == JOINREQ)
    {
        onHeartbeat(addr, heartbeat);
        addMemberToList(addr, id, port, heartbeat);
    }
    else if(msg->msgType == JOINREP)
    {
        onHeartbeat(addr, heartbeat);
        memberNode->inGroup = true;
        // stringstream msg;
        // msg << "JOINREP from " <<  address;
        // msg << " data " << to_string(heartbeat);
        // log->LOG(&memberNode->addr, msg.str().c_str());
    }else if(msg->msgType == GOSSIP)
    {
        onHeartbeat(addr, heartbeat);
    }
    else
    {
        log->LOG(&memberNode->addr, "received other messege");
    }
    return true;
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
    for(vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end();)
    {
        if(par->getcurrtime() - it->timestamp > TREMOVE)
        {
            Address curr_addr = getAddressFromMemberElement(&(*it)); 
            // stringstream ss;
            // ss << "Timing out " << curr_addr.getAddress();
            // log->LOG(&memberNode->addr, ss.str().c_str());
            // ss.str("");

            it = memberNode->memberList.erase(it);
            LogMembershipList();
            log->logNodeRemove(&(memberNode->addr), &curr_addr);
        }
        else
            it++;
    }
    memberNode->heartbeat++;
    onHeartbeat(&memberNode->addr, memberNode->heartbeat);
    gossip(&memberNode->addr, memberNode->heartbeat);
    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();

    int id = *(int*)(&memberNode->addr.addr);
    int port = *(short*)(&memberNode->addr.addr[4]);

    MemberListEntry entry = MemberListEntry(id, port);
    entry.settimestamp(par->getcurrtime());
    entry.setheartbeat(memberNode->heartbeat);
    memberNode->memberList.push_back(entry);
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}

void MP1Node::addMemberToList(Address *addr, int id, short port, long heartbeat)
{

    size_t msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr.addr) + sizeof(long) + 1;
    MessageHdr *msg = (MessageHdr *) malloc(msgsize * sizeof(char));

    // create JOINREP message: format of data is {struct Address myaddr}
    msg->msgType = JOINREP;
    memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
    memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));
    emulNet->ENsend(&memberNode->addr, addr, (char *)msg, msgsize);

    // stringstream ss;
    // ss<< "Sending JOINREP to " << addr->getAddress() <<" heartbeat "<<memberNode->heartbeat;
    // log->LOG(&memberNode->addr, ss.str().c_str());
    free(msg);

}

void MP1Node::onHeartbeat(Address *addr, long heartbeat)
{
    bool flag = false;
    for(vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end(); it++)
    {
        Address curr_addr = getAddressFromMemberElement(&(*it)); 
        if(curr_addr.addr == addr->addr)
        {
            flag = true;
            if(it->getheartbeat() < heartbeat)
            {
                it->setheartbeat(heartbeat);
                it->settimestamp(par->getcurrtime());  
            }
        }
    }

    if(!flag)
    {
        int id = 0;
        short port;
        string address = addr->getAddress();
        memcpy(&id, &address[0], sizeof(int));
        memcpy(&port, &addr[5], sizeof(short));
        MemberListEntry entry(id,port,heartbeat,par->getcurrtime());
        memberNode->memberList.push_back(entry);
        log->logNodeAdd(&(memberNode->addr), addr);
    }
}

void MP1Node::gossip(Address *updatedNodeAddr, long updatedNodeHeartbeat)
{
    int k=3;
    double beta = (double) k/ (double) memberNode->memberList.size();

    size_t msgsize = sizeof(MessageHdr) + sizeof(updatedNodeAddr->addr) + sizeof(long) + 1;
    MessageHdr *msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    
    // create GOSSIP message: format of data is {struct Address myaddr}
    msg->msgType = GOSSIP;
    memcpy((char *)(msg+1), updatedNodeAddr->addr, sizeof(updatedNodeAddr->addr));
    memcpy((char *)(msg+1) + 1 + sizeof(updatedNodeAddr->addr), &updatedNodeHeartbeat, sizeof(long));
    
    for(vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end(); it++)
    {
        Address dest_addr = getAddressFromMemberElement(&(*it)); 
        if( (dest_addr.addr == memberNode->addr.addr) || (dest_addr.addr==updatedNodeAddr->addr))
            continue;

        if(((double)(rand()%100))/100 < beta)
        {
            emulNet->ENsend(&memberNode->addr, &dest_addr, (char *)msg, msgsize);
        }
    }

}

void MP1Node::LogMembershipList() 
{
    stringstream msg;
    msg << "[";
    for (vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end(); it++) {
        msg << it->getid() << ": " << it->getheartbeat() << "(" << it->gettimestamp() << "), ";
    }
    msg << "]";
}

Address MP1Node::getAddressFromMemberElement(MemberListEntry *elem)
{
    Address addr;
    memcpy(&addr.addr[0], &elem->id, sizeof(int));
    memcpy(&addr.addr[4], &elem->port, sizeof(short));
    return addr;
}