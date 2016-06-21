//
//  hpos.c
//

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include "tree.h"

#define     ERRMSG      256
#define     FNAME       260
#define     LINEIN      128
#define     LINEOUT     256
#define     VALIN       32

extern VOID ReportError( LPCTSTR userMsg, DWORD exitCode, BOOL prtErrorMsg );

void addLat( char* hemis, char* valStr, Tree* pt );
void addLon( char* hemis, char* valStr, Tree* pt );
void addAlt( char* valStr, Tree* pt );
void fillWtVals( Tree* pt );
void fillWtValItem( Item* itemPt, int wt, HANDLE hOut );
double calcWtTotVal( Tree* pt );
double fetchWtTotVal( Tree* pt );
void showValsScreen( Tree* pt, HANDLE hOut );
void showValsCSV( Tree* pt, HANDLE hOut );
void printItemScr( Item* itemPt, int ctTot, HANDLE hOut );
void printItemCSV( Item* itemPt, int ctTot, HANDLE hOut );
void outBasic( Tree* ptTrLon, Tree* ptTrLat, Tree* ptTrAlt );
void outDetail( Tree* ptTrLon, Tree* ptTrLat, Tree* ptTrAlt );
void outCVS( Tree* ptTrLon, Tree* ptTrLat, Tree* ptTrAlt, TCHAR* fName );
void outKML( Tree* ptTrLon, Tree* ptTrLat, Tree* ptTrAlt, TCHAR* fName );
int txtToFile( CHAR* txtInPt, DWORD sizeBuf, HANDLE hOut );

