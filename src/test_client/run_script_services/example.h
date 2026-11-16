// AUTO-GENERATED. DO NOT EDIT.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace example {

class File;
class IFile;
class IRunScriptGlobalFunctions;

class IFile {
public:
    virtual ~IFile() = default;
    virtual int64_t GetSize() = 0;
    virtual std::vector<uint8_t> ReadBytes(int64_t readBytes) = 0;

    uint64_t this_id = 0;
};

class IRunScriptGlobalFunctions {
public:
    virtual ~IRunScriptGlobalFunctions() = default;
    virtual std::shared_ptr<IFile> GetFile(std::string filepath) = 0;
    virtual int64_t GetTimeNow() = 0;
    virtual bool HasFile(std::string filepath) = 0;
    virtual std::vector<std::string> GetFileList(std::string filepath) = 0;
};

struct GetFileRequest {
    std::string filepath;
    std::vector<uint8_t> encode() const;
    static GetFileRequest decode(const std::vector<uint8_t>&);
};

struct GetTimeNowRequest {
    std::vector<uint8_t> encode() const;
    static GetTimeNowRequest decode(const std::vector<uint8_t>&);
};

struct GetTimeNowResponse {
    int64_t value;
    std::vector<uint8_t> encode() const;
    static GetTimeNowResponse decode(const std::vector<uint8_t>&);
};

struct HasFileRequest {
    std::string filepath;
    std::vector<uint8_t> encode() const;
    static HasFileRequest decode(const std::vector<uint8_t>&);
};

struct HasFileResponse {
    bool value;
    std::vector<uint8_t> encode() const;
    static HasFileResponse decode(const std::vector<uint8_t>&);
};

struct GetFileListRequest {
    std::string filepath;
    std::vector<uint8_t> encode() const;
    static GetFileListRequest decode(const std::vector<uint8_t>&);
};

struct GetFileListResponse {
    std::vector<std::string> value;
    std::vector<uint8_t> encode() const;
    static GetFileListResponse decode(const std::vector<uint8_t>&);
};

struct GetSizeRequest {
    uint64_t this_id = 0;
    std::vector<uint8_t> encode() const;
    static GetSizeRequest decode(const std::vector<uint8_t>&);
};

struct GetSizeResponse {
    int64_t value;
    std::vector<uint8_t> encode() const;
    static GetSizeResponse decode(const std::vector<uint8_t>&);
};

struct ReadBytesRequest {
    uint64_t this_id = 0;
    int64_t readBytes;
    std::vector<uint8_t> encode() const;
    static ReadBytesRequest decode(const std::vector<uint8_t>&);
};

struct ReadBytesResponse {
    std::vector<uint8_t> value;
    std::vector<uint8_t> encode() const;
    static ReadBytesResponse decode(const std::vector<uint8_t>&);
};

} // namespace example