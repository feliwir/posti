// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/allocator.h"
#include "../core/buffer.h"
#include "../core/error.h"
#include "../core/filesystem.h"
#include "../core/math.h"
#include "../core/membuffer.h"
#include "../core/support.h"
#include "../core/unicode.h"

#if B2D_OS_POSIX
  #include <errno.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::File (Windows)]
// ============================================================================

#if B2D_OS_WINDOWS

static constexpr DWORD kMaxContinuousSize = 1024 * 1024 * 32; // 32 MB.

template<size_t N>
class WinU16String {
public:
  B2D_NONCOPYABLE(WinU16String);

  B2D_INLINE WinU16String() noexcept
    : _data(_embeddedData),
      _size(0),
      _capacity(N) {}

  B2D_INLINE ~WinU16String() noexcept {
    if (_data != _embeddedData)
      Allocator::systemRelease(_data);
  }

  B2D_INLINE uint16_t* data() const noexcept { return _data; }
  B2D_INLINE wchar_t* dataAsWideChar() const noexcept { return reinterpret_cast<wchar_t*>(_data); }

  B2D_INLINE size_t size() const noexcept { return _size; }
  B2D_INLINE size_t capacity() const noexcept { return _capacity; }

  B2D_INLINE void nullTerminate() noexcept { _data[_size] = uint16_t(0); }