int wmain( int argc, TCHAR* argv[] )
{
    //==============================================
    // Vars definitions
    //==============================================
    FILE *inNMEA = NULL;
    TCHAR errMsg[ ERRMSG ] = { 0 };
    char inputLine[ LINEIN ] = { 0 };
    char *ptMsg = NULL;
    char *nextptMsg = NULL;
    int fieldNo = 0;
    TCHAR* wchPt = NULL;
    TCHAR fileName[ FNAME ] = { 0 };

    BOOL dataOk = FALSE;
    char status[ VALIN ] = { 0 };
    char hemiNS[ VALIN ] = { 0 };
    char hemiEW[ VALIN ] = { 0 };
    char tmpLat[ VALIN ] = { 0 };
    char tmpLon[ VALIN ] = { 0 };
    char tmpAlt[ VALIN ] = { 0 };

    Tree latTree;
    Tree lonTree;
    Tree altTree;


    //==============================================
    // Parse arguments and options
    //==============================================
    
    // Validate args count
    if ( argc != 2 )
    {
        // Print usage
        wprintf_s( TEXT( "\n    Usage:  hpos [nmea file]\n" ) );
        return 1;
    }

    // Open nmea file
    if ( _wfopen_s( &inNMEA, argv[ 1 ], TEXT( "r" ) ) != 0 )
    {
        wmemset( errMsg, 0, _countof( errMsg ) );
        __wcserror_s( errMsg, _countof( errMsg ),
            TEXT( "\nOpening source file failed" ) );
        fwprintf( stderr, TEXT( "%s\n" ), errMsg );
        return 1;
    }

    // Retrieve file name
    wcscpy_s( fileName, _countof( fileName ), argv[ 1 ] );
    wchPt = wcsrchr( fileName, L'.' );
    *wchPt = L'\0';


    //==============================================
    // Initialize storage trees
    //==============================================
    InitializeTree( &latTree );
    InitializeTree( &lonTree );
    InitializeTree( &altTree );


    //==============================================
    // Parse nmea file
    //==============================================
    
    // Reset input line buffer
    memset( inputLine, 0, _countof( inputLine ) );

    // Fetch one line at a time
    while ( fscanf_s( inNMEA, "%127s", inputLine,
        _countof( inputLine ) - 1 ) == 1 )
    {
        if ( strstr( inputLine, "$GPGGA" ) )        // Catch 'GGA' messages
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
                    strcpy_s( tmpLat, _countof( tmpLat ), ptMsg );
                else if ( fieldNo == 3 )
                    strcpy_s( hemiNS, _countof( hemiNS ), ptMsg );
                else if ( fieldNo == 4 )
                    strcpy_s( tmpLon, _countof( tmpLon ), ptMsg );
                else if ( fieldNo == 5 )
                    strcpy_s( hemiEW, _countof( hemiEW ), ptMsg );
                else if ( fieldNo == 9 )
                {
                    strcpy_s( tmpAlt, _countof( tmpAlt ), ptMsg );
                    break;      // Skip the rest of the fields
                }

                // Locate next field (token)
                ptMsg = strtok_s( NULL, ",*", &nextptMsg );

                // Inc field counter
                ++fieldNo;
            }
        }
        else if ( strstr( inputLine, "$GPRMC" ) )   // Catch 'RMC' messages
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
                {
                    strcpy_s( status, _countof( status ), ptMsg );
                    break;      // Skip the rest of the fields
                }

                // Locate next field (token)
                ptMsg = strtok_s( NULL, ",*", &nextptMsg );

                // Inc field counter
                ++fieldNo;
            }

            // Validate processed line
            dataOk = ( strlen( status ) == 1 ) && ( strstr( status, "A" ) ) &&
                ( strlen( tmpLat ) == 9 ) && ( strlen( tmpLon ) == 10 );

            if ( dataOk )
            {
                // Store current vals into trees
                addLat( hemiNS, tmpLat, &latTree );
                addLon( hemiEW, tmpLon, &lonTree );
                addAlt( tmpAlt, &altTree );
            }

            // Reset result strings
            dataOk = FALSE;
            memset( status, 0, _countof( status ) );
            memset( hemiNS, 0, _countof( hemiNS ) );
            memset( hemiEW, 0, _countof( hemiEW ) );
            memset( tmpLat, 0, _countof( tmpLat ) );
            memset( tmpLon, 0, _countof( tmpLon ) );
            memset( tmpAlt, 0, _countof( tmpAlt ) );
        }

        // Reset input line buffer
        memset( inputLine, 0, _countof( inputLine ) );
    }

    
    //==============================================
    // Close nmea file
    //==============================================
    if ( fclose( inNMEA ) != 0 )
    {
        wmemset( errMsg, 0, _countof( errMsg ) );
        __wcserror_s( errMsg, _countof( errMsg ),
            TEXT( "\nClosing source file failed" ) );
        fwprintf( stderr, TEXT( "%s\n" ), errMsg );
        return 1;
    }


    //==============================================
    // Calculate weighted results
    // This can only be done after all measurements
    // were processed, so that the amount of
    // measurements per value (node) is known
    //==============================================
    
    // Calc weighted value per node
    fillWtVals( &latTree );
    fillWtVals( &lonTree );
    fillWtVals( &altTree );

    // Calc total accumulated weighted value
    calcWtTotVal( &latTree );
    calcWtTotVal( &lonTree );
    calcWtTotVal( &altTree );


    //==============================================
    // Output results
    //==============================================

    // Output basic data to screen
    // (useful for batch processing)
    // Option: -b
    outBasic( &lonTree, &latTree, &altTree );

    // Output detailed data to screen
    // Option: -d
    outDetail( &lonTree, &latTree, &altTree );
    
    // Output detailed data to CSV file
    // Option: -c
    outCVS( &lonTree, &latTree, &altTree, fileName );

    // Output placemark to KML file
    // Option: -k
    outKML( &lonTree, &latTree, &altTree, fileName );


    //==============================================
    // Destroy storage trees
    //==============================================
    DeleteAll( &latTree );
    DeleteAll( &lonTree );
    DeleteAll( &altTree );

    return 0;
}

