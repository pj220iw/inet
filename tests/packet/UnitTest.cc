//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/common/packet/ChunkBuffer.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/packet/ReassemblyBuffer.h"
#include "inet/common/packet/ReorderBuffer.h"
#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include "NewTest.h"
#include "UnitTest_m.h"
#include "UnitTest.h"

namespace inet {

Register_Serializer(CompoundHeader, CompoundHeaderSerializer);
Register_Serializer(TlvHeader, TlvHeaderSerializer);
Register_Serializer(TlvHeaderBool, TlvHeaderBoolSerializer);
Register_Serializer(TlvHeaderInt, TlvHeaderIntSerializer);
Define_Module(UnitTest);

#define ASSERT_ERROR(code, message) try { code; assert(false); } catch (std::exception& e) { assert(std::string(e.what()).find(message) != -1); }

static std::vector<uint8_t> makeVector(int length)
{
    std::vector<uint8_t> bytes;
    for (int i = 0; i < length; i++)
        bytes.push_back(i);
    return bytes;
}

static const Ptr<ByteCountChunk> makeImmutableByteCountChunk(B length)
{
    auto chunk = makeShared<ByteCountChunk>(length);
    chunk->markImmutable();
    return chunk;
}

static const Ptr<BytesChunk> makeImmutableBytesChunk(const std::vector<uint8_t>& bytes)
{
    auto chunk = makeShared<BytesChunk>(bytes);
    chunk->markImmutable();
    return chunk;
}

static const Ptr<ApplicationHeader> makeImmutableApplicationHeader(int someData)
{
    auto chunk = makeShared<ApplicationHeader>();
    chunk->setSomeData(someData);
    chunk->markImmutable();
    return chunk;
}

static const Ptr<TcpHeader> makeImmutableTcpHeader()
{
    auto chunk = makeShared<TcpHeader>();
    chunk->markImmutable();
    return chunk;
}

static const Ptr<IpHeader> makeImmutableIpHeader()
{
    auto chunk = makeShared<IpHeader>();
    chunk->markImmutable();
    return chunk;
}

static const Ptr<EthernetHeader> makeImmutableEthernetHeader()
{
    auto chunk = makeShared<EthernetHeader>();
    chunk->markImmutable();
    return chunk;
}

static const Ptr<EthernetTrailer> makeImmutableEthernetTrailer()
{
    auto chunk = makeShared<EthernetTrailer>();
    chunk->markImmutable();
    return chunk;
}

const Ptr<Chunk> CompoundHeaderSerializer::deserialize(MemoryInputStream& stream, const std::type_info& typeInfo) const
{
    auto compoundHeader = makeShared<CompoundHeader>();
    IpHeaderSerializer ipHeaderSerializer;
    auto ipHeader = ipHeaderSerializer.deserialize(stream);
    compoundHeader->insertAtEnd(ipHeader);
    return compoundHeader;
}

void TlvHeaderSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    throw cRuntimeError("Invalid operation");
}

const Ptr<Chunk> TlvHeaderSerializer::deserialize(MemoryInputStream& stream) const
{
    uint8_t type = stream.readUint8();
    stream.seek(stream.getPosition() - B(1));
    switch (type) {
        case 1:
            return TlvHeaderBoolSerializer().deserialize(stream);
        case 2:
            return TlvHeaderIntSerializer().deserialize(stream);
        default:
            throw cRuntimeError("Invalid TLV type");
    }
}

void TlvHeaderBoolSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    const auto& tlvHeader = staticPtrCast<const TlvHeaderBool>(chunk);
    stream.writeUint8(tlvHeader->getType());
    stream.writeUint8(B(tlvHeader->getChunkLength()).get());
    stream.writeUint8(tlvHeader->getBoolValue());
}

const Ptr<Chunk> TlvHeaderBoolSerializer::deserialize(MemoryInputStream& stream) const
{
    auto tlvHeader = makeShared<TlvHeaderBool>();
    assert(tlvHeader->getType() == stream.readUint8());
    auto x = B(tlvHeader->getChunkLength());
    auto y = B(stream.readUint8());
    assert(x == y);
    tlvHeader->setBoolValue(stream.readUint8());
    return tlvHeader;
}

void TlvHeaderIntSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    const auto& tlvHeader = staticPtrCast<const TlvHeaderInt>(chunk);
    stream.writeUint8(tlvHeader->getType());
    stream.writeUint8(B(tlvHeader->getChunkLength()).get());
    stream.writeUint16Be(tlvHeader->getInt16Value());
}

const Ptr<Chunk> TlvHeaderIntSerializer::deserialize(MemoryInputStream& stream) const
{
    auto tlvHeader = makeShared<TlvHeaderInt>();
    assert(tlvHeader->getType() == stream.readUint8());
    assert(B(tlvHeader->getChunkLength()) == B(stream.readUint8()));
    tlvHeader->setInt16Value(stream.readUint16Be());
    return tlvHeader;
}

static void testMutable()
{
    // 1. chunk is mutable after construction
    auto byteCountChunk1 = makeShared<ByteCountChunk>(B(10));
    assert(byteCountChunk1->isMutable());
}

static void testImmutable()
{
    // 1. chunk is immutable after marking it immutable
    auto byteCountChunk1 = makeShared<ByteCountChunk>(B(10));
    byteCountChunk1->markImmutable();
    assert(byteCountChunk1->isImmutable());

    // 2. chunk is not modifiable when it is immutable
    auto byteCountChunk2 = makeImmutableByteCountChunk(B(10));
    ASSERT_ERROR(byteCountChunk2->setLength(B(1)), "chunk is immutable");
    auto bytesChunk1 = makeImmutableBytesChunk(makeVector(10));
    ASSERT_ERROR(bytesChunk1->setByte(1, 0), "chunk is immutable");
    auto applicationHeader1 = makeImmutableApplicationHeader(42);
    ASSERT_ERROR(applicationHeader1->setSomeData(0), "chunk is immutable");
}

static void testComplete()
{
    // 1. chunk is complete after construction
    auto byteCountChunk1 = makeShared<ByteCountChunk>(B(10));
    assert(byteCountChunk1->isComplete());
}

static void testIncomplete()
{
    // 1. packet doesn't provide incomplete header if complete is requested but there's not enough data
    Packet packet1;
    packet1.append(makeImmutableApplicationHeader(42));
    Packet fragment1;
    fragment1.append(packet1.peekAt(B(0), B(5)));
    assert(!fragment1.hasHeader<ApplicationHeader>());
    ASSERT_ERROR(fragment1.peekHeader<ApplicationHeader>(), "incomplete chunk is not allowed");

    // 2. packet provides incomplete variable length header if requested
    Packet packet2;
    auto tcpHeader1 = makeShared<TcpHeader>();
    tcpHeader1->setChunkLength(B(16));
    tcpHeader1->setLengthField(16);
    tcpHeader1->setCrcMode(CRC_COMPUTED);
    tcpHeader1->setSrcPort(1000);
    tcpHeader1->setDestPort(1000);
    tcpHeader1->markImmutable();
    packet2.append(tcpHeader1);
    const auto& tcpHeader2 = packet2.popHeader<TcpHeader>(B(4), Chunk::PF_ALLOW_INCOMPLETE);
    assert(tcpHeader2->isIncomplete());
    assert(tcpHeader2->getChunkLength() == B(4));
    assert(tcpHeader2->getCrcMode() == CRC_COMPUTED);
    assert(tcpHeader2->getSrcPort() == 1000);

    // 3. packet provides incomplete variable length serialized header
    Packet packet3;
    auto tcpHeader3 = makeShared<TcpHeader>();
    tcpHeader3->setChunkLength(B(8));
    tcpHeader3->setLengthField(16);
    tcpHeader3->setCrcMode(CRC_COMPUTED);
    tcpHeader3->markImmutable();
    packet3.append(tcpHeader3);
    const auto& bytesChunk1 = packet3.peekAllBytes();
    assert(bytesChunk1->getChunkLength() == B(8));

    // 4. packet provides incomplete variable length deserialized header
    Packet packet4;
    packet4.append(bytesChunk1);
    const auto& tcpHeader4 = packet4.peekHeader<TcpHeader>(b(-1), Chunk::PF_ALLOW_INCOMPLETE);
    assert(tcpHeader4->isIncomplete());
    assert(tcpHeader4->getChunkLength() == B(8));
    assert(tcpHeader4->getLengthField() == 16);
}

