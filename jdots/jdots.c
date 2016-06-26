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
#define     CMDBUF          256

// Flags indices
#define     FL_HELP         0   // Print usage

extern DWORD Options( int argc, LPCWSTR argv[], LPCWSTR OptStr, ... );
extern VOID ReportError( LPCTSTR userMsg, DWORD exitCode, BOOL prtErrorMsg );

BOOL scanDir( LPTSTR tDir, List* resList, Item* parentItem );
int cmpItemsName( Item* pItemN, Item* pItemM );
void showResults( List* resultsList, Item* resultsLevel );
void showItem( Item* pItem );
void sepThousands( const long long* numPt, TCHAR* acc, size_t elemsAcc );
BOOL procNmeaFile( TCHAR* fName, TCHAR* cdsOut, int cdsSize );
void outputKml( List* plist );
void addPtToKml( FILE* outKml, Item* pitem );
void addCoordsToKml( FILE* outKml, Item* pitem );


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
    TCHAR* ptTchar = NULL;

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

    // Initialize list's name (measurement name)
    ptTchar = wcsrchr( targetDir, L'\\' );

    if ( ptTchar != NULL )
        IniListName( &resultsList, ptTchar + 1 );
    else
        IniListName( &resultsList, TEXT( "" ) );

    // Check mem availability
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

        // Generate KML file
        outputKml( &resultsList );

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

                // Apply external tool hpos on current file
                if ( procNmeaFile( currentItem.findInfo.cFileName,
                    currentItem.coords, COORDS ) == FALSE )
                {
                    wprintf_s( TEXT( "Processing NMEA file failed\n" ) );
                    return FALSE;
                }

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
    wprintf_s( TEXT( "    %19s %47s %12s %s\n" ),
        TEXT( "Date Modified" ),
        TEXT( "Coords" ),
        TEXT( "Size" ),
        TEXT( "Name" ) );

    wprintf_s( TEXT( "    %19s %47s %12s %s\n" ),
        TEXT( "-------------------" ),
        TEXT( "-----------------------------------------------" ),
        TEXT( "------------" ),
        TEXT( "--------------------------------" ) );

    // Display founded entries
    Traverse( resultsList, showItem );

    // Display totals
    wprintf_s( TEXT( "    %19s %47s %12s %s\n" ),
        TEXT( "-------------------" ),
        TEXT( "-----------------------------------------------" ),
        TEXT( "------------" ),
        TEXT( "--------------------------------" ) );

    showItem( resultsLevel );
}

void showItem( Item* pItem )
{
    FILETIME lastWriteFTIME;
    SYSTEMTIME lastWriteSYSTIME;
    LARGE_INTEGER entrySize;
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

    // Disp coords
    wprintf_s( TEXT( " %47s" ), pItem->coords );

    // Fetch entry size
    entrySize.LowPart = pItem->findInfo.nFileSizeLow;
    entrySize.HighPart = pItem->findInfo.nFileSizeHigh;

    // Convert nums to strings (thousands separated)
    sepThousands( &entrySize.QuadPart, sizeStr,
        _countof( sizeStr ) );

    // Disp entry details
    wprintf_s( TEXT( "%13s %s\n" ),
        sizeStr,
        pItem->findInfo.cFileName );
}

void addPtToKml( FILE* outKml, Item* pitem )
{
    TCHAR* ptTchar = NULL;
    TCHAR ptName[ MAX_PATH ] = { 0 };

    // Set up point's name
    wcscpy_s( ptName, _countof( ptName ), pitem->findInfo.cFileName );
    ptTchar = wcsrchr( ptName, L'.' );
    if ( ptTchar != NULL )
        *ptTchar = L'\0';

    // Output point placemark
    fwprintf_s( outKml, TEXT( "<Placemark>\n" ) );
    fwprintf_s( outKml, TEXT( "  <name>%s</name>\n" ),
        ptName );
    fwprintf_s( outKml, TEXT( "  <styleUrl>#mypushpin</styleUrl>\n" ) );
    fwprintf_s( outKml, TEXT( "  <Point>\n" ) );
    fwprintf_s( outKml, TEXT( "    <coordinates>%s</coordinates>\n" ),
        pitem->coords );
    fwprintf_s( outKml, TEXT( "  </Point>\n" ) );
    fwprintf_s( outKml, TEXT( "</Placemark>\n\n" ) );
}