void addLat( char* hemis, char* valStr, Tree* pt )
{
    Item tmpItem = { 0 };
    char tmpStr[ VALIN ] = { 0 };
    int intVal = 0;

    if ( TreeIsFull( pt ) )
        puts( "Storage tree is full." );
    else
    {
        // Set up new item
        
        // Set up nmea value
        memset( tmpItem.nmeaVal, 0, _countof( tmpItem.nmeaVal ) );
        
        // Sign
        if ( strstr( hemis, "S" ) )
            tmpItem.nmeaVal[ 0 ] = '-';

        // Store val in item
        strcat_s( tmpItem.nmeaVal, _countof( tmpItem.nmeaVal ), valStr );

        // Set up int val

        // Degs
        memset( tmpStr, 0, _countof( tmpStr ) );
        strncpy_s( tmpStr, _countof( tmpStr ), valStr, 2 );
        intVal = atoi( tmpStr ) * 3600000;

        // Mins
        memset( tmpStr, 0, _countof( tmpStr ) );
        strncpy_s( tmpStr, _countof( tmpStr ), valStr + 2, 2 );
        intVal += atoi( tmpStr ) * 60000;

        // Fractions of mins
        intVal += atoi( valStr + 5 ) * 6;

        // Sign
        if ( strstr( hemis, "S" ) )
            intVal *= ( -1 );

        // Store val in item
        tmpItem.intVal = intVal;

        // Set up double val [deg]
        tmpItem.dblVal = ( double )intVal / 3600000;

        // Set up pts counter
        tmpItem.ct = 1;
        
        // Add new item to the tree
        AddItem( &tmpItem, pt );
    }
}

void addLon( char* hemis, char* valStr, Tree* pt )
{
    Item tmpItem = { 0 };
    char tmpStr[ VALIN ] = { 0 };
    int intVal = 0;

    if ( TreeIsFull( pt ) )
        puts( "Storage tree is full." );
    else
    {
        // Set up new item

        // Set up nmea value
        memset( tmpItem.nmeaVal, 0, _countof( tmpItem.nmeaVal ) );

        // Sign
        if ( strstr( hemis, "W" ) )
            tmpItem.nmeaVal[ 0 ] = '-';
        
        // Store val in item
        strcat_s( tmpItem.nmeaVal, _countof( tmpItem.nmeaVal ), valStr );

        // Set up int val

        // Degs
        memset( tmpStr, 0, _countof( tmpStr ) );
        strncpy_s( tmpStr, _countof( tmpStr ), valStr, 3 );
        intVal = atoi( tmpStr ) * 3600000;

        // Mins
        memset( tmpStr, 0, _countof( tmpStr ) );
        strncpy_s( tmpStr, _countof( tmpStr ), valStr + 3, 2 );
        intVal += atoi( tmpStr ) * 60000;

        // Fractions of mins
        intVal += atoi( valStr + 6 ) * 6;

        // Sign
        if ( strstr( hemis, "W" ) )
            intVal *= ( -1 );

        // Store val in item
        tmpItem.intVal = intVal;

        // Set up double val [deg]
        tmpItem.dblVal = ( double )intVal / 3600000;

        // Set up pts counter
        tmpItem.ct = 1;
        
        // Add new item to the tree
        AddItem( &tmpItem, pt );
    }
}

void addAlt( char* valStr, Tree* pt )
{
    Item tmpItem = { 0 };
    char* chPt = NULL;

    if ( TreeIsFull( pt ) )
        puts( "Storage tree is full." );
    else
    {
        // Set up new item

        // Set up nmea value
        memset( tmpItem.nmeaVal, 0, _countof( tmpItem.nmeaVal ) );
        strcpy_s( tmpItem.nmeaVal, _countof( tmpItem.nmeaVal ), valStr );

        // Set up int val
        
        // Metres -> decimetres
        tmpItem.intVal = atoi( valStr ) * 10;

        // Decimetres
        chPt = strchr( valStr, '.' );
        if ( chPt )
            tmpItem.intVal += ( valStr[ 0 ] == '-' ? (-1) : 1 ) *
                atoi( chPt + 1 );

        // Set up double val [metres]
        tmpItem.dblVal = ( double )tmpItem.intVal / 10;

        // Set up pts counter
        tmpItem.ct = 1;
        
        // Add new item to the tree
        AddItem( &tmpItem, pt );
    }
}

void showValsScreen( Tree* pt, HANDLE hOut )
{
    if ( TreeIsEmpty( pt ) )
        puts( "No entries!" );
    else
        Traverse( pt, printItemScr, hOut );
}

void showValsCSV( Tree* pt, HANDLE hOut )
{
    if ( TreeIsEmpty( pt ) )
        puts( "No entries!" );
    else
        Traverse( pt, printItemCSV, hOut );
}

