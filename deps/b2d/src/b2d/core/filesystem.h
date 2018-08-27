// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_FILESYSTEM_H
#define _B2D_CORE_FILESYSTEM_H

// [Dependencies]
#include "../core/buffer.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::File]
// ============================================================================

//! A thin wrapper around a native OS file support.
class File {
public:
  B2D_NONCOPYABLE(File)

  #if B2D_OS_WINDOWS
  typedef HANDLE HandleType;
  static constexpr HandleType invalidHandle() noexcept { return (HandleType)0; }
  #endif

  #if B2D_OS_POSIX
  typedef int HandleType;
  static constexpr HandleType invalidHandle() noexcept { return -1; }
  #endif

  enum OpenFlags : uint32_t {
    kOpenRead             = 0x00000001u, //!< Open file for reading (O_RDONLY).
    kOpenWrite            = 0x00000002u, //!< Open file for writing (O_WRONLY).
    kOpenRW               = 0x00000003u, //!< Open file for reading & writing (O_RDWR).
    kOpenCreate           = 0x00000004u, //!< Create the file if it doesn't exist (O_CREAT).
    kOpenCreateOnly       = 0x00000008u, //!< Always create the file, fail if it already exists (O_EXCL).
    kOpenTruncate         = 0x00000020u, //!< Truncate the file (O_TRUNC).

    kOpenShareRead        = 0x20000000u, //!< Enables FILE_SHARE_READ   option.
    kOpenShareWrite       = 0x40000000u, //!< Enables FILE_SHARE_WRITE  option.
    kOpenShareRW          = 0x60000000u, //!< Enables both FILE_SHARE_READ and FILE_SHARE_WRITE options.
    kOpenShareDelete      = 0x80000000u  //!< Enables FILE_SHARE_DELETE option.
  };

  enum SeekType : uint32_t {
    kSeekSet              = 0,           //!< Seek from the beginning of the file (SEEK_SET).
    kSeekCur              = 1,           //!< Seek from the current position (SEEK_CUR).
    kSeekEnd              = 2            //!< Seek from the end of the file (SEEK_END).
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE File() noexcept
    : _handle(invalidHandle()) {}

  B2D_INLINE File(File&& other) noexcept
    : _handle(other.handle()) { other._handle = invalidHandle(); }

  B2D_INLINE explicit File(HandleType handle) noexcept
    : _handle(handle) {}

