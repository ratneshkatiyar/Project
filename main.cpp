#include <iostream>

#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <cstdint>
#include <memory>
#include <fstream>

using namespace std;

#define SCALEBITS 8
#define ONE_HALF ( 1 << ( SCALEBITS - 1 ) )
#define FIX( x ) ( (int)( ( x ) * ( 1L << SCALEBITS ) + 0.5 ) )

unsigned char* raw_image = NULL;
unsigned char* yuv_image = NULL;
unsigned char* ybuffer = NULL;
unsigned char* ubuffer = NULL;
unsigned char* vbuffer = NULL;
unsigned char* out_image = NULL;

unsigned char* YBlendedBuffer = NULL;
unsigned char* UBlendedBuffer = NULL;
unsigned char* VBlendedBuffer = NULL;

int width = 0;
int height = 0;

void
blendingForYdata( unsigned char* ybuffer, int width, int height, unsigned char* out_buffer )
{
    unsigned char* tmpbuff = NULL;
    unsigned char* nextbuff = NULL;
    unsigned char* tempOutbuff = NULL;
    tmpbuff = ybuffer;
    tempOutbuff = out_buffer;
    memcpy( tempOutbuff, ybuffer, width );  // first line copy as it is

    tempOutbuff += width;
    nextbuff = ybuffer + width;

    for ( int i = 1; i < height; i++ )
    {
        int cnt = 1;
        while ( cnt <= width )
        {
            // n = (n + n-1 ) /2 blending algo
            unsigned char* val = (unsigned char*)malloc( 1 );
            *val = ( ( *nextbuff ) + ( *tmpbuff ) ) / 2;
            memcpy( tempOutbuff, val, 1 );

            nextbuff++;
            tmpbuff++;
            tempOutbuff++;
            cnt++;
            free( val );
        }
    }
}

void
blendingForUdata( unsigned char* ubuffer, int width, int height, unsigned char* out_buffer )
{
    width = width / 2;
    height = height / 2;

    unsigned char* tmpbuff;
    unsigned char* nextbuff;
    unsigned char* tempOutbuff = NULL;
    tmpbuff = ubuffer;
    tempOutbuff = out_buffer;
    memcpy( tempOutbuff, ubuffer, width );

    tempOutbuff += width;
    nextbuff = ubuffer + width;
    int size = 0;
    for ( int i = 1; i < height; i++ )
    {
        int cnt = 1;
        while ( cnt <= width )
        {
            // n = (n + n-1 ) /2
            unsigned char* val = (unsigned char*)malloc( 1 );
            *val = ( ( *nextbuff ) + ( *tmpbuff ) ) / 2;
            memcpy( tempOutbuff, val, 1 );
            nextbuff++;
            tmpbuff++;
            tempOutbuff++;
            cnt++;
            free( val );
        }
    }
}

void
blendingForVdata( unsigned char* vbuffer, int width, int height, unsigned char* out_buffer )
{
    width = width / 2;
    height = height / 2;

    unsigned char* tmpbuff;
    unsigned char* nextbuff;
    unsigned char* tempOutbuff = NULL;
    tempOutbuff = out_buffer;
    tmpbuff = vbuffer;
    memcpy( tempOutbuff, vbuffer, width );

    tempOutbuff += width;
    nextbuff = vbuffer + width;
    for ( int i = 1; i < height; i++ )
    {
        int cnt = 1;
        while ( cnt <= width )
        {
            // n = (n + n-1 ) /2
            unsigned char* val = (unsigned char*)malloc( 1 );
            *val = ( ( *nextbuff ) + ( *tmpbuff ) ) / 2;
            memcpy( tempOutbuff, val, 1 );
            nextbuff++;
            tmpbuff++;
            tempOutbuff++;
            cnt++;
            free( val );
        }
    }
}

