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
#define     VALIN       32

extern VOID ReportError( LPCTSTR userMsg, DWORD exitCode, BOOL prtErrorMsg );

void addLat( char* hemis, char* valStr, Tree* pt );
void addLon( char* hemis, char* valStr, Tree* pt );
void addAlt( char* valStr, Tree* pt );
void showVals( Tree* pt );
void fillWtVals( Tree* pt );
void printItem( Item* item, int ctTot );
void fillWtValItem( Item* item, int wt );
double getWtTotVal( Tree* pt );

int wmain( int argc, TCHAR* argv[] )
{
    // Vars definitions
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

    double weightedTotLat = 0;
    double weightedTotLon = 0;
    double weightedTotAlt = 0;

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

    // Retrieve file name
    wchPt = wcsrchr( argv[ 1 ], L'\\' );

    if ( wchPt != NULL )
        wcscpy_s( fileName, _countof( fileName ), wchPt + 1 );
    else
        wcscpy_s( fileName, _countof( fileName ), argv[ 1 ] );

    wchPt = wcschr( fileName, L'.' );
    *wchPt = L'\0';

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
                    strcpy_s( tmpLat, _countof( tmpLat ), ptMsg );
                else if ( fieldNo == 3 )
                    strcpy_s( hemiNS, _countof( hemiNS ), ptMsg );
                else if ( fieldNo == 4 )
                    strcpy_s( tmpLon, _countof( tmpLon ), ptMsg );
                else if ( fieldNo == 5 )
                    strcpy_s( hemiEW, _countof( hemiEW ), ptMsg );
                else if ( fieldNo == 9 )
                    strcpy_s( tmpAlt, _countof( tmpAlt ), ptMsg );

                // Locate next field (token)
                ptMsg = strtok_s( NULL, ",*", &nextptMsg );

                // Inc field counter
                ++fieldNo;
            }
        }
        else if ( strstr( inputLine, "$GPRMC" ) )
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
                    strcpy_s( status, _countof( status ), ptMsg );

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

    // Close input nmea file
    if ( fclose( inNMEA ) != 0 )
    {
        wmemset( errMsg, 0, _countof( errMsg ) );
        __wcserror_s( errMsg, _countof( errMsg ),
            TEXT( "\nClosing source file failed" ) );
        fwprintf( stderr, TEXT( "%s\n" ), errMsg );
        return 1;
    }

    // Calculate the weighted values
    // of each measurement (node)
    fillWtVals( &latTree );
    fillWtVals( &lonTree );
    fillWtVals( &altTree );

    // Calculate accumulated weighted value
    weightedTotLat = getWtTotVal( &latTree );
    weightedTotLon = getWtTotVal( &lonTree );
    weightedTotAlt = getWtTotVal( &altTree );

    // Display Latitudes
    wprintf_s( TEXT( "Lat,[ms],[deg],ct,ctTot,[deg]\n" ) );
    showVals( &latTree );
    wprintf_s( TEXT( ",,,,,%.8f\n\n" ), weightedTotLat );

    // Display Longitudes
    wprintf_s( TEXT( "Lon,[ms],[deg],ct,ctTot,[deg]\n" ) );
    showVals( &lonTree );
    wprintf_s( TEXT( ",,,,,%.8f\n\n" ), weightedTotLon );

    // Display Altitudes
    wprintf_s( TEXT( "Alt,[dm],[m],ct,ctTot,[m]\n" ) );
    showVals( &altTree );
    wprintf_s( TEXT( ",,,,,%.1f\n\n" ), weightedTotAlt );

    // Generate .kml output
    wprintf_s( TEXT( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ) );
    wprintf_s( TEXT( "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n" ) );
    wprintf_s( TEXT( "<Document>\n" ) );
    wprintf_s( TEXT( "<Placemark>\n" ) );
    wprintf_s( TEXT( "    <name>%s</name>\n" ), fileName );
    wprintf_s( TEXT( "    <description>%s</description>\n" ), fileName );
    wprintf_s( TEXT( "    <Point>\n" ) );
    wprintf_s( TEXT( "        <coordinates>%.8f,%.8f,%.1f</coordinates>\n" ),
        weightedTotLon, weightedTotLat, weightedTotAlt );
    wprintf_s( TEXT( "    </Point>\n" ) );
    wprintf_s( TEXT( "</Placemark>\n" ) );
    wprintf_s( TEXT( "</Document>\n" ) );
    wprintf_s( TEXT( "</kml>\n\n" ) );

    // Destroy storage trees
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

void showVals( Tree* pt )
{
    if ( TreeIsEmpty( pt ) )
        puts( "No entries!" );
    else
        Traverse( pt, printItem );
}

void fillWtVals( Tree* pt )
{
    if ( !( TreeIsEmpty( pt ) ) )
        Traverse( pt, fillWtValItem );
}

void printItem( Item* itemPt, int ctTot )
{
    // Print values's details
    printf( "%s,%d,%.8f,%d,%d,%.8f\n",
        itemPt->nmeaVal, itemPt->intVal, itemPt->dblVal, itemPt->ct,
        ctTot, itemPt->wtVal );
}

void fillWtValItem( Item* itemPt, int wt )
{
    // Fill in the item's weighted value
    itemPt->wtVal = itemPt->dblVal * ( double )itemPt->ct / wt;
}

double getWtTotVal( Tree* pt )
{
    if ( !( TreeIsEmpty( pt ) ) )
        return TraverseWtVal( pt );
    else
        return 0;
}

void genCSV( double lat, double lon, double alt, TCHAR* fName )
{

}