static void testCorrect()
{
    // 1. chunk is correct after construction
    auto byteCountChunk1 = makeShared<ByteCountChunk>(B(10));
    assert(byteCountChunk1->isCorrect());
}

static void testIncorrect()
{
    // 1. chunk is incorrect after marking it incorrect
    auto applicationHeader1 = makeImmutableApplicationHeader(42);
    applicationHeader1->markIncorrect();
    assert(applicationHeader1->isIncorrect());
}

static void testProperlyRepresented()
{
    // 1. chunk is proper after construction
    auto byteCountChunk1 = makeShared<ByteCountChunk>(B(10));
    assert(byteCountChunk1->isProperlyRepresented());
}

static void testImproperlyRepresented()
{
    // 1. chunk is improperly represented after deserialization of a non-representable packet
    Packet packet1;
    auto ipHeader1 = makeShared<IpHeader>();
    ipHeader1->markImmutable();
    packet1.append(ipHeader1);
    assert(ipHeader1->isProperlyRepresented());
    auto bytesChunk1 = staticPtrCast<BytesChunk>(packet1.peekAllBytes()->dupShared());
    bytesChunk1->setByte(0, 42);
    bytesChunk1->markImmutable();
    Packet packet2(nullptr, bytesChunk1);
    const auto& ipHeader2 = packet2.peekHeader<IpHeader>(b(-1), Chunk::PF_ALLOW_IMPROPERLY_REPRESENTED);
    assert(ipHeader2->isImproperlyRepresented());
}

static void testEmpty()
{
    // 1. peeking an empty packet is an error
    Packet packet1;
    ASSERT_ERROR(packet1.peekHeader<IpHeader>(), "empty chunk is not allowed");
    ASSERT_ERROR(packet1.peekTrailer<IpHeader>(), "empty chunk is not allowed");
}

static void testHeader()
{
    // 1. packet contains header after chunk is appended
    Packet packet1;
    packet1.pushHeader(makeImmutableByteCountChunk(B(10)));
    const auto& chunk1 = packet1.peekHeader();
    assert(chunk1 != nullptr);
    assert(chunk1->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk1) != nullptr);
    const auto& chunk2 = packet1.peekHeader<ByteCountChunk>();
    assert(chunk2 != nullptr);
    assert(chunk2->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk2) != nullptr);

    // 2. packet moves header pointer after pop
    const auto& chunk3 = packet1.popHeader<ByteCountChunk>();
    assert(chunk3 != nullptr);
    assert(chunk3->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk3) != nullptr);
    assert(packet1.getHeaderPopOffset() == B(10));

    // 3. packet provides headers in reverse prepend order
    Packet packet2;
    packet2.pushHeader(makeImmutableBytesChunk(makeVector(10)));
    packet2.pushHeader(makeImmutableByteCountChunk(B(10)));
    const auto& chunk4 = packet2.popHeader<ByteCountChunk>();
    const auto& chunk5 = packet2.popHeader<BytesChunk>();
    assert(chunk4 != nullptr);
    assert(chunk4->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk4) != nullptr);
    assert(chunk5 != nullptr);
    assert(chunk5->getChunkLength() == B(10));
    assert(dynamicPtrCast<const BytesChunk>(chunk5) != nullptr);
    const auto& bytesChunk1 = staticPtrCast<const BytesChunk>(chunk5);
    assert(std::equal(bytesChunk1->getBytes().begin(), bytesChunk1->getBytes().end(), makeVector(10).begin()));

    // 4. packet provides header from bytes
    Packet packet3;
    auto bytesChunk2 = makeShared<BytesChunk>();
    bytesChunk2->setBytes({2, 4, 0, 42});
    bytesChunk2->markImmutable();
    packet3.pushHeader(bytesChunk2);
    auto tlvHeader1 = packet3.peekHeader<TlvHeaderInt>();
    assert(tlvHeader1->getInt16Value() == 42);

    // 5. packet provides mutable headers without duplication if possible
    Packet packet4;
    packet4.pushHeader(makeImmutableBytesChunk(makeVector(10)));
    const auto& chunk6 = packet4.peekHeader<BytesChunk>().get();
    const auto& chunk7 = packet4.removeHeader<BytesChunk>(B(10));
    assert(chunk7.get() == chunk6);
    assert(chunk7->isMutable());
    assert(chunk7->getChunkLength() == B(10));
    assert(packet4.getTotalLength() == B(0));
    const auto& bytesChunk3 = staticPtrCast<const BytesChunk>(chunk7);
    assert(std::equal(bytesChunk3->getBytes().begin(), bytesChunk3->getBytes().end(), makeVector(10).begin()));
}

static void testTrailer()
{
    // 1. packet contains trailer after chunk is appended
    Packet packet1;
    packet1.pushTrailer(makeImmutableByteCountChunk(B(10)));
    const auto& chunk1 = packet1.peekTrailer();
    assert(chunk1 != nullptr);
    assert(chunk1->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk1) != nullptr);
    const auto& chunk2 = packet1.peekTrailer<ByteCountChunk>();
    assert(chunk2 != nullptr);
    assert(chunk2->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk2) != nullptr);

    // 2. packet moves trailer pointer after pop
    const auto& chunk3 = packet1.popTrailer<ByteCountChunk>();
    assert(chunk3 != nullptr);
    assert(chunk3->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk3) != nullptr);
    assert(packet1.getTrailerPopOffset() == b(0));

    // 3. packet provides trailers in reverse order
    Packet packet2;
    packet2.pushTrailer(makeImmutableBytesChunk(makeVector(10)));
    packet2.pushTrailer(makeImmutableByteCountChunk(B(10)));
    const auto& chunk4 = packet2.popTrailer<ByteCountChunk>();
    const auto& chunk5 = packet2.popTrailer<BytesChunk>();
    assert(chunk4 != nullptr);
    assert(chunk4->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk4) != nullptr);
    assert(chunk5 != nullptr);
    assert(chunk5->getChunkLength() == B(10));
    assert(dynamicPtrCast<const BytesChunk>(chunk5) != nullptr);
    const auto& bytesChunk1 = staticPtrCast<const BytesChunk>(chunk5);
    assert(std::equal(bytesChunk1->getBytes().begin(), bytesChunk1->getBytes().end(), makeVector(10).begin()));

    // 4. packet provides trailer from bytes but only when length is provided
    Packet packet3;
    auto bytesChunk2 = makeShared<BytesChunk>();
    bytesChunk2->setBytes({2, 4, 0, 42});
    bytesChunk2->markImmutable();
    packet3.pushTrailer(bytesChunk2);
    // TODO: ASSERT_ERROR(packet3.peekTrailer<TlvHeaderInt>(), "isForward()");
    auto tlvTrailer1 = packet3.peekTrailer<TlvHeaderInt>(B(4));
    assert(tlvTrailer1->getInt16Value() == 42);

    // 5. packet provides mutable trailers without duplication if possible
    Packet packet4;
    packet4.pushTrailer(makeImmutableBytesChunk(makeVector(10)));
    const auto& chunk6 = packet4.peekTrailer<BytesChunk>().get();
    const auto& chunk7 = packet4.removeTrailer<BytesChunk>(B(10));
    assert(chunk7.get() == chunk6);
    assert(chunk7->isMutable());
    assert(chunk7->getChunkLength() == B(10));
    assert(packet4.getTotalLength() == B(0));
    const auto& bytesChunk3 = staticPtrCast<const BytesChunk>(chunk7);
    assert(std::equal(bytesChunk3->getBytes().begin(), bytesChunk3->getBytes().end(), makeVector(10).begin()));
}

static void testHeaderPopOffset()
{
    // 1. TODO
    Packet packet1;
    packet1.append(makeImmutableByteCountChunk(B(10)));
    packet1.append(makeImmutableBytesChunk(makeVector(10)));
    packet1.append(makeImmutableApplicationHeader(42));
    packet1.append(makeImmutableIpHeader());
    packet1.setHeaderPopOffset(B(0));
    const auto& chunk1 = packet1.peekHeader();
    assert(dynamicPtrCast<const ByteCountChunk>(chunk1));
    packet1.setHeaderPopOffset(B(10));
    const auto& chunk2 = packet1.peekHeader();
    assert(dynamicPtrCast<const BytesChunk>(chunk2));
    packet1.setHeaderPopOffset(B(20));
    const auto& chunk3 = packet1.peekHeader();
    assert(dynamicPtrCast<const ApplicationHeader>(chunk3));
    packet1.setHeaderPopOffset(B(30));
    const auto& chunk4 = packet1.peekHeader();
    assert(dynamicPtrCast<const IpHeader>(chunk4));
    packet1.setHeaderPopOffset(B(50));
    ASSERT_ERROR(packet1.peekHeader(), "empty chunk is not allowed");
}