void
rgb24_to_yuv420p( unsigned char* lum,
                  unsigned char* cb,
                  unsigned char* cr,
                  unsigned char* src,
                  int width,
                  int height )
{
    int wrap, wrap3, x, y;
    int r, g, b, r1, g1, b1;
    unsigned char* p;
    wrap = width;
    wrap3 = width * 3;
    p = src;
    for ( y = 0; y < height; y += 2 )
    {
        for ( x = 0; x < width; x += 2 )
        {
            r = p[ 0 ];
            g = p[ 1 ];
            b = p[ 2 ];
            r1 = r;
            g1 = g;
            b1 = b;
            lum[ 0 ] = ( FIX( 0.29900 ) * r + FIX( 0.58700 ) * g + FIX( 0.11400 ) * b + ONE_HALF )
                       >> SCALEBITS;
            r = p[ 3 ];
            g = p[ 4 ];
            b = p[ 5 ];
            r1 += r;
            g1 += g;
            b1 += b;
            lum[ 1 ] = ( FIX( 0.29900 ) * r + FIX( 0.58700 ) * g + FIX( 0.11400 ) * b + ONE_HALF )
                       >> SCALEBITS;
            p += wrap3;
            lum += wrap;
            r = p[ 0 ];
            g = p[ 1 ];
            b = p[ 2 ];
            r1 += r;
            g1 += g;
            b1 += b;
            lum[ 0 ] = ( FIX( 0.29900 ) * r + FIX( 0.58700 ) * g + FIX( 0.11400 ) * b + ONE_HALF )
                       >> SCALEBITS;
            r = p[ 3 ];
            g = p[ 4 ];
            b = p[ 5 ];
            r1 += r;
            g1 += g;
            b1 += b;
            lum[ 1 ] = ( FIX( 0.29900 ) * r + FIX( 0.58700 ) * g + FIX( 0.11400 ) * b + ONE_HALF )
                       >> SCALEBITS;

            cb[ 0 ] = ( ( ( -FIX( 0.16874 ) * r1 - FIX( 0.33126 ) * g1 + FIX( 0.50000 ) * b1
                            + 4 * ONE_HALF - 1 )
                          >> ( SCALEBITS + 2 ) )
                        + 128 );
            cr[ 0 ] = ( ( ( FIX( 0.50000 ) * r1 - FIX( 0.41869 ) * g1 - FIX( 0.08131 ) * b1
                            + 4 * ONE_HALF - 1 )
                          >> ( SCALEBITS + 2 ) )
                        + 128 );
            cb++;
            cr++;
            p += -wrap3 + 2 * 3;
            lum += -wrap + 2;
        }
        p += wrap3;
        lum += wrap;
    }
}

