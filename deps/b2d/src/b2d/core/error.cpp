// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// The DEFLATE algorithm is based on stb_image <https://github.com/nothings/stb>
// released into PUBLIC DOMAIN. It was initially part of Blend2D's PNG decoder,
// but moved to the base library later on. The DEFLATE implementation can be
// distributed under both Blend2D's ZLIB license and STB's PUBLIC DOMAIN license.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/error.h"

#if B2D_OS_POSIX
  #include <errno.h>
  #include <unistd.h>
#endif

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::ErrorUtils - ErrorFromWinError (Windows)]
// ============================================================================

#if B2D_OS_WINDOWS
B2D_API Error ErrorUtils::errorFromWinError(uint32_t e) noexcept {
  switch (e) {
    case ERROR_SUCCESS                : return kErrorOk;                   // 0x00000000
    case ERROR_INVALID_FUNCTION       : return kErrorIONotPermitted;       // 0x00000001
    case ERROR_FILE_NOT_FOUND         : return kErrorIONoEntry;            // 0x00000002
    case ERROR_PATH_NOT_FOUND         : return kErrorIONoEntry;            // 0x00000003
    case ERROR_TOO_MANY_OPEN_FILES    : return kErrorIOOutOfHandles;       // 0x00000004
    case ERROR_ACCESS_DENIED          : return kErrorIOAccessDenied;       // 0x00000005
    case ERROR_INVALID_HANDLE         : return kErrorInvalidHandle;        // 0x00000006
    case ERROR_NOT_ENOUGH_MEMORY      : return kErrorNoMemory;             // 0x00000008
    case ERROR_OUTOFMEMORY            : return kErrorNoMemory;             // 0x0000000E
    case ERROR_INVALID_DRIVE          : return kErrorIONoEntry;            // 0x0000000F
    case ERROR_CURRENT_DIRECTORY      : return kErrorIONotPermitted;       // 0x00000010
    case ERROR_NOT_SAME_DEVICE        : return kErrorIONotSameDevice;      // 0x00000011
    case ERROR_NO_MORE_FILES          : return kErrorIONoMoreFiles;        // 0x00000012
    case ERROR_WRITE_PROTECT          : return kErrorIOReadOnly;           // 0x00000013
    case ERROR_NOT_READY              : return kErrorIONoMedia;            // 0x00000015
    case ERROR_CRC                    : return kErrorIORWFailure;          // 0x00000017
    case ERROR_SEEK                   : return kErrorIOInvalidSeek;        // 0x00000019
    case ERROR_WRITE_FAULT            : return kErrorIORWFailure;          // 0x0000001D
    case ERROR_READ_FAULT             : return kErrorIORWFailure;          // 0x0000001E
    case ERROR_GEN_FAILURE            : return kErrorIORWFailure;          // 0x0000001F
    case ERROR_SHARING_BUFFER_EXCEEDED: return kErrorIOOutOfHandles;       // 0x00000024
    case ERROR_HANDLE_EOF             : return kErrorIOEndOfFile;          // 0x00000026
    case ERROR_HANDLE_DISK_FULL       : return kErrorIONoSpaceLeft;        // 0x00000027
    case ERROR_NOT_SUPPORTED          : return kErrorUnsupportedOperation; // 0x00000032
    case ERROR_FILE_EXISTS            : return kErrorIOAlreadyExists;      // 0x00000050
    case ERROR_CANNOT_MAKE            : return kErrorIONotPermitted;       // 0x00000052
    case ERROR_INVALID_PARAMETER      : return kErrorInvalidArgument;      // 0x00000057
    case ERROR_NET_WRITE_FAULT        : return kErrorIORWFailure;          // 0x00000058
    case ERROR_DRIVE_LOCKED           : return kErrorResourceBusy;         // 0x0000006C
    case ERROR_BROKEN_PIPE            : return kErrorIOPipe;               // 0x0000006D
    case ERROR_OPEN_FAILED            : return kErrorIOOpenFailure;        // 0x0000006E
    case ERROR_BUFFER_OVERFLOW        : return kErrorIONameTooLong;        // 0x0000006F
    case ERROR_DISK_FULL              : return kErrorIONoSpaceLeft;        // 0x00000070
    case ERROR_CALL_NOT_IMPLEMENTED   : return kErrorUnsupportedOperation; // 0x00000078
    case ERROR_INVALID_NAME           : return kErrorIONameInvalid;        // 0x0000007B
    case ERROR_NEGATIVE_SEEK          : return kErrorIOInvalidSeek;        // 0x00000083
    case ERROR_SEEK_ON_DEVICE         : return kErrorIOInvalidSeek;        // 0x00000084
    case ERROR_BUSY_DRIVE             : return kErrorResourceBusy;         // 0x0000008E
    case ERROR_DIR_NOT_ROOT           : return kErrorIONotRootDirectory;   // 0x00000090
    case ERROR_DIR_NOT_EMPTY          : return kErrorIONotEmpty;           // 0x00000091
    case ERROR_PATH_BUSY              : return kErrorResourceBusy;         // 0x00000094
    case ERROR_BAD_ARGUMENTS          : return kErrorInvalidArgument;      // 0x000000A0
    case ERROR_BAD_PATHNAME           : return kErrorIONameInvalid;        // 0x000000A1
    case ERROR_SIGNAL_PENDING         : return kErrorResourceBusy;         // 0x000000A2
    case ERROR_BUSY                   : return kErrorResourceBusy;         // 0x000000AA
    case ERROR_ALREADY_EXISTS         : return kErrorIOAlreadyExists;      // 0x000000B7
    case ERROR_BAD_PIPE               : return kErrorIOPipe;               // 0x000000E6
    case ERROR_PIPE_BUSY              : return kErrorResourceBusy;         // 0x000000E7
    case ERROR_NO_MORE_ITEMS          : return kErrorIONoMoreFiles;        // 0x00000103
    case ERROR_FILE_INVALID           : return kErrorIONoEntry;            // 0x000003EE
    case ERROR_NO_DATA_DETECTED       : return kErrorIOGeneric;            // 0x00000450
    case ERROR_MEDIA_CHANGED          : return kErrorIOMediaChanged;       // 0x00000456
    case ERROR_IO_DEVICE              : return kErrorIONoDevice;           // 0x0000045D
    case ERROR_NO_MEDIA_IN_DRIVE      : return kErrorIONoMedia;            // 0x00000458
    case ERROR_DISK_OPERATION_FAILED  : return kErrorIORWFailure;          // 0x00000467
    case ERROR_TOO_MANY_LINKS         : return kErrorIOTooManyLinks;       // 0x00000476
    case ERROR_DISK_QUOTA_EXCEEDED    : return kErrorIONoSpaceLeft;        // 0x0000050F
    case ERROR_INVALID_USER_BUFFER    : return kErrorResourceBusy;         // 0x000006F8
    case ERROR_UNRECOGNIZED_MEDIA     : return kErrorIOGeneric;            // 0x000006F9
    case ERROR_NOT_ENOUGH_QUOTA       : return kErrorNoMemory;             // 0x00000718
    case ERROR_CANT_ACCESS_FILE       : return kErrorIONotPermitted;       // 0x00000780
    case ERROR_CANT_RESOLVE_FILENAME  : return kErrorIONoEntry;            // 0x00000781
    case ERROR_OPEN_FILES             : return kErrorIONotAvailable;       // 0x00000961
  }

  // Pass the system error if it's below our error indexing.
  if (e < _kErrorCommonStart)
    return e;

  // Otherwise this is an unmapped system error code.
  return kErrorUnsupportedErrorCode;
}
#endif