static void testTrailerPopOffset()
{
    // 1. TODO
    Packet packet1;
    packet1.append(makeImmutableByteCountChunk(B(10)));
    packet1.append(makeImmutableBytesChunk(makeVector(10)));
    packet1.append(makeImmutableApplicationHeader(42));
    packet1.append(makeImmutableIpHeader());
    packet1.setTrailerPopOffset(B(50));
    const auto& chunk1 = packet1.peekTrailer();
    assert(dynamicPtrCast<const IpHeader>(chunk1));
    packet1.setTrailerPopOffset(B(30));
    const auto& chunk2 = packet1.peekTrailer();
    assert(dynamicPtrCast<const ApplicationHeader>(chunk2));
    packet1.setTrailerPopOffset(B(20));
    const auto& chunk3 = packet1.peekTrailer();
    assert(dynamicPtrCast<const BytesChunk>(chunk3));
    packet1.setTrailerPopOffset(B(10));
    const auto& chunk4 = packet1.peekTrailer();
    assert(dynamicPtrCast<const ByteCountChunk>(chunk4));
    packet1.setTrailerPopOffset(B(0));
    ASSERT_ERROR(packet1.peekTrailer(), "empty chunk is not allowed");
}

static void testEncapsulation()
{
    // 1. packet contains all chunks of encapsulated packet as is
    Packet packet1;
    packet1.append(makeImmutableByteCountChunk(B(10)));
    packet1.append(makeImmutableBytesChunk(makeVector(10)));
    // encapsulation packet with header and trailer
    auto& packet2 = packet1;
    packet2.pushHeader(makeImmutableEthernetHeader());
    packet2.pushTrailer(makeImmutableEthernetTrailer());
    const auto& ethernetHeader1 = packet2.popHeader<EthernetHeader>();
    const auto& ethernetTrailer1 = packet2.popTrailer<EthernetTrailer>();
    const auto& byteCountChunk1 = packet2.peekDataAt(B(0), B(10));
    const auto& bytesChunk1 = packet2.peekDataAt(B(10), B(10));
    const auto& dataChunk1 = packet2.peekDataBytes();
    assert(ethernetHeader1 != nullptr);
    assert(ethernetTrailer1 != nullptr);
    assert(byteCountChunk1 != nullptr);
    assert(bytesChunk1 != nullptr);
    assert(dynamicPtrCast<const ByteCountChunk>(byteCountChunk1) != nullptr);
    assert(dynamicPtrCast<const BytesChunk>(bytesChunk1) != nullptr);
    assert(byteCountChunk1->getChunkLength() == B(10));
    assert(bytesChunk1->getChunkLength() == B(10));
    assert(dataChunk1->getChunkLength() == B(20));
}

static void testAggregation()
{
    // 1. packet contains all chunks of aggregated packets as is
    Packet packet1;
    packet1.append(makeImmutableByteCountChunk(B(10)));
    Packet packet2;
    packet2.append(makeImmutableBytesChunk(makeVector(10)));
    Packet packet3;
    packet3.append(makeImmutableIpHeader());
    // aggregate other packets
    packet3.append(packet1.peekAt(b(0), packet2.getTotalLength()));
    packet3.append(packet2.peekAt(b(0), packet2.getTotalLength()));
    const auto& ipHeader1 = packet3.popHeader<IpHeader>();
    const auto& chunk1 = packet3.peekDataAt(B(0), B(10));
    const auto& chunk2 = packet3.peekDataAt(B(10), B(10));
    assert(ipHeader1 != nullptr);
    assert(chunk1 != nullptr);
    assert(chunk1->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk1) != nullptr);
    assert(chunk2 != nullptr);
    assert(chunk2->getChunkLength() == B(10));
    assert(dynamicPtrCast<const BytesChunk>(chunk2) != nullptr);
    const auto& bytesChunk1 = staticPtrCast<const BytesChunk>(chunk2);
    assert(std::equal(bytesChunk1->getBytes().begin(), bytesChunk1->getBytes().end(), makeVector(10).begin()));
}

static void testFragmentation()
{
    // 1. packet contains fragment of another packet
    Packet packet1;
    packet1.append(makeImmutableByteCountChunk(B(10)));
    packet1.append(makeImmutableBytesChunk(makeVector(10)));
    Packet packet2;
    packet2.append(makeImmutableIpHeader());
    // append fragment of another packet
    packet2.append(packet1.peekAt(B(7), B(10)));
    const auto& ipHeader1 = packet2.popHeader<IpHeader>();
    const auto& fragment1 = packet2.peekDataAt(b(0), packet2.getDataLength());
    const auto& chunk1 = fragment1->peek(B(0), B(3));
    const auto& chunk2 = fragment1->peek(B(3), B(7));
    assert(packet2.getTotalLength() == B(30));
    assert(ipHeader1 != nullptr);
    assert(dynamicPtrCast<const IpHeader>(ipHeader1) != nullptr);
    assert(fragment1 != nullptr);
    assert(fragment1->getChunkLength() == B(10));
    assert(chunk1 != nullptr);
    assert(chunk1->getChunkLength() == B(3));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk1) != nullptr);
    assert(chunk2 != nullptr);
    assert(chunk2->getChunkLength() == B(7));
    assert(dynamicPtrCast<const BytesChunk>(chunk2) != nullptr);
    const auto& bytesChunk1 = staticPtrCast<const BytesChunk>(chunk2);
    assert(std::equal(bytesChunk1->getBytes().begin(), bytesChunk1->getBytes().end(), makeVector(7).begin()));
}

static void testPolymorphism()
{
    // 1. packet provides headers in a polymorphic way without serialization
    Packet packet1;
    auto tlvHeader1 = makeShared<TlvHeaderBool>();
    tlvHeader1->setBoolValue(true);
    tlvHeader1->markImmutable();
    packet1.append(tlvHeader1);
    auto tlvHeader2 = makeShared<TlvHeaderInt>();
    tlvHeader2->setInt16Value(42);
    tlvHeader2->markImmutable();
    packet1.append(tlvHeader2);
    const auto& tlvHeader3 = packet1.popHeader<TlvHeader>();
    assert(tlvHeader3 != nullptr);
    assert(tlvHeader3->getChunkLength() == B(3));
    assert(dynamicPtrCast<const TlvHeaderBool>(tlvHeader3) != nullptr);
    const auto & tlvHeaderBool1 = staticPtrCast<const TlvHeaderBool>(tlvHeader3);
    assert(tlvHeaderBool1->getBoolValue());
    const auto& tlvHeader4 = packet1.popHeader<TlvHeader>();
    assert(tlvHeader4 != nullptr);
    assert(tlvHeader4->getChunkLength() == B(4));
    assert(dynamicPtrCast<const TlvHeaderInt>(tlvHeader4) != nullptr);
    const auto & tlvHeaderInt1 = staticPtrCast<const TlvHeaderInt>(tlvHeader4);
    assert(tlvHeaderInt1->getInt16Value() == 42);

    // 2. packet provides deserialized headers in a polymorphic way after serialization
    Packet packet2(nullptr, packet1.peekAllBytes());
    const auto& tlvHeader5 = packet2.popHeader<TlvHeader>();
    assert(tlvHeader5 != nullptr);
    assert(tlvHeader5->getChunkLength() == B(3));
    assert(dynamicPtrCast<const TlvHeaderBool>(tlvHeader5) != nullptr);
    const auto & tlvHeaderBool2 = staticPtrCast<const TlvHeaderBool>(tlvHeader5);
    assert(tlvHeaderBool2->getBoolValue());
    const auto& tlvHeader6 = packet2.popHeader<TlvHeader>();
    assert(tlvHeader6 != nullptr);
    assert(tlvHeader6->getChunkLength() == B(4));
    assert(dynamicPtrCast<const TlvHeaderInt>(tlvHeader6) != nullptr);
    const auto & tlvHeaderInt2 = staticPtrCast<const TlvHeaderInt>(tlvHeader6);
    assert(tlvHeaderInt2->getInt16Value() == 42);
}