  B2D_NOINLINE Error fromUtf8(const char* src) noexcept {
    size_t srcSize = std::strlen(src);
    Unicode::ConversionState conversionState;

    Error err = Unicode::utf16FromUtf8(_data, _capacity, src, srcSize, conversionState);
    if (!err) {
      _size = conversionState.dstIndex();
      nullTerminate();
      return err;
    }

    if (err != kErrorStringNoSpaceLeft) {
      _size = 0;
      nullTerminate();
      return err;
    }

    size_t procUtf8Size = conversionState.srcIndex();
    size_t procUtf16Size = conversionState.dstIndex();

    Unicode::ValidationState validationState;
    B2D_PROPAGATE(Unicode::validateUtf8(src + procUtf8Size, srcSize - procUtf8Size, validationState));

    size_t newSize = procUtf16Size + validationState.utf16Index();
    uint16_t* newData = static_cast<uint16_t*>(Allocator::systemAlloc((newSize + 1) * sizeof(uint16_t)));

    if (B2D_UNLIKELY(!newData)) {
      _size = 0;
      nullTerminate();
      return kErrorNoMemory;
    }

    std::memcpy(newData, _data, procUtf16Size * sizeof(uint16_t));
    Unicode::utf16FromUtf8(newData + procUtf16Size, newSize - procUtf16Size, src + procUtf8Size, srcSize - procUtf8Size, conversionState);
    B2D_ASSERT(newSize == procUtf16Size + conversionState.dstIndex());

    if (_data != _embeddedData)
      Allocator::systemRelease(_data);

    _data = newData;
    _size = newSize;
    _capacity = newSize;

    nullTerminate();
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint16_t* _data;
  size_t _size;
  size_t _capacity;
  uint16_t _embeddedData[N + 1];
};

Error File::open(const char* fileName, uint32_t openFlags) noexcept {
  // Desired Access
  // --------------
  //
  // The same flags as O_RDONLY|O_WRONLY|O_RDWR:

  DWORD dwDesiredAccess = 0;
  switch (openFlags & kOpenRW) {
    case kOpenRead : dwDesiredAccess = GENERIC_READ ; break;
    case kOpenWrite: dwDesiredAccess = GENERIC_WRITE; break;
    case kOpenRW   : dwDesiredAccess = GENERIC_READ | GENERIC_WRITE; break;
    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  // Creation Disposition
  // --------------------
  //
  // Since WinAPI documentation is so brief here is a better explanation
  // about various CreationDisposition modes, reformatted from SO:
  //
  //   https://stackoverflow.com/questions/14469607/difference-between-open-always-and-create-always-in-createfile-of-windows-api
  //
  // +-------------------------+-------------+--------------------+
  // | Creation Disposition    | File Exists | File Doesn't Exist |
  // +-------------------------+-------------+--------------------+
  // | CREATE_ALWAYS           | Truncate    | Create New         |
  // | CREATE_NEW              | Fail        | Create New         |
  // | OPEN_ALWAYS             | Open        | Create New         |
  // | OPEN_EXISTING           | Open        | Fail               |
  // | TRUNCATE_EXISTING       | Truncate    | Fail               |
  // +-------------------------+-------------+--------------------+

  if (openFlags & (kOpenCreate | kOpenCreateOnly | kOpenTruncate) && (!(openFlags & kOpenWrite)))
    return kErrorInvalidArgument;

  DWORD dwCreationDisposition = OPEN_EXISTING;
  if (openFlags & kOpenCreateOnly)
    dwCreationDisposition = CREATE_NEW;
  else if ((openFlags & (kOpenCreate | kOpenTruncate)) == kOpenCreate)
    dwCreationDisposition = OPEN_ALWAYS;
  else if ((openFlags & (kOpenCreate | kOpenTruncate)) == (kOpenCreate | kOpenTruncate))
    dwCreationDisposition = CREATE_ALWAYS;
  else if (openFlags & kOpenTruncate)
    dwCreationDisposition = TRUNCATE_EXISTING;

  // Share Mode
  // ----------

  DWORD dwShareMode = 0;

  if (openFlags & kOpenShareRead  ) dwShareMode |= FILE_SHARE_READ;
  if (openFlags & kOpenShareWrite ) dwShareMode |= FILE_SHARE_WRITE;
  if (openFlags & kOpenShareDelete) dwShareMode |= FILE_SHARE_DELETE;

  // Other Flags
  // -----------

  DWORD dwFlagsAndAttributes = 0;
  LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr;

  // NOTE: Do not close the file before calling `CreateFileW()`. We should
  // behave atomically, which means that we won't close the existing file
  // if `CreateFileW()` fails...
  WinU16String<1024> fileNameW;
  B2D_PROPAGATE(fileNameW.fromUtf8(fileName));

  HANDLE h = ::CreateFileW(fileNameW.dataAsWideChar(),
                           dwDesiredAccess,
                           dwShareMode,
                           lpSecurityAttributes,
                           dwCreationDisposition,
                           dwFlagsAndAttributes,
                           NULL);

  if (h == INVALID_HANDLE_VALUE) {
    return DebugUtils::errored(ErrorUtils::errorFromWinError(::GetLastError()));
  }
  else {
    close();
    _handle = h;
    return kErrorOk;
  }
}

Error File::_close() noexcept {
  BOOL result = ::CloseHandle(_handle);

  // Not sure what should happen if `CloseHandle()` fails, if the handle is invalid
  // or the close can be called again? To ensure compatibility with POSIX implementation
  // we just make it invalid.
  _handle = invalidHandle();

  if (!result)
    return DebugUtils::errored(ErrorUtils::errorFromWinError(::GetLastError()));
  else
    return kErrorOk;
}

Error File::seek(int64_t offset, uint32_t seekType, int64_t& positionOut) noexcept {
  DWORD dwMoveMethod = 0;
  positionOut = -1;

  switch (seekType) {
    case kSeekSet: dwMoveMethod = FILE_BEGIN  ; break;
    case kSeekCur: dwMoveMethod = FILE_CURRENT; break;
    case kSeekEnd: dwMoveMethod = FILE_END    ; break;

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  if (!isOpen())
    return DebugUtils::errored(kErrorInvalidHandle);

  LARGE_INTEGER to;
  LARGE_INTEGER prev;

  to.QuadPart = offset;
  prev.QuadPart = 0;

  BOOL result = ::SetFilePointerEx(_handle, to, &prev, dwMoveMethod);
  if (!result) {
    return DebugUtils::errored(ErrorUtils::errorFromWinError(::GetLastError()));
  }
  else {
    positionOut = prev.QuadPart;
    return kErrorOk;
  }
}

Error File::read(void* buffer, size_t size, size_t& bytesReadOut) noexcept {
  bytesReadOut = 0;
  if (!isOpen())
    return DebugUtils::errored(kErrorInvalidHandle);

  BOOL result = true;
  size_t remainingSize = size;
  size_t bytesReadTotal = 0;

  while (remainingSize) {
    DWORD localSize = static_cast<DWORD>(Math::min<size_t>(remainingSize, kMaxContinuousSize));
    DWORD bytesRead = 0;

    result = ::ReadFile(_handle, buffer, localSize, &bytesRead, NULL);
    remainingSize -= localSize;
    bytesReadTotal += bytesRead;

    if (bytesRead < localSize || !result)
      break;

    buffer = Support::advancePtr(buffer, bytesRead);
  }

  bytesReadOut = bytesReadTotal;
  if (!result) {
    DWORD e = ::GetLastError();
    if (e == ERROR_HANDLE_EOF)
      return kErrorOk;
    return DebugUtils::errored(ErrorUtils::errorFromWinError(e));
  }
  else {
    return kErrorOk;
  }
}

Error File::write(const void* buffer, size_t size, size_t& bytesWrittenOut) noexcept {
  bytesWrittenOut = 0;
  if (!isOpen())
    return DebugUtils::errored(kErrorInvalidHandle);

  BOOL result = true;
  size_t remainingSize = size;
  size_t bytesWrittenTotal = 0;

  while (remainingSize) {
    DWORD localSize = static_cast<DWORD>(Math::min<size_t>(remainingSize, kMaxContinuousSize));
    DWORD bytesWritten = 0;

    result = ::WriteFile(_handle, buffer, localSize, &bytesWritten, NULL);
    remainingSize -= localSize;
    bytesWrittenTotal += bytesWritten;

    if (bytesWritten < localSize || !result)
      break;

    buffer = Support::advancePtr(buffer, bytesWritten);
  }

  bytesWrittenOut = bytesWrittenTotal;
  if (!result)
    return DebugUtils::errored(ErrorUtils::errorFromWinError(::GetLastError()));
  else
    return kErrorOk;
}

Error File::size(uint64_t& sizeOut) noexcept {
  sizeOut = 0;
  if (!isOpen())
    return DebugUtils::errored(kErrorInvalidHandle);

  LARGE_INTEGER size;
  BOOL result = ::GetFileSizeEx(handle(), &size);

  if (!result)
    return DebugUtils::errored(ErrorUtils::errorFromWinError(::GetLastError()));

  sizeOut = uint64_t(size.QuadPart);
  return kErrorOk;
}

Error File::truncate(int64_t maxSize) noexcept {
  if (!isOpen())
    return DebugUtils::errored(kErrorInvalidHandle);

  if (maxSize < 0)
    return DebugUtils::errored(kErrorInvalidArgument);

  int64_t prev;
  B2D_PROPAGATE(seek(maxSize, kSeekSet, prev));

  BOOL result = ::SetEndOfFile(_handle);
  if (prev < maxSize)
    seek(prev, kSeekSet, prev);

  if (!result)
    return DebugUtils::errored(ErrorUtils::errorFromWinError(::GetLastError()));
  else
    return kErrorOk;
}

#endif

// ============================================================================
// [b2d::File (Posix)]
// ============================================================================

#if B2D_OS_POSIX

// These OSes use 64-bit offsets by default.
#if B2D_OS_BSD || B2D_OS_MAC
#  define B2D_FILE64_API(NAME) NAME
#else
#  define B2D_FILE64_API(NAME) NAME##64
#endif

Error File::open(const char* fileName, uint32_t openFlags) noexcept {
  int of = 0;

  switch (openFlags & kOpenRW) {
    case kOpenRead : of |= O_RDONLY; break;
    case kOpenWrite: of |= O_WRONLY; break;
    case kOpenRW   : of |= O_RDWR  ; break;
    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  if (openFlags & (kOpenCreate | kOpenCreateOnly | kOpenTruncate) && (!(openFlags & kOpenWrite)))
    return DebugUtils::errored(kErrorInvalidArgument);

  if (openFlags & kOpenCreate    ) of |= O_CREAT;
  if (openFlags & kOpenCreateOnly) of |= O_CREAT | O_EXCL;
  if (openFlags & kOpenTruncate  ) of |= O_TRUNC;

  mode_t om = S_IRUSR | S_IWUSR |
              S_IRGRP | S_IWGRP |
              S_IROTH | S_IWOTH ;

  // NOTE: Do not close the file before calling `open()`. We should
  // behave atomically, which means that we won't close the existing
  // file if `open()` fails...
  int fd = B2D_FILE64_API(::open)(fileName, of, om);
  if (fd < 0) {
    return DebugUtils::errored(ErrorUtils::errorFromPosixError(errno));
  }
  else {
    close();
    _handle = fd;
    return kErrorOk;
  }
}

Error File::_close() noexcept {
  int result = ::close(_handle);

  // NOTE: Even when `close()` fails the handle cannot be used again as could
  // already be reused. The failure is just to inform the user that there may
  // be data-loss.
  _handle = invalidHandle();

  if (result != 0)
    return DebugUtils::errored(ErrorUtils::errorFromPosixError(errno));
  else
    return kErrorOk;
}

Error File::seek(int64_t offset, uint32_t seekType, int64_t& positionOut) noexcept {
  int whence = 0;
  positionOut = -1;

  switch (seekType) {
    case kSeekSet: whence = SEEK_SET; break;
    case kSeekCur: whence = SEEK_CUR; break;
    case kSeekEnd: whence = SEEK_END; break;

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  if (!isOpen())
    return DebugUtils::errored(kErrorInvalidHandle);

  int64_t result = B2D_FILE64_API(::lseek)(_handle, offset, whence);
  if (result < 0) {
    int e = errno;

    // Returned when the file was not open for reading or writing.
    if (e == EBADF)
      return DebugUtils::errored(kErrorIONotPermitted);

    return DebugUtils::errored(ErrorUtils::errorFromPosixError(errno));
  }
  else {
    positionOut = result;
    return kErrorOk;
  }
}

Error File::read(void* buffer, size_t size, size_t& bytesReadOut) noexcept {
  bytesReadOut = 0;
  if (!isOpen())
    return DebugUtils::errored(kErrorInvalidHandle);

  typedef std::make_signed<size_t>::type SignedSizeT;
  SignedSizeT result = ::read(_handle, buffer, size);

  if (result < 0) {
    int e = errno;
    bytesReadOut = 0;

    // Returned when the file was not open for reading.
    if (e == EBADF)
      return DebugUtils::errored(kErrorIONotPermitted);

    return DebugUtils::errored(ErrorUtils::errorFromPosixError(e));
  }
  else {
    bytesReadOut = size_t(result);
    return kErrorOk;
  }
}

Error File::write(const void* buffer, size_t size, size_t& bytesWrittenOut) noexcept {
  bytesWrittenOut = 0;
  if (!isOpen())
    return DebugUtils::errored(kErrorInvalidHandle);

  typedef std::make_signed<size_t>::type SignedSizeT;
  SignedSizeT result = ::write(_handle, buffer, size);

  if (result < 0) {
    int e = errno;
    bytesWrittenOut = 0;

    // These are the two errors that would be returned if the file was open for read-only.
    if (e == EBADF || e == EINVAL)
      return DebugUtils::errored(kErrorIONotPermitted);

    return DebugUtils::errored(ErrorUtils::errorFromPosixError(e));
  }
  else {
    bytesWrittenOut = size_t(result);
    return kErrorOk;
  }
}

Error File::size(uint64_t& sizeOut) noexcept {
  sizeOut = 0;
  if (!isOpen())
    return DebugUtils::errored(kErrorInvalidHandle);

  struct stat s;
  if (::fstat(handle(), &s) != 0)
    return DebugUtils::errored(ErrorUtils::errorFromPosixError(errno));

  sizeOut = s.st_size;
  return kErrorOk;
}

Error File::truncate(int64_t maxSize) noexcept {
  if (!isOpen())
    return DebugUtils::errored(kErrorInvalidHandle);

  if (maxSize < 0)
    return DebugUtils::errored(kErrorInvalidArgument);

  int result = B2D_FILE64_API(::ftruncate)(_handle, maxSize);
  if (result != 0) {
    int e = errno;

    // These are the two errors that would be returned if the file was open for read-only.
    if (e == EBADF || e == EINVAL)
      return DebugUtils::errored(kErrorIONotPermitted);

    // File was smaller than `maxSize` - we don't consider this to be an error.
    if (e == EFBIG)
      return kErrorOk;

    return DebugUtils::errored(ErrorUtils::errorFromPosixError(e));
  }
  else {
    return kErrorOk;
  }
}

#undef B2D_FILE64_API

#endif

// ============================================================================
// [b2d::FileMapping - Utilities]
// ============================================================================

static Error FileMapping_createCopyOfFile(FileMapping& self, File& file, uint32_t flags, size_t size) noexcept {
  Error err = file.seek(0, File::kSeekSet);
  if (err != kErrorOk && err != kErrorIOInvalidSeek)
    return err;

  void* data = Allocator::systemAlloc(size);
  if (!data)
    return DebugUtils::errored(kErrorNoMemory);

  size_t bytesRead;
  err = file.read(data, size, bytesRead);

  if (err != kErrorOk || bytesRead != size) {
    Allocator::systemRelease(data);
    return err ? err : DebugUtils::errored(kErrorIOEndOfFile);
  }

  self.unmap();
  self._data = data;
  self._size = size;
  self._status = FileMapping::kStatusCopied;

  // Since we loaded the whole `file` and don't need to retain the handle we
  // close it. This makes the API to behave the same way as if we mapped it
  // and we kept the handle.
  if (!(flags & FileMapping::kDontCloseOnCopy))
    file.close();

  return kErrorOk;
}

static Error FileMapping_releaseCopyOfFile(FileMapping& self) noexcept {
  Allocator::systemRelease(self._data);
  self._data = nullptr;
  self._size = 0;
  self._status = FileMapping::kStatusEmpty;
  return kErrorOk;
}

static Error FileMapping_check64BitFileSize(size_t& dstSize, uint64_t srcSize) noexcept {
  if (!srcSize)
    return DebugUtils::errored(kErrorIOFileEmpty);

  #if B2D_ARCH_BITS < 64
  if (srcSize > std::numeric_limits<size_t>::max())
    return DebugUtils::errored(kErrorIOFileTooLarge);
  #endif

  dstSize = size_t(srcSize);
  return kErrorOk;
}

// ============================================================================
// [b2d::FileMapping - Windows]
// ============================================================================

#if B2D_OS_WINDOWS
Error FileMapping::map(File& file, uint32_t flags) noexcept {
  if (!file.isOpen())
    return DebugUtils::errored(kErrorInvalidArgument);

  DWORD dwProtect = PAGE_READONLY;
  DWORD dwDesiredAccess = FILE_MAP_READ;

  // Get the size of the file.
  size_t size;
  uint64_t size64;

  B2D_PROPAGATE(file.size(size64));
  B2D_PROPAGATE(FileMapping_check64BitFileSize(size, size64));

  if ((flags & kCopySmallFiles) && size <= kSmallFileSize)
    return FileMapping_createCopyOfFile(*this, file, flags, size);

  // Create a file mapping handle and map view of file into it.
  HANDLE hFileMapping = ::CreateFileMappingW(file.handle(), NULL, dwProtect, 0, 0, NULL);
  void* data = nullptr;

  if (hFileMapping != NULL) {
    data = ::MapViewOfFile(hFileMapping, dwDesiredAccess, 0, 0, 0);
    if (!data)
      ::CloseHandle(hFileMapping);
  }

  if (data == nullptr) {
    if (!(flags & kCopyOnFailure))
      return DebugUtils::errored(ErrorUtils::errorFromWinError(::GetLastError()));
    else
      return FileMapping_createCopyOfFile(*this, file, flags, size);
  }
  else {
    // Succeeded, now is the time to change the content of `FileMapping`.
    unmap();

    _file._handle = file.takeHandle();
    _hFileMapping = hFileMapping;
    _data = data;
    _size = size;
    _status = kStatusMapped;

    return kErrorOk;
  }
}

Error FileMapping::unmap() noexcept {
  switch (status()) {
    case kStatusMapped: {
      Error err = kErrorOk;
      DWORD dwError = 0;

      if (!::UnmapViewOfFile(_data))
        dwError = ::GetLastError();

      if (!::CloseHandle(_hFileMapping) && !dwError)
        dwError = ::GetLastError();

      if (dwError)
        err = DebugUtils::errored(ErrorUtils::errorFromWinError(dwError));

      _file.close();
      _hFileMapping = INVALID_HANDLE_VALUE;
      _data = nullptr;
      _size = 0;
      _status = kStatusEmpty;

      return err;
    }

    case kStatusCopied:
      return FileMapping_releaseCopyOfFile(*this);

    default:
      return kErrorOk;
  }
}
#endif

// ============================================================================
// [b2d::FileMapping - Posix]
// ============================================================================

#if B2D_OS_POSIX
Error FileMapping::map(File& file, uint32_t flags) noexcept {
  if (!file.isOpen())
    return DebugUtils::errored(kErrorInvalidArgument);

  int mmapProt = PROT_READ;
  int mmapFlags = MAP_SHARED;

  size_t size;
  uint64_t size64;

  B2D_PROPAGATE(file.size(size64));
  B2D_PROPAGATE(FileMapping_check64BitFileSize(size, size64));

  if ((flags & kCopySmallFiles) && size <= kSmallFileSize)
    return FileMapping_createCopyOfFile(*this, file, flags, size);

  // Create the mapping.
  void* data = ::mmap(NULL, size, mmapProt, mmapFlags, file.handle(), 0);
  if (data == (void *)-1) {
    if (!(flags & kCopyOnFailure))
      return DebugUtils::errored(ErrorUtils::errorFromPosixError(errno));
    else
      return FileMapping_createCopyOfFile(*this, file, flags, size);
  }
  else {
    // Succeeded, now is the time to change the content of `FileMapping`.
    unmap();

    _file._handle = file.takeHandle();
    _data = data;
    _size = size;
    _status = kStatusMapped;

    return kErrorOk;
  }
}

Error FileMapping::unmap() noexcept {
  switch (status()) {
    case kStatusMapped: {
      Error err = kErrorOk;

      // If error happened we must read `errno` now as a call to `close()` may
      // trash it as well. We prefer the first error instead of the last one.
      int result = ::munmap(_data, _size);
      if (result != 0)
        err = DebugUtils::errored(ErrorUtils::errorFromPosixError(errno));

      _file.close();
      _data = nullptr;
      _size = 0;
      _status = kStatusEmpty;

      return err;
    }

    case kStatusCopied:
      return FileMapping_releaseCopyOfFile(*this);

    default:
      return kErrorOk;
  }
}
#endif

// ============================================================================
// [b2d::FileUtils]
// ============================================================================

Error FileUtils::_readFile(const char* fileName, Buffer& out, size_t maxSize) noexcept {
  out.reset();

  File file;
  B2D_PROPAGATE(file.open(fileName, File::kOpenRead));

  uint64_t size64;
  B2D_PROPAGATE(file.size(size64));

  if (size64 == 0)
    return DebugUtils::errored(kErrorIOFileEmpty);

  if (size64 >= uint64_t(std::numeric_limits<size_t>::max()))
    return DebugUtils::errored(kErrorIOFileTooLarge);

  size_t size = size_t(size64);
  if (maxSize && size > maxSize)
    size = maxSize;

  uint8_t* data = out.modify(kContainerOpReplace, size);
  if (!data)
    return DebugUtils::errored(kErrorNoMemory);

  size_t bytesRead;
  Error err = file.read(data, size, bytesRead);

  if (err)
    out.reset();
  else if (size != bytesRead)
    out._end(bytesRead);

  return err;
}

Error FileUtils::_writeFile(const char* fileName, const Buffer& buffer, size_t& bytesWritten) noexcept {
  Error err = kErrorOk;
  bytesWritten = 0;

  File file;
  B2D_PROPAGATE(file.open(fileName, File::kOpenWrite | File::kOpenCreate | File::kOpenTruncate));

  if (buffer.empty())
    return kErrorOk;
  else
    return file.write(buffer.data(), buffer.size(), bytesWritten);
}

B2D_END_NAMESPACE
