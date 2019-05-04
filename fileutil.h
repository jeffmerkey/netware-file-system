
//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//

#if (WINDOWS_NT_UTIL)

#define FILE_ERROR_MESSAGE_SIZE 256


VOID FileErrorMessageBox(HWND hWnd,
			 LPTSTR lpszFileName,
			 DWORD ErrorCode) ;


BOOL FileRead (HANDLE hFile,
	       VOID * lpMemory,
	       DWORD nAmtToRead) ;

BOOL FileWrite (HANDLE hFile,
		VOID * lpMemory,
		DWORD nAmtToWrite) ;


#define FileSeekBegin(hFile, lAmtToMove, lAmtToMoveHigh) \
   SetFilePointer (hFile, lAmtToMove, &lAmtToMoveHigh, FILE_BEGIN)

#define FileSeekEnd(hFile, lAmtToMove, lAmtToMoveHigh) \
   SetFilePointer (hFile, lAmtToMove, &lAmtToMoveHigh, FILE_END)

#define FileSeekCurrent(hFile, lAmtToMove, lAmtToMoveHigh) \
   SetFilePointer (hFile, lAmtToMove, &lAmtToMoveHigh, FILE_CURRENT)

#define FileTell(hFile) \
   SetFilePointer (hFile, 0, NULL, FILE_CURRENT)

#define FileHandleOpen(lpszFilePath)         \
   (HANDLE) CreateFile (lpszFilePath,        \
      GENERIC_READ | GENERIC_WRITE,          \
      FILE_SHARE_READ,                       \
      NULL, OPEN_EXISTING,                   \
      FILE_ATTRIBUTE_NORMAL, NULL)

#define FileHandleReadOnly(lpszFilePath)     \
   (HANDLE) CreateFile (lpszFilePath,        \
      GENERIC_READ ,                         \
      FILE_SHARE_READ,                       \
      NULL, OPEN_EXISTING,                   \
      FILE_ATTRIBUTE_NORMAL, NULL)

#define FileHandleCreate(lpszFilePath)       \
   (HANDLE) CreateFile (lpszFilePath,        \
      GENERIC_READ | GENERIC_WRITE,          \
      FILE_SHARE_READ,                       \
      NULL, CREATE_ALWAYS,                   \
      FILE_ATTRIBUTE_NORMAL, NULL)

int FileHandleSeekCurrent (HANDLE hFile,
			   int iAmtToMove,
			   LPTSTR lpszFilePath) ;


int FileHandleSeekStart (HANDLE hFile,
			 int iAmtToMove,
			 LPTSTR lpszFilePath) ;


BOOL FileHandleWrite (HANDLE hFile,
		      VOID * lpBuffer,
		      int cbWrite,
		      LPTSTR lpszFilePath) ;


VOID * FileMap (HANDLE hFile, HANDLE *phMapHandle) ;


BOOL FileUnMap (VOID * pBase, HANDLE hMapHandle) ;


void FileDriveDirectory (LPTSTR lpszFileSpec,
			 LPTSTR lpszDirectory) ;


void FileNameExtension (LPTSTR lpszSpec,
			LPTSTR lpszFileNameExtension) ;


#endif