static void testStreaming()
{
    // 1. bits
    MemoryOutputStream outputStreamBits;
    outputStreamBits.writeBit(true);
    outputStreamBits.writeBitRepeatedly(false, 10);
    std::vector<bool> writeBitsVector = {true, false, true, false, true, false, true, false, true, false};
    outputStreamBits.writeBits(writeBitsVector);
    std::vector<bool> writeBitsData;
    outputStreamBits.copyData(writeBitsData);
    assert(outputStreamBits.getLength() == b(21));
    MemoryInputStream inputStreamBits(outputStreamBits.getData(), outputStreamBits.getLength());
    assert(inputStreamBits.getLength() == b(21));
    assert(inputStreamBits.readBit() == true);
    assert(inputStreamBits.readBitRepeatedly(false, 10));
    std::vector<bool> readBitsVector;
    inputStreamBits.readBits(readBitsVector, b(10));
    assert(std::equal(readBitsVector.begin(), readBitsVector.end(), writeBitsVector.begin()));
    std::vector<bool> readBitsData;
    inputStreamBits.copyData(readBitsData);
    assert(std::equal(readBitsData.begin(), readBitsData.end(), writeBitsData.begin()));
    assert(!inputStreamBits.isReadBeyondEnd());
    assert(inputStreamBits.getRemainingLength() == b(0));
    inputStreamBits.readBit();
    assert(inputStreamBits.isReadBeyondEnd());
    assert(inputStreamBits.getRemainingLength() == B(0));

    // 2. bytes
    MemoryOutputStream outputStreamBytes;
    outputStreamBytes.writeByte(42);
    outputStreamBytes.writeByteRepeatedly(21, 10);
    auto writeBytesVector = makeVector(10);
    outputStreamBytes.writeBytes(writeBytesVector);
    uint8_t writeBytesBuffer[10] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    outputStreamBytes.writeBytes(writeBytesBuffer, B(10));
    std::vector<bool> writeBytesData;
    outputStreamBytes.copyData(writeBytesData);
    assert(outputStreamBytes.getLength() == B(31));
    MemoryInputStream inputStreamBytes(outputStreamBytes.getData());
    assert(inputStreamBytes.getLength() == B(31));
    assert(inputStreamBytes.readByte() == 42);
    assert(inputStreamBytes.readByteRepeatedly(21, 10));
    std::vector<uint8_t> readBytesVector;
    inputStreamBytes.readBytes(readBytesVector, B(10));
    assert(std::equal(readBytesVector.begin(), readBytesVector.end(), writeBytesVector.begin()));
    uint8_t readBytesBuffer[10];
    inputStreamBytes.readBytes(readBytesBuffer, B(10));
    assert(!memcmp(writeBytesBuffer, readBytesBuffer, 10));
    std::vector<bool> readBytesData;
    inputStreamBytes.copyData(readBytesData);
    assert(std::equal(readBytesData.begin(), readBytesData.end(), writeBytesData.begin()));
    assert(!inputStreamBytes.isReadBeyondEnd());
    assert(inputStreamBytes.getRemainingLength() == B(0));
    inputStreamBytes.readByte();
    assert(inputStreamBytes.isReadBeyondEnd());
    assert(inputStreamBytes.getRemainingLength() == B(0));

    // 3. bit-byte conversion
    MemoryOutputStream outputStreamConversion;
    outputStreamConversion.writeBits({false, false, false, false, true, true, true, true});
    outputStreamConversion.writeBits({true, true, true, true, false, false, false, false});
    MemoryInputStream inputStreamConversion(outputStreamConversion.getData());
    std::vector<uint8_t> data;
    inputStreamConversion.readBytes(data, B(2));
    assert(data[0] == 0x0F);
    assert(data[1] == 0xF0);

    // 4. uint8_t
    uint64_t uint8 = 0x42;
    MemoryOutputStream outputStream1;
    outputStream1.writeUint8(uint8);
    MemoryInputStream inputStream1(outputStream1.getData());
    assert(inputStream1.readUint8() == uint8);
    assert(!inputStream1.isReadBeyondEnd());
    assert(inputStream1.getRemainingLength() == b(0));

    // 5. uint16_t
    uint64_t uint16 = 0x4242;
    MemoryOutputStream outputStream2;
    outputStream2.writeUint16Be(uint16);
    MemoryInputStream inputStream2(outputStream2.getData());
    assert(inputStream2.readUint16Be() == uint16);
    assert(!inputStream2.isReadBeyondEnd());
    assert(inputStream2.getRemainingLength() == b(0));

    // 6. uint32_t
    uint64_t uint32 = 0x42424242;
    MemoryOutputStream outputStream3;
    outputStream3.writeUint32Be(uint32);
    MemoryInputStream inputStream3(outputStream3.getData());
    assert(inputStream3.readUint32Be() == uint32);
    assert(!inputStream3.isReadBeyondEnd());
    assert(inputStream3.getRemainingLength() == b(0));

    // 7. uint64_t
    uint64_t uint64 = 0x4242424242424242L;
    MemoryOutputStream outputStream4;
    outputStream4.writeUint64Be(uint64);
    MemoryInputStream inputStream4(outputStream4.getData());
    assert(inputStream4.readUint64Be() == uint64);
    assert(!inputStream4.isReadBeyondEnd());
    assert(inputStream4.getRemainingLength() == b(0));

    // 8. MACAddress
    MACAddress macAddress("0A:AA:01:02:03:04");
    MemoryOutputStream outputStream5;
    outputStream5.writeMACAddress(macAddress);
    MemoryInputStream inputStream5(outputStream5.getData());
    assert(inputStream5.readMACAddress() ==macAddress);
    assert(!inputStream5.isReadBeyondEnd());
    assert(inputStream5.getRemainingLength() == b(0));

    // 9. IPv4Address
    IPv4Address ipv4Address("192.168.10.1");
    MemoryOutputStream outputStream6;
    outputStream6.writeIPv4Address(ipv4Address);
    MemoryInputStream inputStream6(outputStream6.getData());
    assert(inputStream6.readIPv4Address() == ipv4Address);
    assert(!inputStream6.isReadBeyondEnd());
    assert(inputStream6.getRemainingLength() == b(0));

    // 10. IPv6Address
    IPv6Address ipv6Address("1011:1213:1415:1617:1819:2021:2223:2425");
    MemoryOutputStream outputStream7;
    outputStream7.writeIPv6Address(ipv6Address);
    MemoryInputStream inputStream7(outputStream7.getData());
    assert(inputStream7.readIPv6Address() == ipv6Address);
    assert(!inputStream7.isReadBeyondEnd());
    assert(inputStream7.getRemainingLength() == b(0));
}

static void testSerialization()
{
    // 1. serialized bytes is cached after serialization
    MemoryOutputStream stream1;
    auto applicationHeader1 = makeShared<ApplicationHeader>();
    auto totalSerializedLength = ChunkSerializer::totalSerializedLength;
    Chunk::serialize(stream1, applicationHeader1);
    auto size = stream1.getLength();
    assert(size != B(0));
    assert(totalSerializedLength + size == ChunkSerializer::totalSerializedLength);
    totalSerializedLength = ChunkSerializer::totalSerializedLength;
    Chunk::serialize(stream1, applicationHeader1);
    assert(stream1.getLength() == size * 2);
    assert(totalSerializedLength == ChunkSerializer::totalSerializedLength);

    // 2. serialized bytes is cached after deserialization
    MemoryInputStream stream2(stream1.getData());
    auto totalDeserializedLength = ChunkSerializer::totalDeserializedLength;
    const auto& chunk1 = Chunk::deserialize(stream2, typeid(ApplicationHeader));
    assert(chunk1 != nullptr);
    assert(B(chunk1->getChunkLength()) == B(size));
    assert(dynamicPtrCast<const ApplicationHeader>(chunk1) != nullptr);
    auto applicationHeader2 = staticPtrCast<ApplicationHeader>(chunk1);
    assert(totalDeserializedLength + size == ChunkSerializer::totalDeserializedLength);
    totalSerializedLength = ChunkSerializer::totalSerializedLength;
    Chunk::serialize(stream1, applicationHeader2);
    assert(stream1.getLength() == size * 3);
    assert(totalSerializedLength == ChunkSerializer::totalSerializedLength);

    // 3. serialized bytes is deleted after change
    applicationHeader1->setSomeData(42);
    totalSerializedLength = ChunkSerializer::totalSerializedLength;
    Chunk::serialize(stream1, applicationHeader1);
    assert(totalSerializedLength + size == ChunkSerializer::totalSerializedLength);
    applicationHeader2->setSomeData(42);
    totalSerializedLength = ChunkSerializer::totalSerializedLength;
    Chunk::serialize(stream1, applicationHeader2);
    assert(totalSerializedLength + size == ChunkSerializer::totalSerializedLength);
}