int
read_jpeg_file( char* filename )
{
    FILE* blendedbuffer = fopen( "blendedbuffer.yuv", "a" );
    if ( !blendedbuffer )
    {
        printf( "Error opening dumpfile yuv file %s\n!", "blendedbuffer.yuv" );
        return -1;
    }

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[ 1 ];

    FILE* infile = fopen( filename, "rb" );
    unsigned long location = 0;
    int i = 0;

    if ( !infile )
    {
        cout << "Error opening in file: " << filename << endl;
        return -1;
    }

    cinfo.err = jpeg_std_error( &jerr );

    jpeg_create_decompress( &cinfo );

    jpeg_stdio_src( &cinfo, infile );

    jpeg_read_header( &cinfo, TRUE );

    jpeg_start_decompress( &cinfo );

    raw_image
        = (unsigned char*)malloc( cinfo.output_width * cinfo.output_height * cinfo.num_components );

    row_pointer[ 0 ] = (unsigned char*)malloc( cinfo.output_width * cinfo.num_components );

    while ( cinfo.output_scanline < cinfo.image_height )
    {
        jpeg_read_scanlines( &cinfo, row_pointer, 1 );
        for ( i = 0; i < cinfo.image_width * cinfo.num_components; i++ )
            raw_image[ location++ ] = row_pointer[ 0 ][ i ];
    }

    ybuffer = (unsigned char*)malloc( cinfo.output_width * cinfo.output_height );
    ubuffer = (unsigned char*)malloc( ( cinfo.output_width / 2 ) * ( cinfo.output_height / 2 ) );
    vbuffer = (unsigned char*)malloc( ( cinfo.output_width / 2 * cinfo.output_height / 2 ) );

    YBlendedBuffer = (unsigned char*)malloc( cinfo.output_width * cinfo.output_height );
    UBlendedBuffer
        = (unsigned char*)malloc( ( cinfo.output_width / 2 ) * ( cinfo.output_height / 2 ) );
    VBlendedBuffer = (unsigned char*)malloc( ( cinfo.output_width / 2 * cinfo.output_height / 2 ) );

    out_image
        = (unsigned char*)malloc( cinfo.output_width * cinfo.output_height * 1.5 );  // yuv4:2:0
    // RGBtoYUV(raw_image,yuv_image);

    rgb24_to_yuv420p( ybuffer, ubuffer, vbuffer, raw_image, cinfo.output_width,
                      cinfo.output_height );

    // started blending on YUV 4:2:0 data
    blendingForYdata( ybuffer, cinfo.output_width, cinfo.output_height, YBlendedBuffer );

    blendingForUdata( ubuffer, cinfo.output_width, cinfo.output_height, UBlendedBuffer );

    blendingForVdata( vbuffer, cinfo.output_width, cinfo.output_height, VBlendedBuffer );

    // dump of blended data
    fwrite( YBlendedBuffer, 1, cinfo.output_width * cinfo.output_height, blendedbuffer );
    fwrite( UBlendedBuffer, 1, ( cinfo.output_width / 2 ) * ( cinfo.output_height / 2 ),
            blendedbuffer );
    fwrite( VBlendedBuffer, 1, ( cinfo.output_width / 2 ) * ( cinfo.output_height / 2 ),
            blendedbuffer );

    jpeg_finish_decompress( &cinfo );
    jpeg_destroy_decompress( &cinfo );
    free( row_pointer[ 0 ] );
    free( ybuffer );
    free( ubuffer );
    free( vbuffer );
    free( YBlendedBuffer );
    free( UBlendedBuffer );
    free( VBlendedBuffer );

    fclose( infile );
    fclose( blendedbuffer );

    return 1;
}

typedef struct
{
    struct jpeg_destination_mgr pub;
    JOCTET* buf;
    size_t bufsize;
    size_t jpegsize;
} mem_destination_mgr;

typedef mem_destination_mgr* mem_dest_ptr;
static GLOBAL( int ) jpeg_mem_size( j_compress_ptr cinfo )
{
    mem_dest_ptr dest = (mem_dest_ptr)cinfo->dest;
    return dest->jpegsize;
}

int
jpeg_to_yuv420p( unsigned char* dest_image,
                 unsigned char* input_image,
                 int width,
                 int height,
                 unsigned long size )
{
    int i, j, jpeg_image_size;

    JSAMPROW y[ 16 ], cb[ 16 ],
        cr[ 16 ];          // y[2][5] = color sample of row 2 and pixel column 5; (one plane)
    JSAMPARRAY data[ 3 ];  // t[0][2][5] = color sample 0 of row 2 and column 5

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    data[ 0 ] = y;
    data[ 1 ] = cb;
    data[ 2 ] = cr;

    cinfo.err = jpeg_std_error( &jerr );  // errors get written to stderr

    jpeg_create_compress( &cinfo );
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    jpeg_set_defaults( &cinfo );

    jpeg_set_colorspace( &cinfo, JCS_YCbCr );

    cinfo.raw_data_in = TRUE;             // supply downsampled data
    cinfo.do_fancy_downsampling = FALSE;  // fix segfaulst with v7
    cinfo.comp_info[ 0 ].h_samp_factor = 2;
    cinfo.comp_info[ 0 ].v_samp_factor = 2;
    cinfo.comp_info[ 1 ].h_samp_factor = 1;
    cinfo.comp_info[ 1 ].v_samp_factor = 1;
    cinfo.comp_info[ 2 ].h_samp_factor = 1;
    cinfo.comp_info[ 2 ].v_samp_factor = 1;

    // jpeg_set_quality(&cinfo, QUALITY, TRUE);
    cinfo.dct_method = JDCT_FASTEST;

    jpeg_mem_dest( &cinfo, &dest_image, &size );  // data written to mem

    jpeg_start_compress( &cinfo, TRUE );

    for ( j = 0; j < height; j += 16 )
    {
        for ( i = 0; i < 16; i++ )
        {
            y[ i ] = input_image + width * ( i + j );
            if ( i % 2 == 0 )
            {
                cb[ i / 2 ] = input_image + width * height + width / 2 * ( ( i + j ) / 2 );
                cr[ i / 2 ] = input_image + width * height + width * height / 4
                              + width / 2 * ( ( i + j ) / 2 );
            }
        }
        jpeg_write_raw_data( &cinfo, data, 16 );
    }

    jpeg_finish_compress( &cinfo );
    jpeg_image_size = jpeg_mem_size( &cinfo );
    jpeg_destroy_compress( &cinfo );
    return jpeg_image_size;
}