// ============================================================================
// [b2d::ErrorUtils - ErrorFromPosixError (Posix)]
// ============================================================================

#if B2D_OS_POSIX
B2D_API Error ErrorUtils::errorFromPosixError(int e) noexcept {
  switch (e) {
    #if defined(EACCES)
    case EACCES      : return kErrorIOAccessDenied;
    #endif
    #if defined(EAGAIN)
    case EAGAIN      : return kErrorIONotAvailable;
    #endif
    #if defined(EBADF)
    case EBADF       : return kErrorInvalidHandle;
    #endif
    #if defined(EBUSY)
    case EBUSY       : return kErrorResourceBusy;
    #endif
    #if defined(EDQUOT)
    case EDQUOT      : return kErrorIONoSpaceLeft;
    #endif
    #if defined(EEXIST)
    case EEXIST      : return kErrorIOAlreadyExists;
    #endif
    #if defined(EFAULT)
    case EFAULT      : return kErrorInvalidState;
    #endif
    #if defined(EFBIG)
    case EFBIG       : return kErrorIOFileTooLarge;
    #endif
    #if defined(EINTR)
    case EINTR       : return kErrorIOInterrupted;
    #endif
    #if defined(EINVAL)
    case EINVAL      : return kErrorInvalidArgument;
    #endif
    #if defined(EIO)
    case EIO         : return kErrorIOGeneric;
    #endif
    #if defined(EISDIR)
    case EISDIR      : return kErrorIONotFile;
    #endif
    #if defined(ELOOP)
    case ELOOP       : return kErrorIOSymlinkLoop;
    #endif
    #if defined(EMFILE)
    case EMFILE      : return kErrorIOOutOfHandles;
    #endif
    #if defined(ENAMETOOLONG)
    case ENAMETOOLONG: return kErrorIONameTooLong;
    #endif
    #if defined(ENFILE)
    case ENFILE      : return kErrorIOOutOfHandles;
    #endif
    #if defined(ENMFILE)
    case ENMFILE     : return kErrorIONoMoreFiles;
    #endif
    #if defined(ENODATA)
    case ENODATA     : return kErrorIOEndOfFile;
    #endif
    #if defined(ENODEV)
    case ENODEV      : return kErrorIONoDevice;
    #endif
    #if defined(ENOENT)
    case ENOENT      : return kErrorIONoEntry;
    #endif
    #if defined(ENOMEDIUM)
    case ENOMEDIUM   : return kErrorIONoMedia;
    #endif
    #if defined(ENOSPC)
    case ENOSPC      : return kErrorIONoSpaceLeft;
    #endif
    #if defined(ENOTBLK)
    case ENOTBLK     : return kErrorIONotBlockDevice;
    #endif
    #if defined(ENOTDIR)
    case ENOTDIR     : return kErrorIONotDirectory;
    #endif
    #if defined(ENOTEMPTY)
    case ENOTEMPTY   : return kErrorIONotEmpty;
    #endif
    #if defined(ENXIO)
    case ENXIO       : return kErrorIONoDevice;
    #endif
    #if defined(EOVERFLOW)
    case EOVERFLOW   : return kErrorIOOutOfRange;
    #endif
    #if defined(EPERM)
    case EPERM       : return kErrorIONotPermitted;
    #endif
    #if defined(EROFS)
    case EROFS       : return kErrorIOReadOnly;
    #endif
    #if defined(ESPIPE)
    case ESPIPE      : return kErrorIOInvalidSeek;
    #endif
    #if defined(EXDEV)
    case EXDEV       : return kErrorIONotSameDevice;
    #endif
  }

  // Pass the system error if it's below our error indexing.
  if (e != 0 && e < _kErrorCommonStart)
    return uint32_t(e);
  else
    return kErrorUnsupportedErrorCode;
}
#endif

B2D_END_NAMESPACE