static void testConversion()
{
    // 1. implicit non-conversion via serialization is an error by default (would unnecessary slow down simulation)
    Packet packet1;
    auto applicationHeader1 = makeImmutableApplicationHeader(42);
    packet1.append(applicationHeader1->Chunk::peek<BytesChunk>(B(0), B(5)));
    packet1.append(applicationHeader1->Chunk::peek(B(5), B(5)));
    ASSERT_ERROR(packet1.peekHeader<ApplicationHeader>(B(10)), "serialization is disabled");

    // 2. implicit conversion via serialization is an error by default (would result in hard to debug errors)
    Packet packet2;
    packet2.append(makeImmutableIpHeader());
    ASSERT_ERROR(packet2.peekHeader<ApplicationHeader>(), "serialization is disabled");
}

static void testIteration()
{
    // 1. packet provides appended chunks
    Packet packet1;
    packet1.append(makeImmutableByteCountChunk(B(10)));
    packet1.append(makeImmutableBytesChunk(makeVector(10)));
    packet1.append(makeImmutableApplicationHeader(42));
    int index1 = 0;
    auto chunk1 = packet1.popHeader();
    while (chunk1 != nullptr) {
        assert(chunk1 != nullptr);
        assert(chunk1->getChunkLength() == B(10));
        index1++;
        chunk1 = packet1.popHeader(b(-1), Chunk::PF_ALLOW_NULLPTR);
    }
    assert(index1 == 3);

    // 2. SequenceChunk optimizes forward iteration to indexing
    auto sequenceChunk1 = makeShared<SequenceChunk>();
    sequenceChunk1->insertAtEnd(makeImmutableByteCountChunk(B(10)));
    sequenceChunk1->insertAtEnd(makeImmutableBytesChunk(makeVector(10)));
    sequenceChunk1->insertAtEnd(makeImmutableApplicationHeader(42));
    sequenceChunk1->markImmutable();
    int index2 = 0;
    auto iterator2 = Chunk::ForwardIterator(b(0), 0);
    auto chunk2 = sequenceChunk1->peek(iterator2);
    assert(dynamicPtrCast<const ByteCountChunk>(chunk2) != nullptr);
    while (chunk2 != nullptr) {
        assert(iterator2.getIndex() == index2);
        assert(iterator2.getPosition() == B(index2 * 10));
        assert(chunk2 != nullptr);
        assert(chunk2->getChunkLength() == B(10));
        index2++;
        if (chunk2 != nullptr)
            sequenceChunk1->moveIterator(iterator2, chunk2->getChunkLength());
        chunk2 = sequenceChunk1->peek(iterator2, b(-1), Chunk::PF_ALLOW_NULLPTR);
    }
    assert(index2 == 3);

    // 3. SequenceChunk optimizes backward iteration to indexing
    auto sequenceChunk2 = makeShared<SequenceChunk>();
    sequenceChunk2->insertAtEnd(makeImmutableByteCountChunk(B(10)));
    sequenceChunk2->insertAtEnd(makeImmutableBytesChunk(makeVector(10)));
    sequenceChunk2->insertAtEnd(makeImmutableApplicationHeader(42));
    sequenceChunk2->markImmutable();
    int index3 = 0;
    auto iterator3 = Chunk::BackwardIterator(b(0), 0);
    auto chunk3 = sequenceChunk1->peek(iterator3);
    assert(dynamicPtrCast<const ApplicationHeader>(chunk3) != nullptr);
    while (chunk3 != nullptr) {
        assert(iterator3.getIndex() == index3);
        assert(iterator3.getPosition() == B(index3 * 10));
        assert(chunk3 != nullptr);
        assert(chunk3->getChunkLength() == B(10));
        index3++;
        if (chunk3 != nullptr)
            sequenceChunk1->moveIterator(iterator3, chunk3->getChunkLength());
        chunk3 = sequenceChunk1->peek(iterator3, b(-1), Chunk::PF_ALLOW_NULLPTR);
    }
    assert(index2 == 3);
}

static void testCorruption()
{
    // 1. data corruption with constant bit error rate
    double random[] = {0.1, 0.7, 0.9};
    double ber = 1E-2;
    Packet packet1;
    const auto& chunk1 = makeImmutableByteCountChunk(B(10));
    const auto& chunk2 = makeImmutableBytesChunk(makeVector(10));
    const auto& chunk3 = makeImmutableApplicationHeader(42);
    packet1.append(chunk1);
    packet1.append(chunk2);
    packet1.append(chunk3);
    int index = 0;
    auto chunk = packet1.popHeader();
    Packet packet2;
    while (chunk != nullptr) {
        const auto& clone = chunk->dupShared();
        b length = chunk->getChunkLength();
        if (random[index++] >= std::pow(1 - ber, length.get()))
            clone->markIncorrect();
        clone->markImmutable();
        packet2.append(clone);
        chunk = packet1.popHeader(b(-1), Chunk::PF_ALLOW_NULLPTR);
    }
    assert(packet2.popHeader(b(-1), Chunk::PF_ALLOW_INCORRECT)->isCorrect());
    assert(packet2.popHeader(b(-1), Chunk::PF_ALLOW_INCORRECT)->isIncorrect());
    assert(packet2.popHeader(b(-1), Chunk::PF_ALLOW_INCORRECT)->isIncorrect());
}

static void testDuplication()
{
    // 1. copy of immutable packet shares chunk
    Packet packet1;
    const Ptr<ByteCountChunk> byteCountChunk1 = makeImmutableByteCountChunk(B(10));
    packet1.append(byteCountChunk1);
    auto packet2 = packet1.dup();
    assert(packet2->getTotalLength() == B(10));
    assert(byteCountChunk1.use_count() == 3); // 1 here + 2 in the packets
    delete packet2;
}

static void testDuality()
{
    // 1. packet provides header in both fields and bytes representation
    Packet packet1;
    packet1.append(makeImmutableApplicationHeader(42));
    const auto& applicationHeader1 = packet1.peekHeader<ApplicationHeader>();
    const auto& bytesChunk1 = packet1.peekHeader<BytesChunk>(B(10));
    assert(applicationHeader1 != nullptr);
    assert(applicationHeader1->getChunkLength() == B(10));
    assert(bytesChunk1 != nullptr);
    assert(bytesChunk1->getChunkLength() == B(10));

    // 2. packet provides header in both fields and bytes representation after serialization
    Packet packet2(nullptr, packet1.peekAllBytes());
    const auto& applicationHeader2 = packet2.peekHeader<ApplicationHeader>();
    const auto& bytesChunk2 = packet2.peekHeader<BytesChunk>(B(10));
    assert(applicationHeader2 != nullptr);
    assert(applicationHeader2->getChunkLength() == B(10));
    assert(bytesChunk2 != nullptr);
    assert(bytesChunk2->getChunkLength() == B(10));
    assert(std::equal(bytesChunk1->getBytes().begin(), bytesChunk1->getBytes().end(), bytesChunk1->getBytes().begin()));
    assert(applicationHeader2->getSomeData() == applicationHeader2->getSomeData());
}

