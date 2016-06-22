//
// jdots.c
//
// Command line tool for LocBench
//

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include "list.h"               // Definition list ADT

#define     MAX_OPTIONS     20  // Max # command line options

// Flags indices
#define     FL_HELP         0   // Print usage

extern DWORD Options( int argc, LPCWSTR argv[], LPCWSTR OptStr, ... );
extern VOID ReportError( LPCTSTR userMsg, DWORD exitCode, BOOL prtErrorMsg );

BOOL scanDir( LPTSTR tDir, List* resList, Item* parentItem );
int cmpItemsName( Item* pItemN, Item* pItemM );
void showResults( List* resultsList, Item* resultsLevel );
void showItem( Item* pItem );
void sepThousands( const long long* numPt, TCHAR* acc, size_t elemsAcc );

int wmain( int argc, LPTSTR argv[] )
{
    // Vars declarations
    int targetDirInd = 0;
    BOOL flags[ MAX_OPTIONS ] = { 0 };
    TCHAR workDir[ MAX_PATH ] = { 0 };
    TCHAR targetDir[ MAX_PATH ] = { 0 };
    DWORD workLength = 0;
    List resultsList = { 0 };
    Item resultsItem = { 0 };
    PVOID oldValueWow64 = NULL;
    BOOL wow64Disabled = FALSE;

    // Get index of first argument after options
    // Also determine which options are active
    targetDirInd = Options( argc, argv, TEXT( "h" ), &flags[ FL_HELP ], NULL );
    
    // Get current working dir
    workLength = GetCurrentDirectory( _countof( workDir ), workDir );

    // Validate target dir
    if ( ( argc > targetDirInd + 1 ) || flags[ FL_HELP ] )
    {
        // More than one target or
        // target with gaps (no quotes) specified or
        // asked for help

        // Print usage
        wprintf_s( TEXT( "\n    Usage:    jdots [options] [target dir]\n\n" ) );
        wprintf_s( TEXT( "    Options:\n\n" ) );
        wprintf_s( TEXT( "      -h   :  Print usage\n\n" ) );
        wprintf_s( TEXT( "    If no target dir is specified, then the current working dir will be used\n" ) );

        return 1;
    }
    else if ( ( argc < targetDirInd + 1 ) && ( workLength <= MAX_PATH - 3 ) )
    {
        // No target specified --> assume current dir
        wcscpy_s( targetDir, MAX_PATH, workDir );
    }
    else if ( argc == targetDirInd + 1 )
    {
        // One target specified

        // Validate target dir starting with '\'
        if ( argv[ targetDirInd ][ 0 ] == '\\' )
        {
            // Fetch drive letter from working dir
            wcsncpy_s( targetDir, MAX_PATH, workDir, 2 );
        }

        // Append passed dir to target dir
        wcscat_s( targetDir, MAX_PATH, argv[ targetDirInd ] );
    }

    // Set up absolute target dir --> resolve '.' and '..' in target dir
    if ( !SetCurrentDirectory( targetDir ) )
    {
        ReportError( TEXT( "\nTarget directory not found.\n" ), 0, TRUE );
        return 1;
    }

    // Display absolute target dir
    GetCurrentDirectory( _countof( targetDir ), targetDir );
    wprintf_s( TEXT( "\n    Target dir: \"%s\"\n\n" ), targetDir );

    // Initialize results list
    InitializeList( &resultsList );
    if ( ListIsFull( &resultsList ) )
    {
        wprintf_s( TEXT( "\nNo memory available!\n" ) );
        return 1;
    }

    // Disable file system redirection
    wow64Disabled = Wow64DisableWow64FsRedirection( &oldValueWow64 );

    // Scan target dir
    scanDir( targetDir, &resultsList, &resultsItem );

    // Re-enable redirection
    if ( wow64Disabled )
    {
        if ( !( Wow64RevertWow64FsRedirection( oldValueWow64 ) ) )
            ReportError( TEXT( "Re-enable redirection failed." ), 1, TRUE );
    }

    // Display results
    if ( ListIsEmpty( &resultsList ) )
        wprintf_s( TEXT( "\nNo data.\n\n" ) );
    else
    {
        // Sort by name (a to Z)
        SortList( &resultsList, cmpItemsName );

        // Display sorted results
        showResults( &resultsList, &resultsItem );
    }

    // Housekeeping
    EmptyTheList( &resultsList );

    return 0;
}

