#include <math.h>
#include <string.h>

#include "Math.hpp"
#include "ProcessCommon.hpp"
#include "ProcessRGB.h"
#include "Tables.hpp"
#include "Types.hpp"
#include "Vector.hpp"

namespace
{

static inline uint32 byteswap(uint32 l)
{
    return ((l >> 8u) & 0xff00u) | ((l & 0xff00u) << 8u) | (l << 24u) | (l >> 24u);
}

template<typename T, size_t size>
struct simple_array
{
    simple_array()
    {
        memset(data, 0, sizeof(data));
    }

    T operator [](size_t i) const
    {
        return data[i];
    }
    T & operator [](size_t i)
    {
        return data[i];
    }

protected:
    T data[size];
};

typedef simple_array<uint16, 4> v4i;

void Average( const uint8* data, v4i* a )
{
    uint32 r[4];
    uint32 g[4];
    uint32 b[4];

    memset(r, 0, sizeof(r));
    memset(g, 0, sizeof(g));
    memset(b, 0, sizeof(b));

    for( int j=0; j<4; j++ )
    {
        for( int i=0; i<4; i++ )
        {
            int index = (j & 2) + (i >> 1);
            r[index] += *data++;
            g[index] += *data++;
            b[index] += *data++;
            data++;
        }
    }

    a[0][0] = uint16( (r[2] + r[3] + 4) / 8 );
    a[0][1] = uint16( (g[2] + g[3] + 4) / 8 );
    a[0][2] = uint16( (b[2] + b[3] + 4) / 8 );
    a[0][3] = 0;
    a[1][0] = uint16( (r[0] + r[1] + 4) / 8 );
    a[1][1] = uint16( (g[0] + g[1] + 4) / 8 );
    a[1][2] = uint16( (b[0] + b[1] + 4) / 8 );
    a[1][3] = 0;
    a[2][0] = uint16( (r[1] + r[3] + 4) / 8 );
    a[2][1] = uint16( (g[1] + g[3] + 4) / 8 );
    a[2][2] = uint16( (b[1] + b[3] + 4) / 8 );
    a[2][3] = 0;
    a[3][0] = uint16( (r[0] + r[2] + 4) / 8 );
    a[3][1] = uint16( (g[0] + g[2] + 4) / 8 );
    a[3][2] = uint16( (b[0] + b[2] + 4) / 8 );
    a[3][3] = 0;
}

void CalcErrorBlock( const uint8* data, uint err[4][4] )
{
    uint terr[4][4];

    memset(terr, 0, 16 * sizeof(uint));

    for( int j=0; j<4; j++ )
    {
        for( int i=0; i<4; i++ )
        {
            int index = (j & 2) + (i >> 1);
            uint d = *data++;
            terr[index][0] += d;
            d = *data++;
            terr[index][1] += d;
            d = *data++;
            terr[index][2] += d;
            data++;
        }
    }

    for( int i=0; i<3; i++ )
    {
        err[0][i] = terr[2][i] + terr[3][i];
        err[1][i] = terr[0][i] + terr[1][i];
        err[2][i] = terr[1][i] + terr[3][i];
        err[3][i] = terr[0][i] + terr[2][i];
    }
    for( int i=0; i<4; i++ )
    {
        err[i][3] = 0;
    }
}

uint CalcError( const uint block[4], const v4i& average )
{
    uint err = 0x3FFFFFFF; // Big value to prevent negative values, but small enough to prevent overflow
    err -= block[0] * 2 * average[0];
    err -= block[1] * 2 * average[1];
    err -= block[2] * 2 * average[2];
    err += 8 * ( sq( average[0] ) + sq( average[1] ) + sq( average[2] ) );
    return err;
}

void ProcessAverages( v4i* a )
{
    for( int i=0; i<2; i++ )
    {
        for( int j=0; j<3; j++ )
        {
            int32 c1 = mul8bit( a[i*2+1][j], 31 );
            int32 c2 = mul8bit( a[i*2][j], 31 );

            int32 diff = c2 - c1;
            if( diff > 3 ) diff = 3;
            else if( diff < -4 ) diff = -4;

            int32 co = c1 + diff;

            a[5+i*2][j] = ( c1 << 3 ) | ( c1 >> 2 );
            a[4+i*2][j] = ( co << 3 ) | ( co >> 2 );
        }
    }

    for( int i=0; i<4; i++ )
    {
        a[i][0] = g_avg2[mul8bit( a[i][0], 15 )];
        a[i][1] = g_avg2[mul8bit( a[i][1], 15 )];
        a[i][2] = g_avg2[mul8bit( a[i][2], 15 )];
    }
}

void EncodeAverages( uint64& _d, const v4i* a, size_t idx )
{
    auto d = _d;
    d |= ( idx << 24 );
    size_t base = idx << 1;

    if( ( idx & 0x2 ) == 0 )
    {
        for( int i=0; i<3; i++ )
        {
            d |= uint64( a[base+0][i] >> 4 ) << ( i*8 );
            d |= uint64( a[base+1][i] >> 4 ) << ( i*8 + 4 );
        }
    }
    else
    {
        for( int i=0; i<3; i++ )
        {
            d |= uint64( a[base+1][i] & 0xF8 ) << ( i*8 );
            int32 c = ( ( a[base+0][i] & 0xF8 ) - ( a[base+1][i] & 0xF8 ) ) >> 3;
            c &= ~0xFFFFFFF8;
            d |= ((uint64)c) << ( i*8 );
        }
    }
    _d = d;
}

uint64 CheckSolid( const uint8* src )
{
    const uint8* ptr = src + 4;
    for( int i=1; i<16; i++ )
    {
        if( memcmp( src, ptr, 4 ) != 0 )
        {
            return 0;
        }
        ptr += 4;
    }

    return 0x02000000 |
        ( uint( src[0] & 0xF8 ) ) |
        ( uint( src[1] & 0xF8 ) << 8 ) |
        ( uint( src[2] & 0xF8 ) << 16 );
}

void PrepareAverages( v4i a[8], const uint8* src, uint err[4] )
{
    Average( src, a );
    ProcessAverages( a );

    uint errblock[4][4];
    CalcErrorBlock( src, errblock );

    for( int i=0; i<4; i++ )
    {
        err[i/2] += CalcError( errblock[i], a[i] );
        err[2+i/2] += CalcError( errblock[i], a[i+4] );
    }
}

void FindBestFit( uint64 terr[2][8], uint16 tsel[16][8], v4i a[8], const uint32* id, const uint8* data )
{
    for( size_t i=0; i<16; i++ )
    {
        uint16* sel = tsel[i];
        uint bid = id[i];
        uint64* ter = terr[bid%2];

        uint8 r = *data++;
        uint8 g = *data++;
        uint8 b = *data++;
        data++;

        int dr = a[bid][0] - r;
        int dg = a[bid][1] - g;
        int db = a[bid][2] - b;

        int pix = dr * 77 + dg * 151 + db * 28;

        for( int t=0; t<8; t++ )
        {
            const int64* tab = g_table256[t];
            uint idx = 0;
            uint64 err = sq( tab[0] + pix );
            for( int j=1; j<4; j++ )
            {
                uint64 local = sq( tab[j] + pix );
                if( local < err )
                {
                    err = local;
                    idx = j;
                }
            }
            *sel++ = idx;
            *ter++ += err;
        }
    }
}

uint8_t convert6(float f)
{
    int i = (std::min(std::max(static_cast<int>(f), 0), 1023) - 15) >> 1;
    return (i + 11 - ((i + 11) >> 7) - ((i + 4) >> 7)) >> 3;
}

uint8_t convert7(float f)
{
    int i = (std::min(std::max(static_cast<int>(f), 0), 1023) - 15) >> 1;
    return (i + 9 - ((i + 9) >> 8) - ((i + 6) >> 8)) >> 2;
}

std::pair<uint64, uint64> Planar(const uint8* src)
{
    int32 r = 0;
    int32 g = 0;
    int32 b = 0;

    for (int i = 0; i < 16; ++i)
    {
        r += src[i * 4 + 0];
        g += src[i * 4 + 1];
        b += src[i * 4 + 2];
    }

    int32 difRyz = 0;
    int32 difGyz = 0;
    int32 difByz = 0;
    int32 difRxz = 0;
    int32 difGxz = 0;
    int32 difBxz = 0;

    const int32 scaling[] = { -255, -85, 85, 255 };

    for (int i = 0; i < 16; ++i)
    {
        int32 difR = (static_cast<int>(src[i * 4 + 0]) << 4) - r;
        int32 difG = (static_cast<int>(src[i * 4 + 1]) << 4) - g;
        int32 difB = (static_cast<int>(src[i * 4 + 2]) << 4) - b;

        difRyz += difR * scaling[i % 4];
        difGyz += difG * scaling[i % 4];
        difByz += difB * scaling[i % 4];

        difRxz += difR * scaling[i / 4];
        difGxz += difG * scaling[i / 4];
        difBxz += difB * scaling[i / 4];
    }

    const float scale = -4.0f / ((255 * 255 * 8.0f + 85 * 85 * 8.0f) * 16.0f);

    float aR = difRxz * scale;
    float aG = difGxz * scale;
    float aB = difBxz * scale;

    float bR = difRyz * scale;
    float bG = difGyz * scale;
    float bB = difByz * scale;

    float dR = r * (4.0f / 16.0f);
    float dG = g * (4.0f / 16.0f);
    float dB = b * (4.0f / 16.0f);

    // calculating the three colors RGBO, RGBH, and RGBV.  RGB = df - af * x - bf * y;
    float cofR = fma(aR,  255.0f, fma(bR,  255.0f, dR));
    float cofG = fma(aG,  255.0f, fma(bG,  255.0f, dG));
    float cofB = fma(aB,  255.0f, fma(bB,  255.0f, dB));
    float chfR = fma(aR, -425.0f, fma(bR,  255.0f, dR));
    float chfG = fma(aG, -425.0f, fma(bG,  255.0f, dG));
    float chfB = fma(aB, -425.0f, fma(bB,  255.0f, dB));
    float cvfR = fma(aR,  255.0f, fma(bR, -425.0f, dR));
    float cvfG = fma(aG,  255.0f, fma(bG, -425.0f, dG));
    float cvfB = fma(aB,  255.0f, fma(bB, -425.0f, dB));

    // convert to r6g7b6
    int32 coR = convert6(cofR);
    int32 coG = convert7(cofG);
    int32 coB = convert6(cofB);
    int32 chR = convert6(chfR);
    int32 chG = convert7(chfG);
    int32 chB = convert6(chfB);
    int32 cvR = convert6(cvfR);
    int32 cvG = convert7(cvfG);
    int32 cvB = convert6(cvfB);

    // Error calculation
    auto ro0 = coR;
    auto go0 = coG;
    auto bo0 = coB;
    auto ro1 = (ro0 >> 4) | (ro0 << 2);
    auto go1 = (go0 >> 6) | (go0 << 1);
    auto bo1 = (bo0 >> 4) | (bo0 << 2);
    auto ro2 = (ro1 << 2) + 2;
    auto go2 = (go1 << 2) + 2;
    auto bo2 = (bo1 << 2) + 2;

    auto rh0 = chR;
    auto gh0 = chG;
    auto bh0 = chB;
    auto rh1 = (rh0 >> 4) | (rh0 << 2);
    auto gh1 = (gh0 >> 6) | (gh0 << 1);
    auto bh1 = (bh0 >> 4) | (bh0 << 2);

    auto rh2 = rh1 - ro1;
    auto gh2 = gh1 - go1;
    auto bh2 = bh1 - bo1;

    auto rv0 = cvR;
    auto gv0 = cvG;
    auto bv0 = cvB;
    auto rv1 = (rv0 >> 4) | (rv0 << 2);
    auto gv1 = (gv0 >> 6) | (gv0 << 1);
    auto bv1 = (bv0 >> 4) | (bv0 << 2);

    auto rv2 = rv1 - ro1;
    auto gv2 = gv1 - go1;
    auto bv2 = bv1 - bo1;

    uint64 error = 0;

    for (int i = 0; i < 16; ++i)
    {
        int32 cR = clampu8((rh2 * (i / 4) + rv2 * (i % 4) + ro2) >> 2);
        int32 cG = clampu8((gh2 * (i / 4) + gv2 * (i % 4) + go2) >> 2);
        int32 cB = clampu8((bh2 * (i / 4) + bv2 * (i % 4) + bo2) >> 2);

        int32 difR = static_cast<int>(src[i * 4 + 0]) - cR;
        int32 difG = static_cast<int>(src[i * 4 + 1]) - cG;
        int32 difB = static_cast<int>(src[i * 4 + 2]) - cB;

        int32 dif = difR * 38 + difG * 76 + difB * 14;

        error += dif * dif;
    }

    /**/
    uint32 rgbv = cvB | (cvG << 6) | (cvR << 13);
    uint32 rgbh = chB | (chG << 6) | (chR << 13);
    uint32 hi = rgbv | ((rgbh & 0x1FFF) << 19);
    uint32 lo = (chR & 0x1) | 0x2 | ((chR << 1) & 0x7C);
    lo |= ((coB & 0x07) <<  7) | ((coB & 0x18) <<  8) | ((coB & 0x20) << 11);
    lo |= ((coG & 0x3F) << 17) | ((coG & 0x40) << 18);
    lo |= coR << 25;

    const auto idx = (coR & 0x20) | ((coG & 0x20) >> 1) | ((coB & 0x1E) >> 1);

    lo |= g_flags[idx];

    uint64 result = static_cast<uint32>(byteswap(lo));
    result |= static_cast<uint64>(static_cast<uint32>(byteswap(hi))) << 32;

    return std::make_pair(result, error);
}

template<class T, class S>
uint64 EncodeSelectors( uint64 d, const T terr[2][8], const S tsel[16][8], const uint32* id, const uint64 value, const uint64 error)
{
    size_t tidx[2];
    tidx[0] = GetLeastError( terr[0], 8 );
    tidx[1] = GetLeastError( terr[1], 8 );

    if ((terr[0][tidx[0]] + terr[1][tidx[1]]) >= error)
    {
        return value;
    }

    d |= tidx[0] << 26;
    d |= tidx[1] << 29;
    for( int i=0; i<16; i++ )
    {
        uint64 t = tsel[i][tidx[id[i]%2]];
        d |= ( t & 0x1 ) << ( i + 32 );
        d |= ( t & 0x2 ) << ( i + 47 );
    }

    return FixByteOrder(d);
}
}

uint64 ProcessRGB( const uint8* src )
{
    uint64 d = CheckSolid( src );
    if( d != 0 ) return d;

    v4i a[8];
    uint err[4] = {};
    PrepareAverages( a, src, err );
    size_t idx = GetLeastError( err, 4 );
    EncodeAverages( d, a, idx );

    uint64 terr[2][8] = {};
    uint16 tsel[16][8];
    auto id = g_id[idx];
    FindBestFit( terr, tsel, a, id, src );

    return FixByteOrder( EncodeSelectors( d, terr, tsel, id ) );
}

uint64 ProcessRGB_ETC2( const uint8* src )
{
    auto result = Planar( src );

    uint64 d = 0;

    v4i a[8];
    uint err[4] = {};
    PrepareAverages( a, src, err );
    size_t idx = GetLeastError( err, 4 );
    EncodeAverages( d, a, idx );

    uint64 terr[2][8] = {};
    uint16 tsel[16][8];
    auto id = g_id[idx];
    FindBestFit( terr, tsel, a, id, src );

    return EncodeSelectors( d, terr, tsel, id, result.first, result.second );
}