static void testMerging()
{
    // 1. packet provides complete merged header if the whole header is available
    Packet packet1;
    packet1.append(makeImmutableApplicationHeader(42));
    Packet packet2;
    packet2.append(packet1.peekAt(B(0), B(5)));
    packet2.append(packet1.peekAt(B(5), B(5)));
    const auto& chunk1 = packet2.peekHeader();
    assert(chunk1 != nullptr);
    assert(chunk1->isComplete());
    assert(chunk1->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ApplicationHeader>(chunk1) != nullptr);
    const auto& chunk2 = packet2.peekHeader<ApplicationHeader>();
    assert(chunk2->isComplete());
    assert(chunk2->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ApplicationHeader>(chunk2) != nullptr);

    // 2. packet provides compacts ByteCountChunks
    Packet packet3;
    packet3.append(makeImmutableByteCountChunk(B(5)));
    packet3.append(makeImmutableByteCountChunk(B(5)));
    const auto& chunk3 = packet3.peekAt(b(0), packet3.getTotalLength());
    const auto& chunk4 = packet3.peekAt<ByteCountChunk>(b(0), packet3.getTotalLength());
    assert(chunk3 != nullptr);
    assert(chunk3->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk3) != nullptr);
    assert(chunk4 != nullptr);
    assert(chunk4->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk4) != nullptr);

    // 2. packet provides compacts ByteChunks
    Packet packet4;
    packet4.append(makeImmutableBytesChunk(makeVector(5)));
    packet4.append(makeImmutableBytesChunk(makeVector(5)));
    const auto& chunk5 = packet4.peekAt(b(0), packet4.getTotalLength());
    const auto& chunk6 = packet4.peekAllBytes();
    assert(chunk5 != nullptr);
    assert(chunk5->getChunkLength() == B(10));
    assert(dynamicPtrCast<const BytesChunk>(chunk5) != nullptr);
    const auto& bytesChunk1 = staticPtrCast<const BytesChunk>(chunk5);
    assert(std::equal(bytesChunk1->getBytes().begin(), bytesChunk1->getBytes().end(), std::vector<uint8_t>({0, 1, 2, 3, 4, 0, 1, 2, 3, 4}).begin()));
    assert(chunk6 != nullptr);
    assert(chunk6->getChunkLength() == B(10));
    assert(dynamicPtrCast<const BytesChunk>(chunk6) != nullptr);
    const auto& bytesChunk2 = staticPtrCast<const BytesChunk>(chunk6);
    assert(std::equal(bytesChunk2->getBytes().begin(), bytesChunk2->getBytes().end(), std::vector<uint8_t>({0, 1, 2, 3, 4, 0, 1, 2, 3, 4}).begin()));
}

static void testSlicing()
{
    // 1. ByteCountChunk always returns ByteCountChunk
    auto byteCountChunk1 = makeImmutableByteCountChunk(B(10));
    const auto& chunk1 = byteCountChunk1->peek(B(0), B(10));
    const auto& chunk2 = byteCountChunk1->peek(B(0), B(5));
    assert(chunk1 == byteCountChunk1);
    assert(dynamicPtrCast<const ByteCountChunk>(chunk1) != nullptr);
    assert(chunk2 != nullptr);
    assert(chunk2->getChunkLength() == B(5));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk2) != nullptr);

    // 2. BytesChunk always returns BytesChunk
    auto bytesChunk1 = makeImmutableBytesChunk(makeVector(10));
    const auto& chunk3 = bytesChunk1->peek(B(0), B(10));
    const auto& chunk4 = bytesChunk1->peek(B(0), B(5));
    assert(chunk3 != nullptr);
    assert(chunk3->getChunkLength() == B(10));
    assert(dynamicPtrCast<const BytesChunk>(chunk3) != nullptr);
    const auto& bytesChunk2 = staticPtrCast<const BytesChunk>(chunk3);
    assert(std::equal(bytesChunk2->getBytes().begin(), bytesChunk2->getBytes().end(), makeVector(10).begin()));
    assert(chunk4 != nullptr);
    assert(chunk4->getChunkLength() == B(5));
    assert(dynamicPtrCast<const BytesChunk>(chunk4) != nullptr);
    const auto& bytesChunk3 = staticPtrCast<const BytesChunk>(chunk4);
    assert(std::equal(bytesChunk3->getBytes().begin(), bytesChunk3->getBytes().end(), makeVector(5).begin()));

    // 3a. SliceChunk returns a SliceChunk containing the requested slice of the chunk that is used by the original SliceChunk
    auto applicationHeader1 = makeImmutableApplicationHeader(42);
    auto sliceChunk1 = makeShared<SliceChunk>(applicationHeader1, b(0), B(10));
    sliceChunk1->markImmutable();
    const auto& chunk5 = sliceChunk1->peek(B(5), B(5));
    assert(chunk5 != nullptr);
    assert(chunk5->getChunkLength() == B(5));
    assert(dynamicPtrCast<const SliceChunk>(chunk5) != nullptr);
    auto sliceChunk2 = staticPtrCast<const SliceChunk>(chunk5);
    assert(sliceChunk2->getChunk() == sliceChunk1->getChunk());
    assert(sliceChunk2->getOffset() == B(5));
    assert(sliceChunk2->getLength() == B(5));

    // 4a. SequenceChunk may return an element chunk
    auto sequenceChunk1 = makeShared<SequenceChunk>();
    sequenceChunk1->insertAtEnd(byteCountChunk1);
    sequenceChunk1->insertAtEnd(bytesChunk1);
    sequenceChunk1->insertAtEnd(applicationHeader1);
    sequenceChunk1->markImmutable();
    const auto& chunk6 = sequenceChunk1->peek(B(0), B(10));
    const auto& chunk7 = sequenceChunk1->peek(B(10), B(10));
    const auto& chunk8 = sequenceChunk1->peek(B(20), B(10));
    assert(chunk6 != nullptr);
    assert(chunk6->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk6) != nullptr);
    assert(chunk7 != nullptr);
    assert(chunk7->getChunkLength() == B(10));
    assert(dynamicPtrCast<const BytesChunk>(chunk7) != nullptr);
    assert(chunk8 != nullptr);
    assert(chunk8->getChunkLength() == B(10));
    assert(dynamicPtrCast<const ApplicationHeader>(chunk8) != nullptr);

    // 4b. SequenceChunk may return a (simplified) SliceChunk of an element chunk
    const auto& chunk9 = sequenceChunk1->peek(B(0), B(5));
    const auto& chunk10 = sequenceChunk1->peek(B(15), B(5));
    const auto& chunk11 = sequenceChunk1->peek(B(20), B(5));
    assert(chunk9 != nullptr);
    assert(chunk9->getChunkLength() == B(5));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk9) != nullptr);
    assert(chunk10 != nullptr);
    assert(chunk10->getChunkLength() == B(5));
    assert(dynamicPtrCast<const BytesChunk>(chunk10) != nullptr);
    assert(chunk11 != nullptr);
    assert(chunk11->getChunkLength() == B(5));
    assert(dynamicPtrCast<const SliceChunk>(chunk11) != nullptr);

    // 4c. SequenceChunk may return a new SequenceChunk
    const auto& chunk12 = sequenceChunk1->peek(B(5), B(10));
    assert(chunk12 != nullptr);
    assert(chunk12->getChunkLength() == B(10));
    assert(dynamicPtrCast<const SequenceChunk>(chunk12) != nullptr);
    const auto& sequenceChunk2 = staticPtrCast<const SequenceChunk>(chunk12);
    assert(sequenceChunk1 != sequenceChunk2);
    assert(sequenceChunk2->getChunks().size() == 2);

    // 5. any other chunk returns a SliceChunk
    auto applicationHeader2 = makeImmutableApplicationHeader(42);
    const auto& chunk13 = applicationHeader2->peek(B(0), B(5));
    assert(chunk13 != nullptr);
    assert(chunk13->getChunkLength() == B(5));
    assert(dynamicPtrCast<const SliceChunk>(chunk13) != nullptr);
    const auto& sliceChunk4 = dynamicPtrCast<const SliceChunk>(chunk13);
    assert(sliceChunk4->getChunk() == applicationHeader2);
    assert(sliceChunk4->getOffset() == b(0));
    assert(sliceChunk4->getLength() == B(5));
}