void addCoordsToKml( FILE* outKml, Item* pitem )
{
    fwprintf_s( outKml, TEXT( "      %s\n" ), pitem->coords );
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

BOOL procNmeaFile( TCHAR* fName, TCHAR* cdsOut, int cdsSize )
{
    TCHAR cmdBuffer[ CMDBUF ] = { 0 };
    FILE* pPipe = NULL;
    BOOL result = TRUE;

    // Set up external command
    swprintf_s( cmdBuffer, _countof( cmdBuffer ), TEXT( "%s %s" ),
        TEXT( "C:\\tmp\\myTools\\hpos.exe" ),
        fName );

    // Create pipe (read text mode) and execute external program
    if ( ( pPipe = _wpopen( cmdBuffer, TEXT( "rt" ) ) ) == NULL )
        return FALSE;

    // Reset output buffer
    wmemset( cdsOut, 0, cdsSize );

    // Read pipe until end of file, or an error occurs.
    // hpos outputs only one line of text
    while ( fgetws( cdsOut, cdsSize, pPipe ) )
    {
        ;
    }

    // Close pipe and print return value of pPipe.
    if ( feof( pPipe ) )
        _pclose( pPipe );
    else
    {
        wprintf_s( TEXT( "Failed to read pipe for 'hpos' to the end.\n" ) );
        result = FALSE;
    }

    return result;
}

void outputKml( List* plist )
{
    errno_t err;
    FILE* outKml = NULL;
    TCHAR fName[ MAX_PATH ] = { 0 };

    // Set up kml file's name
    wcscpy_s( fName, _countof( fName ), plist->measureName );
    wcscat_s( fName, _countof( fName ), TEXT( ".kml" ) );

    // Set up 'lati' output file
    err = _wfopen_s( &outKml, fName, TEXT( "w" ) );

    if ( err != 0 )
    {                       
        fwprintf_s( stderr, TEXT( "Can't create output file.\n" ) );
        return;
    }

    // Output header
    fwprintf_s( outKml,
        TEXT( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ) );
    fwprintf_s( outKml,
        TEXT( "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n\n" ) );
    fwprintf_s( outKml, TEXT( "<Document>\n\n" ) );

    // Output styles
    fwprintf_s( outKml, TEXT( "<Style id=\"mypushpin\">\n" ) );
    fwprintf_s( outKml, TEXT( "  <LabelStyle>\n" ) );
    fwprintf_s( outKml, TEXT( "    <scale>0.5</scale>\n" ) );
    fwprintf_s( outKml, TEXT( "  </LabelStyle>\n" ) );
    fwprintf_s( outKml, TEXT( "  <IconStyle>\n" ) );
    fwprintf_s( outKml, TEXT( "    <scale>0.5</scale>\n" ) );
    fwprintf_s( outKml, TEXT( "    <Icon>\n" ) );
    fwprintf_s( outKml, TEXT( "      <href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href>\n" ) );
    fwprintf_s( outKml, TEXT( "    </Icon>\n" ) );
    fwprintf_s( outKml, TEXT( "  </IconStyle>\n" ) );
    fwprintf_s( outKml, TEXT( "</Style>\n\n" ) );

    fwprintf_s( outKml, TEXT( "<Style id=\"myline\">\n" ) );
    fwprintf_s( outKml, TEXT( "  <LineStyle>\n" ) );
    fwprintf_s( outKml, TEXT( "    <color>ff7fff55</color>\n" ) );
    fwprintf_s( outKml, TEXT( "    <colorMode>normal</colorMode>\n" ) );
    fwprintf_s( outKml, TEXT( "    <width>3</width>\n" ) );
    fwprintf_s( outKml, TEXT( "  </LineStyle>\n" ) );
    fwprintf_s( outKml, TEXT( "</Style>\n\n" ) );

    // Output measurement points (placemarks) to kml file
    TraverseToFile( plist, outKml, addPtToKml );

    // Output polygon joining measurement points
    fwprintf_s( outKml, TEXT( "<Placemark>\n" ) );
    fwprintf_s( outKml, TEXT( "  <name>%s</name>\n" ), plist->measureName );
    fwprintf_s( outKml, TEXT( "  <styleUrl>#myline</styleUrl>\n" ) );
    fwprintf_s( outKml, TEXT( "  <LineString>\n" ) );
    fwprintf_s( outKml, TEXT( "    <coordinates>\n" ) );

    // Add coods of all pts to kml
    TraverseToFile( plist, outKml, addCoordsToKml );

    // If needed, then close the polygon
    // Output coords of first point again
    if ( plist->iCount > 2 )
        fwprintf_s( outKml, TEXT( "      %s\n" ), plist->head->item.coords );

    // Terminate polygon output
    fwprintf_s( outKml, TEXT( "    </coordinates>\n" ) );
    fwprintf_s( outKml, TEXT( "  </LineString>\n" ) );
    fwprintf_s( outKml, TEXT( "</Placemark>\n\n" ) );

    // Output footer
    fwprintf_s( outKml, TEXT( "</Document>\n\n" ) );
    fwprintf_s( outKml, TEXT( "</kml>" ) );

    // Close kml file
    if ( fclose( outKml ) != 0 )
        fwprintf_s( stderr, TEXT( "Error closing kml file\n" ) );
}
