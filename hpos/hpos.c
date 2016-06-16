//
//  hpos.c
//

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include "tree.h"

#define     ERRMSG      256
#define     LINEIN      128

extern VOID ReportError( LPCTSTR userMsg, DWORD exitCode, BOOL prtErrorMsg );

void addVal( char* valStr, Tree* pt );
void showVals( const Tree* pt );
void printitem( Item item );

int wmain( int argc, TCHAR* argv[] )
{
    // Vars definitions
    FILE *inNMEA = NULL;
    TCHAR errMsg[ ERRMSG ] = { 0 };
    char inputLine[ LINEIN ] = { 0 };
    char *ptMsg = NULL;
    char *nextptMsg = NULL;
    int fieldNo = 0;

    Tree latTree;
    Tree lonTree;
    Tree altTree;

    // Validate args count
    if ( argc != 2 )
    {
        // Print usage
        wprintf_s( TEXT( "\n    Usage:  hpos [nmea file]\n" ) );
        return 1;
    }

    // Open 'NMEA' input file
    if ( _wfopen_s( &inNMEA, argv[ 1 ], TEXT( "r" ) ) != 0 )
    {
        wmemset( errMsg, 0, _countof( errMsg ) );
        __wcserror_s( errMsg, _countof( errMsg ),
            TEXT( "\nOpening source file failed" ) );
        fwprintf( stderr, TEXT( "%s\n" ), errMsg );
        return 1;
    }

    // Initialize storage trees
    InitializeTree( &latTree );
    InitializeTree( &lonTree );
    InitializeTree( &altTree );

    // Reset input line buffer
    memset( inputLine, 0, _countof( inputLine ) );

    // Fetch one line at a time
    while ( fscanf_s( inNMEA, "%127s", inputLine,
        _countof( inputLine ) - 1 ) == 1 )
    {
        // Process only 'GGA' messages
        if ( strstr( inputLine, "$GPGGA" ) )
        {
            // Reset field counter
            fieldNo = 0;

            // Locate first field (token)
            ptMsg = strtok_s( inputLine, ",*", &nextptMsg );

            while ( ptMsg != NULL )
            {
                // Process fields of interest
                // Store value in tree
                // If already in tree, then update counters
                if ( fieldNo == 2 )
                    addVal( ptMsg, &latTree );
                else if ( fieldNo == 4 )
                    addVal( ptMsg, &lonTree );
                else if ( fieldNo == 9 )
                    addVal( ptMsg, &altTree );

                // Locate next field (token)
                ptMsg = strtok_s( NULL, ",*", &nextptMsg );

                // Inc field counter
                ++fieldNo;
            }
        }

        // Reset input line buffer
        memset( inputLine, 0, _countof( inputLine ) );
    }

    // Close input nmea file
    if ( fclose( inNMEA ) != 0 )
    {
        wmemset( errMsg, 0, _countof( errMsg ) );
        __wcserror_s( errMsg, _countof( errMsg ),
            TEXT( "\nClosing source file failed" ) );
        fwprintf( stderr, TEXT( "%s\n" ), errMsg );
        return 1;
    }

    // Display Latitudes
    wprintf_s( TEXT( "\n    Lat:\n\n" ) );
    showVals( &latTree );

    // Display Longitudes
    wprintf_s( TEXT( "\n\n    Lon:\n\n" ) );
    showVals( &lonTree );

    // Display Altitudes
    wprintf_s( TEXT( "\n\n    Alt:\n\n" ) );
    showVals( &altTree );

    // Destroy storage trees
    DeleteAll( &latTree );
    DeleteAll( &lonTree );
    DeleteAll( &altTree );

    return 0;
}

void addVal( char* valStr, Tree* pt )
{
    int i = 0;
    Item temp = { 0 };

    if ( TreeIsFull( pt ) )
        puts( "Storage tree is full." );
    else
    {
        // Set up new item
        sprintf_s( temp.val, _countof( temp.val ), "%10s", valStr );
        temp.count = 1;
        
        // Add new item to the tree
        AddItem( &temp, pt );
    }
}

void showVals( const Tree* pt )
{
    if ( TreeIsEmpty( pt ) )
        puts( "No entries!" );
    else
        Traverse( pt, printitem );
}

void printitem( Item item )
{
    // Print values's details
    printf( "    %s,%d\n", item.val, item.count );
}