static void testNesting()
{
    // 1. packet contains compound header as is
    Packet packet1;
    auto ipHeader1 = makeShared<IpHeader>();
    ipHeader1->setProtocol(Protocol::Tcp);
    auto compoundHeader1 = makeShared<CompoundHeader>();
    compoundHeader1->insertAtEnd(ipHeader1);
    compoundHeader1->markImmutable();
    packet1.append(compoundHeader1);
    const auto& compoundHeader2 = packet1.peekHeader<CompoundHeader>(compoundHeader1->getChunkLength());
    assert(compoundHeader2 != nullptr);

    // 2. packet provides compound header after serialization
    Packet packet2(nullptr, packet1.peekAllBytes());
    const auto& compoundHeader3 = packet2.peekHeader<CompoundHeader>();
    assert(compoundHeader3 != nullptr);
    auto it = Chunk::ForwardIterator(b(0), 0);
    const auto& ipHeader2 = compoundHeader3->Chunk::peek<IpHeader>(it);
    assert(ipHeader2 != nullptr);
    assert(ipHeader2->getProtocol() == Protocol::Tcp);
}

static void testPeeking()
{
    // 1. packet provides ByteCountChunks by default if it contains a ByteCountChunk only
    Packet packet1;
    packet1.append(makeImmutableByteCountChunk(B(10)));
    packet1.append(makeImmutableByteCountChunk(B(10)));
    packet1.append(makeImmutableByteCountChunk(B(10)));
    const auto& chunk1 = packet1.popHeader(B(15));
    const auto& chunk2 = packet1.popHeader(B(15));
    assert(chunk1 != nullptr);
    assert(chunk1->getChunkLength() == B(15));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk1) != nullptr);
    assert(chunk2 != nullptr);
    assert(chunk2->getChunkLength() == B(15));
    assert(dynamicPtrCast<const ByteCountChunk>(chunk2) != nullptr);

    // 2. packet provides BytesChunks by default if it contains a BytesChunk only
    Packet packet2;
    packet2.append(makeImmutableBytesChunk(makeVector(10)));
    packet2.append(makeImmutableBytesChunk(makeVector(10)));
    packet2.append(makeImmutableBytesChunk(makeVector(10)));
    const auto& chunk3 = packet2.popHeader(B(15));
    const auto& chunk4 = packet2.popHeader(B(15));
    assert(chunk3 != nullptr);
    assert(chunk3->getChunkLength() == B(15));
    assert(dynamicPtrCast<const BytesChunk>(chunk3) != nullptr);
    assert(chunk4 != nullptr);
    assert(chunk4->getChunkLength() == B(15));
    assert(dynamicPtrCast<const BytesChunk>(chunk4) != nullptr);
}

static void testSequence()
{
    // 1. sequence merges immutable slices
    auto applicationHeader1 = makeImmutableApplicationHeader(42);
    auto sequenceChunk1 = makeShared<SequenceChunk>();
    sequenceChunk1->insertAtEnd(applicationHeader1->peek(B(0), B(5)));
    sequenceChunk1->insertAtEnd(applicationHeader1->peek(B(5), B(5)));
    const auto& chunk1 = sequenceChunk1->peek(b(0));
    assert(dynamicPtrCast<const ApplicationHeader>(chunk1) != nullptr);

    // 2. sequence merges mutable slices
    auto sequenceChunk2 = makeShared<SequenceChunk>();
    sequenceChunk2->insertAtEnd(makeShared<SliceChunk>(applicationHeader1, B(0), B(5)));
    sequenceChunk2->insertAtEnd(makeShared<SliceChunk>(applicationHeader1, B(5), B(5)));
    const auto& chunk2 = sequenceChunk2->peek(b(0));
    assert(dynamicPtrCast<const ApplicationHeader>(chunk2) != nullptr);
}

static void testChunkQueue()
{
    // 1. queue provides ByteCountChunks by default if it contains a ByteCountChunk only
    ChunkQueue queue1;
    auto byteCountChunk1 = makeImmutableByteCountChunk(B(10));
    queue1.push(byteCountChunk1);
    queue1.push(byteCountChunk1);
    queue1.push(byteCountChunk1);
    const auto& byteCountChunk2 = dynamicPtrCast<const ByteCountChunk>(queue1.pop(B(15)));
    const auto& byteCountChunk3 = dynamicPtrCast<const ByteCountChunk>(queue1.pop(B(15)));
    assert(byteCountChunk2 != nullptr);
    assert(byteCountChunk3 != nullptr);

    // 2. queue provides BytesChunks by default if it contains a BytesChunk only
    ChunkQueue queue2;
    auto bytesChunk1 = makeImmutableBytesChunk(makeVector(10));
    queue2.push(bytesChunk1);
    queue2.push(bytesChunk1);
    queue2.push(bytesChunk1);
    const auto& bytesChunk2 = dynamicPtrCast<const BytesChunk>(queue2.pop(B(15)));
    const auto& bytesChunk3 = dynamicPtrCast<const BytesChunk>(queue2.pop(B(15)));
    assert(bytesChunk2 != nullptr);
    assert(bytesChunk3 != nullptr);

    // 3. queue provides reassembled header
    ChunkQueue queue3;
    auto applicationHeader1 = makeImmutableApplicationHeader(42);
    queue3.push(applicationHeader1->peek(B(0), B(5)));
    queue3.push(applicationHeader1->peek(B(5), B(5)));
    assert(queue3.has<ApplicationHeader>());
    const auto& applicationHeader2 = queue3.pop<ApplicationHeader>();
    assert(applicationHeader2 != nullptr);
    assert(applicationHeader2->getSomeData() == 42);
}

