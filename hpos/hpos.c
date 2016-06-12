//
//  hpos.c
//

#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#define     ERRMSG      256
#define     LINEIN      128

extern VOID ReportError( LPCTSTR userMsg, DWORD exitCode, BOOL prtErrorMsg );

int wmain( int argc, TCHAR* argv[] )
{
    // Vars definitions
    FILE *inNMEA = NULL;
    TCHAR errMsg[ ERRMSG ] = { 0 };
    char inputLine[ LINEIN ] = { 0 };
    char *ptMsg = NULL;
    char *nextptMsg = NULL;
    int fieldNo = 0;

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
                if ( fieldNo == 2 )
                    printf( "Lat: %s ", ptMsg );
                else if ( fieldNo == 4 )
                    printf( "Lon: %s ", ptMsg );
                else if ( fieldNo == 9 )
                    printf( "Alt: %s\n", ptMsg );

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

    return 0;
}