int
findHeightandWidth( char* cpFileName )
{
    int iHeight = 0, iWidth = 0, iPos, i;
    // char *cpFileName = "/images/image1.jpg";

    FILE* fp = fopen( cpFileName, "rb" );
    fseek( fp, 0, SEEK_END );
    long len = ftell( fp );
    fseek( fp, 0, SEEK_SET );

    unsigned char* ucpImageBuffer = (unsigned char*)malloc( len + 1 );
    fread( ucpImageBuffer, 1, len, fp );
    fclose( fp );

    /*Extract start of frame marker(FFCO) of width and hight and get the position*/
    for ( i = 0; i < len; i++ )
    {
        if ( ( ucpImageBuffer[ i ] == 0xFF ) && ( ucpImageBuffer[ i + 1 ] == 0xC0 ) )
        {
            iPos = i;
            break;
        }
    }

    /*Moving to the particular byte position and assign byte value to pointer variable*/
    iPos = iPos + 5;
    iHeight = ucpImageBuffer[ iPos ] << 8 | ucpImageBuffer[ iPos + 1 ];
    iWidth = ucpImageBuffer[ iPos + 2 ] << 8 | ucpImageBuffer[ iPos + 3 ];
    width = iWidth;
    height = iHeight;
    if ( height <= 0 && width < 0 )
        return -1;

    cout << "Width and height for Input image are: " << width << "x" << height << endl;

    free( ucpImageBuffer );
    return 1;
}

int
main( int argc, char** argv )
{
    if ( argc < 3 )
    {
        cout << "Usage : ./a.out <Input filename> <Output filename>" << endl;
        exit( -1 );
    }

    char *infilename = argv[ 1 ], *outfilename = argv[ 2 ];
    int res = findHeightandWidth( infilename );
    if ( res == -1 )
    {
        cout << "Height and Width for the image not calculated properly. Cant proceed further "
             << endl;
    }
    if ( read_jpeg_file( infilename ) > 0 )
    {
        cout << "read_jpeg_file() completed successfully" << endl;
    }
    else
    {
        return -1;
    }

    FILE* infp = fopen( "blendedbuffer.yuv", "rb" );
    if ( !infp )
    {
        cout << "Error opening blendedbuffer.yuv jpeg file \n!" << endl;
        return -1;
    }
    fseek( infp, 0, SEEK_END );
    long len = ftell( infp );
    fseek( infp, 0, SEEK_SET );

    unsigned char* ucpImageBuffer = (unsigned char*)malloc( len + 1 );
    fread( ucpImageBuffer, 1, len, infp );
    fclose( infp );
    unsigned char* pDst = (unsigned char*)malloc( len + 1 );
    long long int lSize = jpeg_to_yuv420p( pDst, ucpImageBuffer, width, height, len );

    FILE* outfile = fopen( outfilename, "wb" );

    if ( !outfile )
    {
        cout << "Error opening output jpeg file\n!" << outfilename << endl;
        return -1;
    }
    fwrite( pDst, 1, len, outfile );
    cout << "YUV File writing completed " << endl;
    fclose( outfile );
    remove( "blendedbuffer.yuv" );

    return 0;
}
