// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "bucketmessages.h"

namespace storage {

ReadBucketList::ReadBucketList(spi::PartitionId partition)
    : api::InternalCommand(ID), _partition(partition)
{ }

ReadBucketList::~ReadBucketList() { }

void
ReadBucketList::print(std::ostream& out, bool verbose, const std::string& indent) const {
    out << "ReadBucketList(" << _partition << ")";

    if (verbose) {
        out << " : ";
        InternalCommand::print(out, true, indent);
    }
}

ReadBucketListReply::ReadBucketListReply(const ReadBucketList& cmd)
    : api::InternalReply(ID, cmd),
      _partition(cmd.getPartition())
{ }

ReadBucketListReply::~ReadBucketListReply() { }

void
ReadBucketListReply::print(std::ostream& out, bool verbose, const std::string& indent) const
{
    out << "ReadBucketListReply(" << _buckets.size() << " buckets)";
    if (verbose) {
        out << " : ";
        InternalReply::print(out, true, indent);
    }
}

std::unique_ptr<api::StorageReply>
ReadBucketList::makeReply() {
    return std::make_unique<ReadBucketListReply>(*this);
}

ReadBucketInfo::ReadBucketInfo(const document::BucketId& bucketId)
    : api::InternalCommand(ID), _bucketId(bucketId)
{ }

ReadBucketInfo::~ReadBucketInfo() { }

void
ReadBucketInfo::print(std::ostream& out, bool verbose, const std::string& indent) const
{
    out << "ReadBucketInfo(" << _bucketId << ")";

    if (verbose) {
        out << " : ";
        InternalCommand::print(out, true, indent);
    }
}

vespalib::string
ReadBucketInfo::getSummary() const {
    vespalib::string s("ReadBucketInfo(");
    s.append(_bucketId.toString());
    s.append(')');
    return s;
}

ReadBucketInfoReply::ReadBucketInfoReply(const ReadBucketInfo& cmd)
    : api::InternalReply(ID, cmd),
     _bucketId(cmd.getBucketId())
{ }

ReadBucketInfoReply::~ReadBucketInfoReply() { }
void
ReadBucketInfoReply::print(std::ostream& out, bool verbose, const std::string& indent) const {
    out << "ReadBucketInfoReply()";
    if (verbose) {
        out << " : ";
        InternalReply::print(out, true, indent);
    }
}

std::unique_ptr<api::StorageReply> ReadBucketInfo::makeReply() {
    return std::make_unique<ReadBucketInfoReply>(*this);
}


RepairBucketCommand::RepairBucketCommand(const document::BucketId& bucket, uint16_t disk)
    : api::InternalCommand(ID),
      _bucket(bucket),
      _disk(disk),
      _verifyBody(false),
      _moveToIdealDisk(false)
{
    setPriority(LOW);
}

RepairBucketCommand::~RepairBucketCommand() { }

void
RepairBucketCommand::print(std::ostream& out, bool verbose, const std::string& indent) const {
    out << getSummary();
    if (verbose) {
        out << " : ";
        InternalCommand::print(out, true, indent);
    }
}
    
vespalib::string
RepairBucketCommand::getSummary() const {
    vespalib::asciistream s;
    s << "ReadBucketInfo(" << _bucket.toString() << ", disk " << _disk
      << (_verifyBody ? ", verifying body" : "")
      << (_moveToIdealDisk ? ", moving to ideal disk" : "")
      << ")";
    return s.str();
}

RepairBucketReply::RepairBucketReply(const RepairBucketCommand& cmd, const api::BucketInfo& bucketInfo)
    : api::InternalReply(ID, cmd),
      _bucket(cmd.getBucketId()),
      _bucketInfo(bucketInfo),
      _disk(cmd.getDisk()),
      _altered(false)
{ }

RepairBucketReply::~RepairBucketReply() { }

void
RepairBucketReply::print(std::ostream& out, bool verbose, const std::string& indent) const {
    out << "RepairBucketReply()";

    if (verbose) {
        out << " : ";
        InternalReply::print(out, true, indent);
    }
}

std::unique_ptr<api::StorageReply>
RepairBucketCommand::makeReply() {
    return std::make_unique<RepairBucketReply>(*this);
}

BucketDiskMoveCommand::BucketDiskMoveCommand(const document::BucketId& bucket,
                                             uint16_t srcDisk, uint16_t dstDisk)
    : api::InternalCommand(ID),
      _bucket(bucket),
      _srcDisk(srcDisk),
      _dstDisk(dstDisk)
{
    setPriority(LOW);
}

BucketDiskMoveCommand::~BucketDiskMoveCommand() { }

void
BucketDiskMoveCommand::print(std::ostream& out, bool, const std::string&) const {
    out << "BucketDiskMoveCommand(" << _bucket << ", source " << _srcDisk
        << ", target " << _dstDisk << ")";
}

BucketDiskMoveReply::BucketDiskMoveReply(const BucketDiskMoveCommand& cmd,
                                        const api::BucketInfo& bucketInfo,
                                        uint32_t sourceFileSize,
                                        uint32_t destinationFileSize)
    : api::InternalReply(ID, cmd),
      _bucket(cmd.getBucketId()),
      _bucketInfo(bucketInfo),
      _fileSizeOnSrc(sourceFileSize),
      _fileSizeOnDst(destinationFileSize),
      _srcDisk(cmd.getSrcDisk()),
      _dstDisk(cmd.getDstDisk())
{ }

BucketDiskMoveReply::~BucketDiskMoveReply() { }

void
BucketDiskMoveReply::print(std::ostream& out, bool, const std::string&) const
{
    out << "BucketDiskMoveReply(" << _bucket << ", source " << _srcDisk
        << ", target " << _dstDisk << ", " << _bucketInfo << ", "
        << getResult() << ")";
}

std::unique_ptr<api::StorageReply>
BucketDiskMoveCommand::makeReply()
{
    return std::make_unique<BucketDiskMoveReply>(*this);
}


InternalBucketJoinCommand::InternalBucketJoinCommand(const document::BucketId& bucket,
                                                     uint16_t keepOnDisk, uint16_t joinFromDisk)
    : api::InternalCommand(ID),
      _bucket(bucket),
      _keepOnDisk(keepOnDisk),
      _joinFromDisk(joinFromDisk)
{
    setPriority(HIGH); // To not get too many pending of these, prioritize
                       // them higher than getting more bucket info lists.
}

InternalBucketJoinCommand::~InternalBucketJoinCommand() { }

void
InternalBucketJoinCommand::print(std::ostream& out, bool verbose, const std::string& indent) const {
    out << "InternalBucketJoinCommand()";

    if (verbose) {
        out << " : ";
        InternalCommand::print(out, true, indent);
    }
}

InternalBucketJoinReply::InternalBucketJoinReply(const InternalBucketJoinCommand& cmd,
                                                 const api::BucketInfo& info)
    : api::InternalReply(ID, cmd),
      _bucket(cmd.getBucketId()),
      _bucketInfo(info)
{ }

InternalBucketJoinReply::~InternalBucketJoinReply() { }

void
InternalBucketJoinReply::print(std::ostream& out, bool verbose, const std::string& indent) const
{
    out << "InternalBucketJoinReply()";

    if (verbose) {
        out << " : ";
        InternalReply::print(out, true, indent);
    }
}

std::unique_ptr<api::StorageReply>
InternalBucketJoinCommand::makeReply()
{
    return std::make_unique<InternalBucketJoinReply>(*this);
}

} // storage
