#include "TCPQueueTesterFunctions.h"

#include "inet/common/packet/Packet.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"

using namespace inet;
using namespace inet::tcp;

void enqueue(TCPSendQueue *sq, const char *msgname, ulong numBytes)
{
    EV << "SQ:" << "enqueue(\"" << msgname << "\", " << numBytes << "):";

    Packet *msg = new Packet(msgname);
    const auto & bytes = makeShared<ByteCountChunk>(byte(numBytes));
    msg->insertTrailer(bytes);
    ASSERT(msg->getByteLength() == numBytes);
    sq->enqueueAppData(msg);

    EV << " --> " << sq->str() <<"\n";
}

void tryenqueue(TCPSendQueue *sq, const char *msgname, ulong numBytes)
{
    EV << "SQ:" << "enqueue(\"" << msgname << "\", " << numBytes << "):";

    Packet *msg = new Packet(msgname);
    const auto & bytes = makeShared<ByteCountChunk>(byte(numBytes));
    msg->insertTrailer(bytes);
    ASSERT(msg->getByteLength() == numBytes);
    try {
    sq->enqueueAppData(msg);
    } catch (cRuntimeError& e) {
        delete msg;
        EV << " --> Error: " << e.what();
    }

    EV << " --> " << sq->str() <<"\n";
}

Packet *createSegmentWithBytes(TCPSendQueue *sq, uint32 fromSeq, uint32 toSeq)
{
    EV << "SQ:" << "createSegmentWithBytes(" << fromSeq << ", " << toSeq << "):";

    ulong numBytes = toSeq-fromSeq;
    Packet *pk = sq->createSegmentWithBytes(fromSeq, numBytes);

    {
        Packet *tcpseg = pk->dup();
        uint32_t startSeq = fromSeq;
        for (int i = 0; tcpseg->getByteLength() > 0; i++)
        {
            const auto& payload = tcpseg->popHeader<Chunk>();
            int len = byte(payload->getChunkLength()).get();
            EV << (i?", ":" ") << payload->getClassName() << '[' << startSeq << ".." << startSeq + len <<')';
            startSeq += len;
        }
        EV << "\n";
        delete tcpseg;
    }

    const auto& tcpHdr = makeShared<TcpHeader>();
    tcpHdr->setSequenceNo(fromSeq);
    pk->insertHeader(tcpHdr);

    return pk;
}

void discardUpTo(TCPSendQueue *sq, uint32 seqNum)
{
    EV << "SQ:" << "discardUpTo(" << seqNum << "): ";
    sq->discardUpTo(seqNum);

    EV << sq->str() <<"\n";
}

//////////////////////////////////////////////////////////////

void insertSegment(TCPReceiveQueue *rq, Packet *tcpseg)
{
    const auto& tcphdr = tcpseg->peekHeader<TcpHeader>();
    uint32_t beg = tcphdr->getSequenceNo();
    uint32_t end = beg + tcpseg->getByteLength() - tcphdr->getHeaderLength();
    EV << "RQ:" << "insertSeg [" << beg << ".." << end << ")";
    uint32 rcv_nxt = rq->insertBytesFromSegment(tcpseg, tcphdr);
    delete tcpseg;
    EV << " --> " << rq->str() <<"\n";
}

void tryinsertSegment(TCPReceiveQueue *rq, Packet *tcpseg)
{
    const auto& tcphdr = tcpseg->peekHeader<TcpHeader>();
    uint32_t beg = tcphdr->getSequenceNo();
    uint32_t end = beg + tcpseg->getByteLength() - tcphdr->getHeaderLength();
    EV << "RQ:" << "insertSeg [" << beg << ".." << end << ")";
    try {
        uint32 rcv_nxt = rq->insertBytesFromSegment(tcpseg,tcphdr);
    } catch (cRuntimeError& e) {
        EV << " --> Error: " << e.what();
    }
    delete tcpseg;

    EV << " --> " << rq->str() <<"\n";
}

void extractBytesUpTo(TCPReceiveQueue *rq, uint32 seq)
{
    EV << "RQ:" << "extractUpTo(" << seq << "): <";
    cPacket *msg;
    while ((msg=rq->extractBytesUpTo(seq)) != NULL)
    {
        EV << " < " << msg->getName() << ": " << msg->getByteLength() << " bytes >";
        delete msg;
    }
    EV << " > --> " << rq->str() <<"\n";
}

/////////////////////////////////////////////////////////////////////////

void insertSegment(TCPReceiveQueue *q, uint32 beg, uint32 end)
{
    EV << "RQ:" << "insertSeg [" << beg << ".." << end << ")";

    Packet *msg = new Packet();
    unsigned int numBytes = end - beg;
    const auto& bytes = makeShared<ByteCountChunk>(byte(numBytes));
    msg->insertTrailer(bytes);
    const auto& tcpseg = makeShared<TcpHeader>();
    tcpseg->setSequenceNo(beg);
    msg->insertHeader(tcpseg);

    uint32 rcv_nxt = q->insertBytesFromSegment(msg, tcpseg);
    delete msg;

    EV << " --> " << q->str() <<"\n";
}

void tryinsertSegment(TCPReceiveQueue *q, uint32 beg, uint32 end)
{
    EV << "RQ:" << "insertSeg [" << beg << ".." << end << ")";

    Packet *msg = new Packet();
    unsigned int numBytes = end - beg;
    const auto& bytes = makeShared<ByteCountChunk>(byte(numBytes));
    msg->insertTrailer(bytes);
    const auto& tcpseg = makeShared<TcpHeader>();
    tcpseg->setSequenceNo(beg);
    msg->insertHeader(tcpseg);

    try {
        uint32 rcv_nxt = q->insertBytesFromSegment(msg, tcpseg);
    } catch (cRuntimeError& e) {
        EV << " --> Error: " << e.what();
    }
    delete msg;
    EV << " --> " << q->str() <<"\n";
}