void fillWtVals( Tree* pt )
{
    if ( !( TreeIsEmpty( pt ) ) )
        Traverse( pt, fillWtValItem, NULL );
}

void printItemScr( Item* itemPt, int ctTot, HANDLE hOut )
{
    CHAR bufOut[ LINEOUT ] = { 0 };

    // Print values's details
    sprintf_s( bufOut, _countof( bufOut ),
        "    %12s  %13d  %14.8f  %6d  %6d  %14.8f\n",
        itemPt->nmeaVal, itemPt->intVal, itemPt->dblVal,
        itemPt->ct, ctTot, itemPt->wtVal );

    txtToFile( bufOut, _countof( bufOut ), hOut );
}

void printItemCSV( Item* itemPt, int ctTot, HANDLE hOut )
{
    CHAR bufOut[ LINEOUT ] = { 0 };

    // Print values's details to CSV file
    sprintf_s( bufOut, _countof( bufOut ),
        "%s,%d,%.8f,%d,%d,%.8f\n",
        itemPt->nmeaVal, itemPt->intVal, itemPt->dblVal, itemPt->ct,
        ctTot, itemPt->wtVal );

    txtToFile( bufOut, _countof( bufOut ), hOut );
}

void fillWtValItem( Item* itemPt, int wt, HANDLE hOut )
{
    // Fill in the item's weighted value
    itemPt->wtVal = itemPt->dblVal * ( double )itemPt->ct / wt;
}

double calcWtTotVal( Tree* pt )
{
    if ( !( TreeIsEmpty( pt ) ) )
        return TraverseWtVal( pt );
    else
        return 0;
}

double fetchWtTotVal( Tree* pt )
{
    if ( !( TreeIsEmpty( pt ) ) )
        return pt->wtTotVal;
    else
        return 0;
}

void outBasic( Tree* ptTrLon, Tree* ptTrLat, Tree* ptTrAlt )
{
    wprintf_s( TEXT( "%.8f,%.8f,%.8f\n" ),
        fetchWtTotVal( ptTrLon ),
        fetchWtTotVal( ptTrLat ),
        fetchWtTotVal( ptTrAlt ) );
}

void outDetail( Tree* ptTrLon, Tree* ptTrLat, Tree* ptTrAlt )
{
    wprintf_s( TEXT( "\n" ) );

    // Display lon values
    wprintf_s( TEXT( "    %12s  %13s  %14s  %6s  %6s  %14s\n" ),
        TEXT( "Lon" ), TEXT( "[ms]" ), TEXT( "[deg]" ),
        TEXT( "ct" ), TEXT( "ctTot" ), TEXT( "[deg]" ) );
    showValsScreen( ptTrLon, GetStdHandle( STD_OUTPUT_HANDLE ) );
    wprintf_s( TEXT( "%79.8f\n\n" ), fetchWtTotVal( ptTrLon ) );

    // Display lat values
    wprintf_s( TEXT( "    %12s  %13s  %14s  %6s  %6s  %14s\n" ),
        TEXT( "Lat" ), TEXT( "[ms]" ), TEXT( "[deg]" ),
        TEXT( "ct" ), TEXT( "ctTot" ), TEXT( "[deg]" ) );
    showValsScreen( ptTrLat, GetStdHandle( STD_OUTPUT_HANDLE ) );
    wprintf_s( TEXT( "%79.8f\n\n" ), fetchWtTotVal( ptTrLat ) );

    // Display alt values
    wprintf_s( TEXT( "    %12s  %13s  %14s  %6s  %6s  %14s\n" ),
        TEXT( "Alt" ), TEXT( "[dm]" ), TEXT( "[m]" ),
        TEXT( "ct" ), TEXT( "ctTot" ), TEXT( "[m]" ) );
    showValsScreen( ptTrAlt, GetStdHandle( STD_OUTPUT_HANDLE ) );
    wprintf_s( TEXT( "%79.8f\n\n\n" ), fetchWtTotVal( ptTrAlt ) );
}