BOOL scanDir( LPTSTR tDir, List* resList, Item* parentItem )
{
    TCHAR dirStr[ MAX_PATH ] = { 0 };
    HANDLE hFind = INVALID_HANDLE_VALUE;

    Item currentItem = { 0 };
    
    LARGE_INTEGER parentSize = { 0 };
    LARGE_INTEGER currentSize = { 0 };

    // Prepare string for use with FindFile functions.
    
    // Validate space to extend dirStr
    // There must be enough space to append
    // "*.nmea" + '\0' or "\*.nmea" + '\0'
    if ( wcsnlen( tDir, MAX_PATH ) >= MAX_PATH - 8 )
    {
        wprintf_s( TEXT( "\nDirectory path is too long.\n" ) );
        return FALSE;
    }

    // Copy passed string to buffer,
    // then append "*.nmea" + '\0' or "\*.nmea" + '\0' to the directory name.
    wcscpy_s( dirStr, MAX_PATH, tDir );

    if ( dirStr[ wcslen( dirStr ) - 1 ] == TEXT( '\\' ) )
        wcscat_s( dirStr, MAX_PATH, TEXT( "*.nmea" ) );
    else
        wcscat_s( dirStr, MAX_PATH, TEXT( "\\*.nmea" ) );

    // Find the first file in the directory.
    hFind = FindFirstFile( dirStr, &currentItem.findInfo );

    // Validate search handle
    if ( INVALID_HANDLE_VALUE == hFind )
    {
        // Only report error if different from 'Access Denied'.
        // For example, system symbolic links report 'access denied'.
        // If a handle is obtained and the size is requested,
        // Win32 reports 0 bytes.
        // See results using '..\progsDev\others\TestGetFileSizeEx\'
        if ( GetLastError() != ERROR_ACCESS_DENIED )
            ReportError( TEXT( "FindFirstFile failed." ), 0, TRUE );

        // Exit in any case
        return FALSE;
    }

    // List only nmea files in target dir
    do
    {
        // Do not follow symbolic links
        if ( !( currentItem.findInfo.dwFileAttributes &
                FILE_ATTRIBUTE_REPARSE_POINT ) )
        {
            // Ignore subdirs
            if ( !( currentItem.findInfo.dwFileAttributes &
                FILE_ATTRIBUTE_DIRECTORY ) )
            {
                // File found

                // Increment files count of parent
                ++parentItem->filesCount.QuadPart;

                // Current item has one file
                currentItem.filesCount.QuadPart = 1;


                // Update last write time information of the parent.
                // Is current item's LastWriteTime later
                // than parent's LastWriteTime ?
                if ( CompareFileTime( &currentItem.findInfo.ftLastWriteTime,
                    &parentItem->findInfo.ftLastWriteTime ) == 1 )
                {
                    // Parent gets the LastWriteTime from current item
                    parentItem->findInfo.ftLastWriteTime =
                        currentItem.findInfo.ftLastWriteTime;
                }

                // Get size of current found file(s)
                currentSize.LowPart = currentItem.findInfo.nFileSizeLow;
                currentSize.HighPart = currentItem.findInfo.nFileSizeHigh;

                // Get size of parent so far
                parentSize.LowPart = parentItem->findInfo.nFileSizeLow;
                parentSize.HighPart = parentItem->findInfo.nFileSizeHigh;

                // Add current size to parent size (64-bit addition)
                parentSize.QuadPart += currentSize.QuadPart;

                // Update parent file size
                parentItem->findInfo.nFileSizeLow = parentSize.LowPart;
                parentItem->findInfo.nFileSizeHigh = parentSize.HighPart;

                // Append current item to results list
                if ( AddItem( currentItem, resList ) == false )
                {
                    wprintf_s( TEXT( "Problem allocating memory\n" ) );
                    return FALSE;
                }
            }
        }

        // Reset current item
        memset( &currentItem, 0, sizeof( Item ) );

    } while ( FindNextFile( hFind, &currentItem.findInfo ) != 0 );

    // Validate end of search
    if ( GetLastError() != ERROR_NO_MORE_FILES )
    {
        ReportError( TEXT( "\nFindNextFile failed.\n" ), 0, TRUE );
        return FALSE;
    }

    // Close search handle
    FindClose( hFind );

    return TRUE;
}

// compare names of two items
//  >0 : item N before item M
//   0 : item N same place as item M
//  <0 : item N after item M
int cmpItemsName( Item* pItemN, Item* pItemM )
{
    int result;

    // case-insensitive comparison
    result = _wcsicmp( pItemM->findInfo.cFileName, pItemN->findInfo.cFileName );

    return result;
}