static void testChunkBuffer()
{
    // 1. single chunk
    ChunkBuffer buffer1;
    auto byteCountChunk1 = makeImmutableByteCountChunk(B(10));
    buffer1.replace(b(0), byteCountChunk1);
    assert(buffer1.getNumRegions() == 1);
    assert(buffer1.getRegionData(0) != nullptr);

    // 2. consecutive chunks
    ChunkBuffer buffer2;
    buffer2.replace(B(0), byteCountChunk1);
    buffer2.replace(B(10), byteCountChunk1);
    const auto& byteCountChunk2 = dynamicPtrCast<const ByteCountChunk>(buffer2.getRegionData(0));
    assert(buffer2.getNumRegions() == 1);
    assert(byteCountChunk2 != nullptr);
    assert(byteCountChunk2->getChunkLength() == B(20));

    // 3. consecutive slice chunks
    ChunkBuffer buffer3;
    auto applicationHeader1 = makeImmutableApplicationHeader(42);
    buffer3.replace(B(0), applicationHeader1->peek(B(0), B(5)));
    buffer3.replace(B(5), applicationHeader1->peek(B(5), B(5)));
    const auto& applicationHeader2 = dynamicPtrCast<const ApplicationHeader>(buffer3.getRegionData(0));
    assert(buffer3.getNumRegions() == 1);
    assert(applicationHeader2 != nullptr);
    assert(applicationHeader2->getSomeData() == 42);

    // 4. out of order consecutive chunks
    ChunkBuffer buffer4;
    buffer4.replace(B(0), byteCountChunk1);
    buffer4.replace(B(20), byteCountChunk1);
    buffer4.replace(B(10), byteCountChunk1);
    const auto& byteCountChunk3 = dynamicPtrCast<const ByteCountChunk>(buffer4.getRegionData(0));
    assert(buffer4.getNumRegions() == 1);
    assert(byteCountChunk3 != nullptr);
    assert(byteCountChunk3->getChunkLength() == B(30));

    // 5. out of order consecutive slice chunks
    ChunkBuffer buffer5;
    buffer5.replace(B(0), applicationHeader1->peek(B(0), B(3)));
    buffer5.replace(B(7), applicationHeader1->peek(B(7), B(3)));
    buffer5.replace(B(3), applicationHeader1->peek(B(3), B(4)));
    const auto& applicationHeader3 = dynamicPtrCast<const ApplicationHeader>(buffer5.getRegionData(0));
    assert(buffer5.getNumRegions() == 1);
    assert(applicationHeader3 != nullptr);
    assert(applicationHeader3->getSomeData() == 42);

    // 6. heterogeneous chunks
    ChunkBuffer buffer6;
    auto byteArrayChunk1 = makeImmutableBytesChunk(makeVector(10));
    buffer6.replace(B(0), byteCountChunk1);
    buffer6.replace(B(10), byteArrayChunk1);
    assert(buffer6.getNumRegions() == 1);
    assert(buffer6.getRegionData(0) != nullptr);

    // 7. completely overwriting a chunk
    ChunkBuffer buffer7;
    auto byteCountChunk4 = makeImmutableByteCountChunk(B(8));
    buffer7.replace(B(1), byteCountChunk4);
    buffer7.replace(B(0), byteArrayChunk1);
    const auto& bytesChunk1 = dynamicPtrCast<const BytesChunk>(buffer7.getRegionData(0));
    assert(buffer7.getNumRegions() == 1);
    assert(bytesChunk1 != nullptr);

    // 8. partially overwriting multiple chunks
    ChunkBuffer buffer8;
    buffer8.replace(B(0), byteCountChunk1);
    buffer8.replace(B(10), byteCountChunk1);
    buffer8.replace(B(3), byteArrayChunk1);
    assert(buffer8.getNumRegions() == 1);
    const auto& sequenceChunk1 = dynamicPtrCast<const SequenceChunk>(buffer8.getRegionData(0));
    assert(sequenceChunk1 != nullptr);
    const auto& byteCountChunk5 = dynamicPtrCast<const ByteCountChunk>(sequenceChunk1->peek(B(0), B(3)));
    assert(byteCountChunk5 != nullptr);
    assert(byteCountChunk5->getChunkLength() == B(3));
    const auto& byteCountChunk6 = dynamicPtrCast<const ByteCountChunk>(sequenceChunk1->peek(B(13), B(7)));
    assert(byteCountChunk6 != nullptr);
    assert(byteCountChunk6->getChunkLength() == B(7));
    const auto& bytesChunk2 = dynamicPtrCast<const BytesChunk>(sequenceChunk1->peek(B(3), B(10)));
    assert(bytesChunk2 != nullptr);
    assert(std::equal(bytesChunk2->getBytes().begin(), bytesChunk2->getBytes().end(), makeVector(10).begin()));

    // 9. random test
    bool debug = false;
    cLCG32 random;
    B bufferSize = B(1000);
    B maxChunkLength = B(100);
    ChunkBuffer buffer9;
    int *buffer10 = new int[bufferSize.get()];
    memset(buffer10, -1, bufferSize.get() * sizeof(int));
    for (int c = 0; c < 1000; c++) {
        // replace data
        B chunkOffset = B(random.intRand((bufferSize - maxChunkLength).get()));
        B chunkLength = B(random.intRand(maxChunkLength.get()) + 1);
        auto chunk = makeShared<BytesChunk>();
        std::vector<uint8_t> bytes;
        for (B i = B(0); i < chunkLength; i++)
            bytes.push_back(i.get() & 0xFF);
        chunk->setBytes(bytes);
        chunk->markImmutable();
        if (debug)
            std::cout << "Replace " << c << ": offset = " << chunkOffset << ", chunk = " << chunk << std::endl;
        buffer9.replace(B(chunkOffset), chunk);
        for (B i = B(0); i < chunkLength; i++)
            *(buffer10 + chunkOffset.get() + i.get()) = i.get() & 0xFF;

        // clear data
        chunkOffset = B(random.intRand((bufferSize - maxChunkLength).get()));
        chunkLength = B(random.intRand(maxChunkLength.get()) + 1);
        buffer9.clear(B(chunkOffset), chunkLength);
        for (B i = B(0); i < chunkLength; i++)
            *(buffer10 + chunkOffset.get() + i.get()) = -1;

        // compare data
        if (debug) {
            std::cout << "ChunkBuffer: " << buffer9 << std::endl;
            std::cout << "PlainBuffer: ";
            for (B i = B(0); i < bufferSize; i++)
                printf("%d", *(buffer10 + i.get()));
            std::cout << std::endl << std::endl;
        }
        B previousEndOffset = B(0);
        for (int i = 0; i < buffer9.getNumRegions(); i++) {
            auto data = dynamicPtrCast<const BytesChunk>(buffer9.getRegionData(i));
            auto startOffset = B(buffer9.getRegionStartOffset(i));
            for (B j = previousEndOffset; j < startOffset; j++)
                assert(*(buffer10 + j.get()) == -1);
            for (B j = B(0); j < B(data->getChunkLength()); j++)
                assert(data->getByte(j.get()) == *(buffer10 + startOffset.get() + j.get()));
            previousEndOffset = startOffset + data->getChunkLength();
        }
        for (B j = previousEndOffset; j < bufferSize; j++)
            assert(*(buffer10 + j.get()) == -1);
    }
    delete [] buffer10;
}

static void testReassemblyBuffer()
{
    // 1. single chunk
    ReassemblyBuffer buffer1(B(10));
    auto byteCountChunk1 = makeImmutableByteCountChunk(B(10));
    buffer1.replace(b(0), byteCountChunk1);
    assert(buffer1.isComplete());
    const auto& data1 = buffer1.getReassembledData();
    assert(data1 != nullptr);
    assert(dynamicPtrCast<const ByteCountChunk>(data1) != nullptr);
    assert(data1->getChunkLength() == B(10));

    // 2. consecutive chunks
    ReassemblyBuffer buffer2(B(20));
    buffer2.replace(b(0), byteCountChunk1);
    assert(!buffer2.isComplete());
    buffer2.replace(B(10), byteCountChunk1);
    assert(buffer2.isComplete());
    const auto& data2 = buffer2.getReassembledData();
    assert(data2 != nullptr);
    assert(dynamicPtrCast<const ByteCountChunk>(data2) != nullptr);
    assert(data2->getChunkLength() == B(20));

    // 3. out of order consecutive chunks
    ReassemblyBuffer buffer3(B(30));
    buffer3.replace(b(0), byteCountChunk1);
    assert(!buffer3.isComplete());
    buffer3.replace(B(20), byteCountChunk1);
    assert(!buffer3.isComplete());
    buffer3.replace(B(10), byteCountChunk1);
    assert(buffer3.isComplete());
    const auto& data3 = buffer3.getReassembledData();
    assert(data3 != nullptr);
    assert(dynamicPtrCast<const ByteCountChunk>(data3) != nullptr);
    assert(data3->getChunkLength() == B(30));
}

static void testReorderBuffer()
{
    // 1. single chunk
    ReorderBuffer buffer1(B(1000));
    auto byteCountChunk1 = makeImmutableByteCountChunk(B(10));
    buffer1.replace(B(1000), byteCountChunk1);
    const auto& data1 = buffer1.popAvailableData();
    assert(data1 != nullptr);
    assert(dynamicPtrCast<const ByteCountChunk>(data1) != nullptr);
    assert(data1->getChunkLength() == B(10));
    assert(buffer1.getExpectedOffset() == B(1010));

    // 2. consecutive chunks
    ReorderBuffer buffer2(B(1000));
    buffer2.replace(B(1000), byteCountChunk1);
    buffer2.replace(B(1010), byteCountChunk1);
    const auto& data2 = buffer2.popAvailableData();
    assert(data2 != nullptr);
    assert(dynamicPtrCast<const ByteCountChunk>(data2) != nullptr);
    assert(data2->getChunkLength() == B(20));
    assert(buffer2.getExpectedOffset() == B(1020));

    // 3. out of order consecutive chunks
    ReorderBuffer buffer3(B(1000));
    buffer3.replace(B(1020), byteCountChunk1);
    assert(buffer2.popAvailableData() == nullptr);
    buffer3.replace(B(1000), byteCountChunk1);
    buffer3.replace(B(1010), byteCountChunk1);
    const auto& data3 = buffer3.popAvailableData();
    assert(data3 != nullptr);
    assert(dynamicPtrCast<const ByteCountChunk>(data3) != nullptr);
    assert(data3->getChunkLength() == B(30));
    assert(buffer3.getExpectedOffset() == B(1030));
}

void UnitTest::initialize()
{
    testMutable();
    testImmutable();
    testComplete();
    testIncomplete();
    testCorrect();
    testIncorrect();
    testProperlyRepresented();
    testImproperlyRepresented();
    testEmpty();
    testHeader();
    testTrailer();
    testHeaderPopOffset();
    testTrailerPopOffset();
    testEncapsulation();
    testAggregation();
    testFragmentation();
    testPolymorphism();
    testStreaming();
    testSerialization();
    testConversion();
    testIteration();
    testCorruption();
    testDuplication();
    testDuality();
    testMerging();
    testSlicing();
    testNesting();
    testPeeking();
    testSequence();
    testChunkQueue();
    testChunkBuffer();
    testReassemblyBuffer();
    testReorderBuffer();
}

} // namespace