void outCVS( Tree* ptTrLon, Tree* ptTrLat, Tree* ptTrAlt, TCHAR* fName )
{
    HANDLE hFileOut;
    TCHAR fNameTot[ FNAME ] = { 0 };
    CHAR bufOut[ LINEOUT ] = { 0 };

    // Set up complete file name (name + ext)
    wcscpy_s( fNameTot, _countof( fNameTot ), fName );
    wcscat_s( fNameTot, _countof( fNameTot ), TEXT( ".csv" ) );

    // Open output file
    hFileOut = CreateFile( fNameTot, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL );

    // Validate output handle
    if ( hFileOut == INVALID_HANDLE_VALUE )
    {
        ReportError( TEXT( "Open output file failed." ), 0, TRUE );
        return;
    }

    // Display Longitudes
    sprintf_s( bufOut, _countof( bufOut ), "%s",
        "Lon,[ms],[deg],ct,ctTot,[deg]\n" );
    txtToFile( bufOut, _countof( bufOut ), hFileOut );

    showValsCSV( ptTrLon, hFileOut );

    sprintf_s( bufOut, _countof( bufOut ), ",,,,,%.8f\n\n",
        fetchWtTotVal( ptTrLon ) );
    txtToFile( bufOut, _countof( bufOut ), hFileOut );
    
    // Display Latitudes
    sprintf_s( bufOut, _countof( bufOut ), "%s",
        "Lat,[ms],[deg],ct,ctTot,[deg]\n" );
    txtToFile( bufOut, _countof( bufOut ), hFileOut );

    showValsCSV( ptTrLat, hFileOut );

    sprintf_s( bufOut, _countof( bufOut ), ",,,,,%.8f\n\n",
        fetchWtTotVal( ptTrLat ) );
    txtToFile( bufOut, _countof( bufOut ), hFileOut );

    // Display Altitudes
    sprintf_s( bufOut, _countof( bufOut ), "%s",
        "Alt,[dm],[m],ct,ctTot,[m]\n" );
    txtToFile( bufOut, _countof( bufOut ), hFileOut );

    showValsCSV( ptTrAlt, hFileOut );

    sprintf_s( bufOut, _countof( bufOut ), ",,,,,%.8f\n\n",
        fetchWtTotVal( ptTrAlt ) );
    txtToFile( bufOut, _countof( bufOut ), hFileOut );

    // Close handle
    CloseHandle( hFileOut );
}

void outKML( Tree* ptTrLon, Tree* ptTrLat, Tree* ptTrAlt, TCHAR* fName )
{
    wprintf_s( TEXT( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ) );
    wprintf_s( TEXT( "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n" ) );
    wprintf_s( TEXT( "<Document>\n" ) );
    wprintf_s( TEXT( "<Placemark>\n" ) );
    wprintf_s( TEXT( "    <name>%s</name>\n" ), fName );
    wprintf_s( TEXT( "    <description>%s</description>\n" ), fName );
    wprintf_s( TEXT( "    <Point>\n" ) );
    wprintf_s( TEXT( "        <coordinates>%.8f,%.8f,%.8f</coordinates>\n" ),
        fetchWtTotVal( ptTrLon ),
        fetchWtTotVal( ptTrLat ),
        fetchWtTotVal( ptTrAlt ) );
    wprintf_s( TEXT( "    </Point>\n" ) );
    wprintf_s( TEXT( "</Placemark>\n" ) );
    wprintf_s( TEXT( "</Document>\n" ) );
    wprintf_s( TEXT( "</kml>\n\n" ) );
}

int txtToFile( CHAR* txtInPt, DWORD sizeBuf, HANDLE hOut )
{
    DWORD txtLen, nOut;

    // Calc amount of bytes to output
    txtLen = strnlen( txtInPt, sizeBuf );
    
    // Validate text buffer null termination
    if ( txtLen >= sizeBuf )
    {
        // String not null terminated -> terminate output
        fprintf( stderr, "String length error.\n" );
        return 1;
    }

    // Write text to file
    WriteFile( hOut, txtInPt, txtLen, &nOut, NULL );

    // Validate bytes 'to output' vs 'output' bytes
    if ( txtLen != nOut )
    {
        ReportError( TEXT( "Output to file failed." ), 0, TRUE );
        return 1;
    }

    return 0;
}