  B2D_INLINE ~File() noexcept { close(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE bool isOpen() const noexcept { return _handle != invalidHandle(); }
  B2D_INLINE HandleType handle() const noexcept { return _handle; }

  B2D_INLINE HandleType takeHandle() noexcept {
    HandleType result = _handle;
    _handle = invalidHandle();
    return result;
  }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_API Error open(const char* fileName, uint32_t openFlags) noexcept;

  B2D_API Error _close() noexcept;
  B2D_INLINE Error close() noexcept { return _handle != invalidHandle() ? _close() : kErrorOk; }

  B2D_INLINE Error seek(int64_t offset, uint32_t seekType) noexcept {
    int64_t dummyPosition;
    return seek(offset, seekType, dummyPosition);
  }

  B2D_API Error seek(int64_t offset, uint32_t seekType, int64_t& positionOut) noexcept;
  B2D_API Error read(void* buffer, size_t size, size_t& bytesReadOut) noexcept;
  B2D_API Error write(const void* buffer, size_t size, size_t& bytesWrittenOut) noexcept;

  B2D_API Error size(uint64_t& sizeOut) noexcept;
  B2D_API Error truncate(int64_t maxSize) noexcept;

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE File& operator=(File&& other) noexcept {
    close();

    _handle = other._handle;
    other._handle = invalidHandle();

    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  HandleType _handle;
};

// ============================================================================
// [b2d::FileMapping]
// ============================================================================

//! A thin abstraction over `mmap() / munmap()` (Posix) and `FileMapping` (Windows)
//! to create a read-only file mapping for loading fonts and other resources.
class FileMapping {
public:
  B2D_NONCOPYABLE(FileMapping)

  enum Limits : uint32_t {
    kSmallFileSize = 1024 * 32           //!< Size threshold (32kB) used when `kCopySmallFiles` is enabled.
  };

  enum Status : uint32_t {
    kStatusEmpty          = 0,           //!< File is empty (not mapped nor copied).
    kStatusMapped         = 1,           //!< File is memory-mapped.
    kStatusCopied         = 2            //!< File was copied.
  };

  enum Flags : uint32_t {
    kCopySmallFiles       = 0x01000000u, //!< Copy the file data if it's smaller/equal to `kSmallFileSize.
    kCopyOnFailure        = 0x02000000u, //!< Copy the file data if mapping is not possible or it failed.
    kDontCloseOnCopy      = 0x04000000u  //!< Don't close the `file` handle if the file was copied.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  #if B2D_OS_WINDOWS
  B2D_INLINE FileMapping() noexcept
    : _file(),
      _hFileMapping(INVALID_HANDLE_VALUE),
      _data(nullptr),
      _size(0),
      _status(kStatusEmpty) {}

  B2D_INLINE FileMapping(FileMapping&& other) noexcept
    : _file(std::move(other._file)),
      _hFileMapping(other._hFileMapping),
      _data(other._data),
      _size(other._size),
      _status(other._status) {
    other._hFileMapping = INVALID_HANDLE_VALUE;
    other._data = nullptr;
    other._size = 0;
    other._status = kStatusEmpty;
  }
  #endif

  #if B2D_OS_POSIX
  B2D_INLINE FileMapping() noexcept
    : _data(nullptr),
      _size(0),
      _status(kStatusEmpty) {}

  B2D_INLINE FileMapping(FileMapping&& other) noexcept
    : _data(other._data),
      _size(other._size),
      _status(other._status) {
    other._data = nullptr;
    other._size = 0;
    other._status = kStatusEmpty;
  }
  #endif

  B2D_INLINE ~FileMapping() noexcept { unmap(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the status of the FileMapping, see `Status.
  B2D_INLINE uint32_t status() const noexcept { return _status; }

  //! Get whether the FileMapping has mapped data.
  B2D_INLINE bool isMapped() const noexcept { return _status == kStatusMapped; }
  //! Get whether the FileMapping has copied data.
  B2D_INLINE bool isCopied() const noexcept { return _status == kStatusCopied; }

  //! Get whether the FileMapping is empty (no file mapped or loaded).
  B2D_INLINE bool empty() const noexcept { return _status == kStatusEmpty; }
  //! Get whether the FileMapping has mapped or copied data.
  B2D_INLINE bool hasData() const noexcept { return _status != kStatusEmpty; }

  //! Get raw data to the FileMapping content..
  template<typename T = void>
  B2D_INLINE const T* data() const noexcept { return static_cast<T*>(_data); }

  //! Get the size of mapped or copied data.
  B2D_INLINE size_t size() const noexcept { return _size; }

  //! Get the associated `File` with `FileMapping` (Windows).
  B2D_INLINE File& file() noexcept { return _file; }

  #if B2D_OS_WINDOWS
  //! Get the FileMapping handle (Windows).
  B2D_INLINE HANDLE fileMappingHandle() const noexcept { return _hFileMapping; }
  #endif

  // --------------------------------------------------------------------------
  // [Map / Unmap]
  // --------------------------------------------------------------------------

  //! Maps file `file` to memory. Takes ownership of `file` (moves) on success.
  B2D_API Error map(File& file, uint32_t flags) noexcept;

  //! Unmaps previously mapped file or does nothing, if no file was mapped.
  B2D_API Error unmap() noexcept;

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE FileMapping& operator=(FileMapping&& other) noexcept {
    unmap();

    #if B2D_OS_WINDOWS
    _file = std::move(other._file);
    _hFileMapping = other._hFileMapping;
    other._hFileMapping = INVALID_HANDLE_VALUE;
    #endif

    _data = other._data;
    _size = other._size;
    _status = other._status;

    other._data = nullptr;
    other._size = 0;
    other._status = kStatusEmpty;

    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  File _file;

  #if B2D_OS_WINDOWS
  HANDLE _hFileMapping;
  #endif

  void* _data;
  size_t _size;
  uint32_t _status;
};

// ============================================================================
// [b2d::FileUtils]
// ============================================================================

namespace FileUtils {
  B2D_API Error _readFile(const char* fileName, Buffer& out, size_t maxSize = 0) noexcept;
  B2D_API Error _writeFile(const char* fileName, const Buffer& buffer, size_t& bytesWritten) noexcept;

  static B2D_INLINE Error readFile(const char* fileName, Buffer& out, size_t maxSize = 0) noexcept {
    return _readFile(fileName, out, maxSize);
  }

  static B2D_INLINE Error writeFile(const char* fileName, const Buffer& buffer) noexcept {
    size_t bytesWritten;
    return _writeFile(fileName, buffer, bytesWritten);
  }

  static B2D_INLINE Error writeFile(const char* fileName, const Buffer& buffer, size_t& bytesWritten) noexcept {
    return _writeFile(fileName, buffer, bytesWritten);
  }
}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_FILESYSTEM_H