void showResults( List* resultsList, Item* resultsLevel )
{
    // Display header
    wprintf_s( TEXT( "    %19s %5s %10s %10s %18s %s\n" ),
        TEXT( "Date Modified" ),
        TEXT( "Type" ),
        TEXT( "Dirs" ),
        TEXT( "Files" ),
        TEXT( "Size" ),
        TEXT( "Name" ) );

    wprintf_s( TEXT( "    %19s %5s %10s %10s %18s %s\n" ),
        TEXT( "-------------------" ),
        TEXT( "-----" ),
        TEXT( "----------" ),
        TEXT( "----------" ),
        TEXT( "------------------" ),
        TEXT( "---------------------------------------------" ) );

    // Display founded entries
    Traverse( resultsList, showItem );

    // Display totals
    wprintf_s( TEXT( "    %19s %5s %10s %10s %18s %s\n" ),
        TEXT( "-------------------" ),
        TEXT( "-----" ),
        TEXT( "----------" ),
        TEXT( "----------" ),
        TEXT( "------------------" ),
        TEXT( "---------------------------------------------" ) );

    showItem( resultsLevel );
}

void showItem( Item* pItem )
{
    FILETIME lastWriteFTIME;
    SYSTEMTIME lastWriteSYSTIME;
    LARGE_INTEGER entrySize;
    TCHAR dirsCtStr[ 32 ] = { 0 };
    TCHAR filesCtStr[ 32 ] = { 0 };
    TCHAR sizeStr[ 32 ] = { 0 };

    // Fetch and prepare entry last modification date & time

    // UTC time (FILETIME) to local time (FILETIME)
    FileTimeToLocalFileTime( &pItem->findInfo.ftLastWriteTime,
        &lastWriteFTIME );

    // local time (FILETIME) to local time (SYSTIME)
    FileTimeToSystemTime( &lastWriteFTIME, &lastWriteSYSTIME );

    // Disp entry last modification date & time
    wprintf_s( TEXT( "    %04d-%02d-%02d %02d:%02d:%02d" ),
        lastWriteSYSTIME.wYear,
        lastWriteSYSTIME.wMonth,
        lastWriteSYSTIME.wDay,
        lastWriteSYSTIME.wHour,
        lastWriteSYSTIME.wMinute,
        lastWriteSYSTIME.wSecond );

    // Disp entry type (dir/file)
    if ( pItem->findInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
        wprintf_s( TEXT( " %5s" ), TEXT( "<LIN>" ) );
    else if ( pItem->findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        wprintf_s( TEXT( " %5s" ), TEXT( "<DIR>" ) );
    else
        wprintf_s( TEXT( " %5s" ), TEXT( "     " ) );

    // Fetch entry size
    entrySize.LowPart = pItem->findInfo.nFileSizeLow;
    entrySize.HighPart = pItem->findInfo.nFileSizeHigh;

    // Convert nums to strings (thousands separated)
    sepThousands( &pItem->dirsCount.QuadPart, dirsCtStr,
        _countof( dirsCtStr ) );
    sepThousands( &pItem->filesCount.QuadPart, filesCtStr,
        _countof( filesCtStr ) );
    sepThousands( &entrySize.QuadPart, sizeStr,
        _countof( sizeStr ) );

    // Disp entry details
    wprintf_s( TEXT( " %10s %10s %18s %s\n" ),
        dirsCtStr,
        filesCtStr,
        sizeStr,
        pItem->findInfo.cFileName );
}

void sepThousands( const long long* numPt, TCHAR* acc, size_t elemsAcc )
{
    static TCHAR app[ 32 ] = { 0 }; // append data (last three digits as chars)
    long long numIn = *numPt;       // remaining num
    long long res = 0;              // append data (last three digits as nums)    

    // Reset accumulated result (output str)
    wmemset( acc, 0, elemsAcc );

    // Iterate as long as there are digits left
    if ( numIn == 0 )
        wcscpy_s( acc, elemsAcc, TEXT( "0" ) );
    else
    {
        while ( numIn > 0 )
        {
            // Reset append storage
            wmemset( app, 0, _countof( app ) );

            // Get three right most digits
            res = numIn % 1000;

            // Get the remaining num
            numIn = numIn / 1000;

            // Build up string to be appended in front of accumulated result
            swprintf_s( app, _countof( app ),
                ( numIn > 0 ) ? TEXT( ",%03d" ) : TEXT( "%3d" ), res );

            // Append accumulated result to new digits
            wcscat_s( app, _countof( app ), acc );

            // Update accumulated result
            wcscpy_s( acc, elemsAcc, app );
        }
    }
}