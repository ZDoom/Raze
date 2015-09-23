/**
 * BUILD ART file editing tool
 * @author Jonathon Fowler
 * @license Artistic License 2.0 (http://www.perlfoundation.org/artistic_license_2_0)
 */
// Bstring and C++ STL --> C conversion by Hendricks266

#include <stdio.h>
#include <string.h>

#include "compat.h"

////////// Bstring //////////

class Bstring {
    public:
        Bstring(void);
        Bstring(const char*);
        Bstring(const Bstring&);
        ~Bstring(void);

        operator const char*() const;
        const char* operator()(void) const;
        char& operator[](int);

        Bstring& operator=(const char*);
        Bstring& operator=(const Bstring&);

        Bstring& operator+=(const char*);
        Bstring& operator+=(const Bstring&);

        bool operator==(const Bstring&) const;
        bool operator!=(const Bstring&) const;
        bool operator< (const Bstring&) const;
        bool operator<=(const Bstring&) const;
        bool operator> (const Bstring&) const;
        bool operator>=(const Bstring&) const;

        bool operator==(const char*) const;
        bool operator!=(const char*) const;
        bool operator< (const char*) const;
        bool operator<=(const char*) const;
        bool operator> (const char*) const;
        bool operator>=(const char*) const;

        int compare(const char*) const;
        int compare(const Bstring&) const;

        unsigned length(void) const;

        void clear(void);

    protected:
        char* data;
};

Bstring::Bstring(void) { data = NULL; }
Bstring::Bstring(const Bstring &value) {
    if (&value != this)
        (*this)=value();
}
Bstring::Bstring(const char *str) {
    data = NULL;
    (*this)=str;
}

Bstring::~Bstring(void) { clear(); }

Bstring::operator const char*() const { return data; }
const char* Bstring::operator()(void) const { return data; }
char& Bstring::operator[](int index) { return data[index]; }

Bstring& Bstring::operator=(const Bstring &value)
{
    if (&value != this)
        (*this)=value();

    return *this;
}
Bstring& Bstring::operator=(const char *str)
{
    clear();
    data = Bstrdup(str);

    return *this;
}

Bstring& Bstring::operator+=(const Bstring &value)
{
    (*this)+=value();

    return *this;
}
Bstring& Bstring::operator+=(const char *str)
{
    data = (char*) Brealloc(data, (Bstrlen(data) + Bstrlen(str) + 1) * sizeof(char));
    Bstrcat(data, str);

    return *this;
}

bool Bstring::operator==(const Bstring &value) const { return Bstrcmp(data, value()) == 0; }
bool Bstring::operator!=(const Bstring &value) const { return Bstrcmp(data, value()) != 0; }
bool Bstring::operator< (const Bstring &value) const { return Bstrcmp(data, value())  < 0; }
bool Bstring::operator<=(const Bstring &value) const { return Bstrcmp(data, value()) <= 0; }
bool Bstring::operator> (const Bstring &value) const { return Bstrcmp(data, value())  > 0; }
bool Bstring::operator>=(const Bstring &value) const { return Bstrcmp(data, value()) >= 0; }

bool Bstring::operator==(const char *str) const { return Bstrcmp(data, str) == 0; }
bool Bstring::operator!=(const char *str) const { return Bstrcmp(data, str) != 0; }
bool Bstring::operator< (const char *str) const { return Bstrcmp(data, str)  < 0; }
bool Bstring::operator<=(const char *str) const { return Bstrcmp(data, str) <= 0; }
bool Bstring::operator> (const char *str) const { return Bstrcmp(data, str)  > 0; }
bool Bstring::operator>=(const char *str) const { return Bstrcmp(data, str) >= 0; }

int Bstring::compare(const Bstring &value) const { return Bstrcmp(data,value()); }
int Bstring::compare(const char *str) const { return Bstrcmp(data,str); }

unsigned Bstring::length(void) const { return Bstrlen(data); }

void Bstring::clear(void)
{
    DO_FREE_AND_NULL(data);
}

////////// arttool //////////

void usage()
{
    Bprintf("BUILD ART file editing tool\n");
    Bprintf("Copyright (C) 2008 Jonathon Fowler <jf@jonof.id.au>\n");
    Bprintf("Released under the Artistic License 2.0\n");
    Bprintf("\n");
    Bprintf("  arttool create [options]\n");
    Bprintf("    -f <filenum>   Selects which numbered ART file to create (default 0)\n");
    Bprintf("    -o <offset>    Specifies the first tile in the file (default 0)\n");
    Bprintf("    -n <ntiles>    The number of tiles for the art file (default 256)\n");
    Bprintf("    Creates an empty ART file named 'tilesXXX.art'\n");
    Bprintf("\n");
    Bprintf("  arttool addtile [options] <tilenum> <filename>\n");
    Bprintf("    -x <pixels>    X-centre\n");
    Bprintf("    -y <pixels>    Y-centre\n");
    Bprintf("    -ann <frames>  Animation frame span\n");
    Bprintf("    -ant <type>    Animation type (0=none, 1=oscillate, 2=forward, 3=reverse)\n");
    Bprintf("    -ans <speed>   Animation speed\n");
    Bprintf("    Adds a tile to the 'tilesXXX.art' set from a TGA or PCX source\n");
    Bprintf("\n");
    Bprintf("  arttool rmtile <tilenum>\n");
    Bprintf("    Removes a tile from the 'tilesXXX.art' set\n");
    Bprintf("\n");
    Bprintf("  arttool tileprop [options] <tilenum>\n");
    Bprintf("    -x <pixels>    X-centre\n");
    Bprintf("    -y <pixels>    Y-centre\n");
    Bprintf("    -ann <frames>  Animation frame span, may be negative\n");
    Bprintf("    -ant <type>    Animation type (0=none, 1=oscillate, 2=forward, 3=reverse)\n");
    Bprintf("    -ans <speed>   Animation speed\n");
    Bprintf("    Changes tile properties\n");
    Bprintf("\n");
}

class ARTFile {
private:
    Bstring filename_;
    long localtilestart_;
    long localtileend_;
    short * tilesizx_;
    short * tilesizy_;
    long * picanm_;

    // for removing or replacing tile data
    int markprelength_, markskiplength_, markpostlength_;
    char * insert_;
    int insertlen_;

    void writeShort(BFILE *ofs, short s)
    {
        Bassert(ofs);
        char d[2] = { static_cast<char>(s&255), static_cast<char>((s>>8)&255) };
        Bfwrite(d,1,2,ofs); // 2 == sizeof(d)
    }

    void writeLong(BFILE *ofs, long l)
    {
        Bassert(ofs);
        char d[4] = { static_cast<char>(l&255), static_cast<char>((l>>8)&255), static_cast<char>((l>>16)&255), static_cast<char>((l>>24)&255) };
        Bfwrite(d,1,4,ofs); // 4 == sizeof(d)
    }

    short readShort(BFILE *ifs)
    {
        Bassert(ifs);
        unsigned char d[2];
        unsigned short s;
        Bfread(d,1,2,ifs); // 2 == sizeof(d)
        s = (unsigned short)d[0];
        s |= (unsigned short)d[1] << 8;
        return (short)s;
    }

    long readLong(BFILE *ifs)
    {
        Bassert(ifs);
        unsigned char d[4];
        unsigned long l;
        Bfread(d,1,4,ifs); // 4 == sizeof(d)
        l = (unsigned long)d[0];
        l |= (unsigned long)d[1] << 8;
        l |= (unsigned long)d[2] << 16;
        l |= (unsigned long)d[3] << 24;
        return (long)l;
    }

    void dispose()
    {
        if (tilesizx_) delete [] tilesizx_;
        if (tilesizy_) delete [] tilesizy_;
        if (picanm_) delete [] picanm_;
        if (insert_) delete [] insert_;

        insert_ = 0;
        insertlen_ = 0;
    }

    void load()
    {
        BFILE *infile = Bfopen(filename_(),"rb");
        int i, ntiles;

        if (infile != NULL && readLong(infile) == 1)
        {
            readLong(infile);    // skip the numtiles
            dispose();

            localtilestart_ = readLong(infile);
            localtileend_   = readLong(infile);
            ntiles = localtileend_ - localtilestart_ + 1;

            tilesizx_ = new short[ntiles];
            tilesizy_ = new short[ntiles];
            picanm_   = new long[ntiles];

            for (i = 0; i < ntiles; ++i) {
                tilesizx_[i] = readShort(infile);
            }
            for (i = 0; i < ntiles; ++i) {
                tilesizy_[i] = readShort(infile);
            }
            for (i = 0; i < ntiles; ++i) {
                picanm_[i] = readLong(infile);
            }
        }
    }

public:
    ARTFile(Bstring filename)
     : filename_(filename), localtilestart_(0), localtileend_(-1),
       tilesizx_(0), tilesizy_(0), picanm_(0),
       markprelength_(0), markskiplength_(0), markpostlength_(0),
       insert_(0), insertlen_(0)
    {
        load();
    }

    ~ARTFile()
    {
        dispose();
    }

    /**
     * Sets up for an empty file
     * @param start the starting tile
     * @param ntiles the number of tiles total
     */
    void init(int start, int ntiles)
    {
        dispose();

        localtilestart_ = start;
        localtileend_ = start + ntiles - 1;
        tilesizx_ = new short[ntiles];
        tilesizy_ = new short[ntiles];
        picanm_   = new long[ntiles];

        memset(tilesizx_, 0, sizeof(short)*ntiles);
        memset(tilesizy_, 0, sizeof(short)*ntiles);
        memset(picanm_, 0, sizeof(long)*ntiles);

        markprelength_ = 0;
        markskiplength_ = 0;
        markpostlength_ = 0;
        insert_ = 0;
        insertlen_ = 0;
    }

    /**
     * Returns the number of tiles in the loaded file
     * @return 0 means no file loaded
     */
    int getNumTiles()
    {
        return (localtileend_ - localtilestart_ + 1);
    }

    int getFirstTile()
    {
        return localtilestart_;
    }

    int getLastTile()
    {
        return localtileend_;
    }

    void removeTile(int tile)
    {
        int i, end;

        if (tile < localtilestart_ || tile > localtileend_) {
            return;
        }

        end   = localtileend_ - tile;
        tile -= localtilestart_;

        markprelength_ = markpostlength_ = 0;

        for (i = 0; i < tile; ++i) {
            markprelength_ += tilesizx_[i] * tilesizy_[i];
        }
        markskiplength_ = tilesizx_[tile] * tilesizy_[tile];
        for (i = tile + 1; i <= end; ++i) {
            markpostlength_ += tilesizx_[i] * tilesizy_[i];
        }

        tilesizx_[tile] = tilesizy_[tile] = 0;
    }

    void replaceTile(int tile, char * replace, int replacelen)
    {
        if (tile < localtilestart_ || tile > localtileend_) {
            return;
        }

        removeTile(tile);

        insert_ = replace;
        insertlen_ = replacelen;
    }

    void setTileSize(int tile, int x, int y)
    {
        if (tile < localtilestart_ || tile > localtileend_) {
            return;
        }

        tile -= localtilestart_;
        tilesizx_[tile] = x;
        tilesizy_[tile] = y;
    }

    void setXOfs(int tile, int x)
    {
        if (tile < localtilestart_ || tile > localtileend_) {
            return;
        }

        tile -= localtilestart_;
        picanm_[tile] &= ~(255<<8);
        picanm_[tile] |= ((long)((unsigned char)x) << 8);
    }

    void setYOfs(int tile, int y)
    {
        if (tile < localtilestart_ || tile > localtileend_) {
            return;
        }

        tile -= localtilestart_;
        picanm_[tile] &= ~(255<<16);
        picanm_[tile] |= ((long)((unsigned char)y) << 16);
    }

    void setAnimType(int tile, int type)
    {
        if (tile < localtilestart_ || tile > localtileend_) {
            return;
        }

        tile -= localtilestart_;
        picanm_[tile] &= ~(3<<6);
        picanm_[tile] |= ((long)(type&3) << 6);
    }

    void setAnimFrames(int tile, int frames)
    {
        if (tile < localtilestart_ || tile > localtileend_) {
            return;
        }

        tile -= localtilestart_;
        picanm_[tile] &= ~(63);
        picanm_[tile] |= ((long)(frames&63));
    }

    void setAnimSpeed(int tile, int speed)
    {
        if (tile < localtilestart_ || tile > localtileend_) {
            return;
        }

        tile -= localtilestart_;
        picanm_[tile] &= ~(15<<24);
        picanm_[tile] |= ((long)(speed&15) << 24);
    }

    int write()
    {
        BFILE *outfile = tmpfile();

        BFILE *infile = Bfopen(filename_(),"rb");
        int tmp, left;
        static const unsigned int blksize = 4096;
        char blk[blksize];

        if (infile == NULL && (markprelength_ > 0 || markskiplength_ > 0 || markpostlength_ > 0)) {
            return -1;    // couldn't open the original file for copying
        } else if (infile != NULL) {
            // skip to the start of the existing ART data
            int ofs = 4+4+4+4+(2+2+4)*(localtileend_-localtilestart_+1);
            Bfseek(infile, ofs, SEEK_CUR);
        }

        // write a header to the temporary file
        writeLong(outfile, 1);    // version
        writeLong(outfile, 0);    // numtiles
        writeLong(outfile, localtilestart_);
        writeLong(outfile, localtileend_);
        for (int i = 0; i < localtileend_ - localtilestart_ + 1; ++i) {
            writeShort(outfile, tilesizx_[i]);
        }
        for (int i = 0; i < localtileend_ - localtilestart_ + 1; ++i) {
            writeShort(outfile, tilesizy_[i]);
        }
        for (int i = 0; i < localtileend_ - localtilestart_ + 1; ++i) {
            writeLong(outfile, picanm_[i]);
        }

        // copy the existing leading tile data to be kept
        left = markprelength_;
        while (left > 0) {
            tmp = left;
            if ((unsigned int)tmp > blksize) {
                tmp = blksize;
            }
            Bfread(blk, 1, tmp, infile);
            Bfwrite(blk, 1, tmp, outfile);
            left -= tmp;
        }

        // insert the replacement data
        if (insertlen_ > 0) {
            Bfwrite(insert_, 1, insertlen_, outfile);
        }

        if (markskiplength_ > 0) {
            Bfseek(infile, markskiplength_, SEEK_CUR);
        }

        // copy the existing trailing tile data to be kept
        left = markpostlength_;
        while (left > 0) {
            tmp = left;
            if ((unsigned int)tmp > blksize) {
                tmp = blksize;
            }
            Bfread(blk, 1, tmp, infile);
            Bfwrite(blk, 1, tmp, outfile);
            left -= tmp;
        }

        // clean up
        const long int tempsize = Bftell(outfile);
        Brewind(outfile);

        Bfclose(infile);

        infile = Bfopen(filename_(),"wb");

        char * buffer = (char*)Bmalloc(tempsize * sizeof(char));

        Bfread(buffer, 1, tempsize, outfile);
        Bfwrite(buffer, 1, tempsize, infile);

        Bfclose(infile);
        Bfclose(outfile);

        return 0;
    }
};

/**
 * Decodes a PCX file directly to BUILD's column-major pixel order
 * @param data the raw file data
 * @param datalen the length of the raw file data
 * @param imgdata receives a pointer to the decoded image data
 * @param imgdataw receives the decoded image width
 * @param imgdatah receives the decoded image height
 * @return 0 on success, 1 if the format is invalid
 */
int loadimage_pcx(unsigned char * data, int datalen ATTRIBUTE((unused)), char ** imgdata, int& imgdataw, int& imgdatah)
{
    if (data[0] != 10 ||
        data[1] != 5 ||
        data[2] != 1 ||
        data[3] != 8 ||
        data[64] != 0 ||
        data[65] != 1) {
        return 1;
    }

    int bpl = data[66] + ((int)data[67] << 8);
    int x, y, repeat, colour;
    unsigned char *wptr, *rptr;

    imgdataw = (data[8] + ((int)data[9] << 8)) - (data[4] + ((int)data[5] << 8)) + 1;
    imgdatah = (data[10] + ((int)data[11] << 8)) - (data[6] + ((int)data[7] << 8)) + 1;

    *imgdata = new char [imgdataw * imgdatah];

    rptr = data + 128;
    for (y = 0; y < imgdatah; y++) {
        wptr = (unsigned char *) (*imgdata + y);
        x = 0;
        do {
            repeat = *(rptr++);
            if ((repeat & 192) == 192) {
                colour = *(rptr++);
                repeat = repeat & 63;
            } else {
                colour = repeat;
                repeat = 1;
            }

            for (; repeat > 0; repeat--, x++) {
                if (x < imgdataw) {
                    *wptr = (unsigned char) colour;
                    wptr += imgdatah;    // next column
                }
            }
        } while (x < bpl);
    }

    return 0;
}

/**
 * Loads a tile from a picture file into memory
 * @param filename the filename
 * @param imgdata receives a pointer to the decoded image data
 * @param imgdataw receives the decoded image width
 * @param imgdatah receives the decoded image height
 * @return 0 on success
 */
int loadimage(Bstring filename, char ** imgdata, int& imgdataw, int& imgdatah)
{
    BFILE *infile = Bfopen(filename(),"rb");
    unsigned char * data = 0;
    int datalen = 0, err = 0;

    if (infile == NULL)
        return 1;

    struct Bstat stbuf;
    if (Bfstat(Bfileno(infile), &stbuf) == -1)
        return 1;

    datalen = stbuf.st_size;

    data = new unsigned char [datalen];
    Bfread(data, 1, datalen, infile);
    Bfclose(infile);

    err = loadimage_pcx(data, datalen, imgdata, imgdataw, imgdatah);

    delete [] data;

    return err;
}

class Operation {
protected:
    Bstring makefilename(int n)
    {
        Bstring filename("tilesXXX.art");
        filename[5] = '0' + (n / 100) % 10;
        filename[6] = '0' + (n / 10) % 10;
        filename[7] = '0' + (n / 1) % 10;
        return filename;
    }

public:
    typedef enum {
        ERR_NO_ERROR = 0,
        ERR_BAD_OPTION = 1,
        ERR_BAD_VALUE = 2,
        ERR_TOO_MANY_PARAMS = 3,
        ERR_NO_ART_FILE = 4,
        ERR_INVALID_IMAGE = 5,
    } Result;

    static char const * translateResult(Result r)
    {
        switch (r) {
            case ERR_NO_ERROR: return "no error";
            case ERR_BAD_OPTION: return "bad option";
            case ERR_BAD_VALUE: return "bad value";
            case ERR_TOO_MANY_PARAMS: return "too many parameters given";
            case ERR_NO_ART_FILE: return "no ART file was found";
            case ERR_INVALID_IMAGE: return "a nonexistent, corrupt, or unrecognised image was given";
            default: return "unknown error";
        }
    }

    virtual ~Operation()
    {
    }

    /**
     * Sets an option
     * @param opt the option name
     * @param value the option value
     * @return a value from the Result enum
     */
    virtual Result setOption(const Bstring &opt, const Bstring &value) = 0;

    /**
     * Sets a parameter from the unnamed sequence
     * @param number the parameter number
     * @param value the parameter value
     * @return a value from the Result enum
     */
    virtual Result setParameter(const int &number, const Bstring &value) = 0;

    /**
     * Do the operation
     * @return a value from the Result enum
     */
    virtual Result perform() = 0;
};

class CreateOp : public Operation {
private:
    int filen_, offset_, ntiles_;
public:
    CreateOp() : filen_(0), offset_(0), ntiles_(256) { }

    virtual Result setOption(const Bstring &opt, const Bstring &value)
    {
        if (opt == "f") {
            filen_ = atoi(value());
            if (filen_ < 0 || filen_ > 999) {
                return ERR_BAD_VALUE;
            }
        } else if (opt == "o") {
            offset_ = atoi(value());
            if (offset_ < 0) {
                return ERR_BAD_VALUE;
            }
        } else if (opt == "n") {
            ntiles_ = atoi(value());
            if (ntiles_ < 1) {
                return ERR_BAD_VALUE;
            }
        } else {
            return ERR_BAD_OPTION;
        }
        return ERR_NO_ERROR;
    }

    virtual Result setParameter(const int &number ATTRIBUTE((unused)), const Bstring &value ATTRIBUTE((unused)))
    {
        return ERR_TOO_MANY_PARAMS;
    }

    virtual Result perform()
    {
        ARTFile art(makefilename(filen_));

        art.init(offset_, ntiles_);
        art.write();

        return ERR_NO_ERROR;
    }
};

class AddTileOp : public Operation {
private:
    int xofs_, yofs_;
    int animframes_, animtype_, animspeed_;
    int tilenum_;
    Bstring filename_;
public:
    AddTileOp()
     : xofs_(0), yofs_(0),
       animframes_(0), animtype_(0), animspeed_(0),
       tilenum_(-1), filename_("")
    { }

    virtual Result setOption(const Bstring &opt, const Bstring &value)
    {
        if (opt == "x") {
            xofs_ = atoi(value());
        } else if (opt == "y") {
            yofs_ = atoi(value());
        } else if (opt == "ann") {
            animframes_ = atoi(value());
            if (animframes_ < 0 || animframes_ > 63) {
                return ERR_BAD_VALUE;
            }
        } else if (opt == "ant") {
            animtype_ = atoi(value());
            if (animtype_ < 0 || animtype_ > 3) {
                return ERR_BAD_VALUE;
            }
        } else if (opt == "ans") {
            animspeed_ = atoi(value());
            if (animspeed_ < 0 || animspeed_ > 15) {
                return ERR_BAD_VALUE;
            }
        } else {
            return ERR_BAD_OPTION;
        }
        return ERR_NO_ERROR;
    }

    virtual Result setParameter(const int &number, const Bstring &value)
    {
        switch (number) {
            case 0:
                tilenum_ = atoi(value());
                return ERR_NO_ERROR;
            case 1:
                filename_ = value;
                return ERR_NO_ERROR;
            default:
                return ERR_TOO_MANY_PARAMS;
        }
    }

    virtual Result perform()
    {
        int tilesperfile = 0, nextstart = 0;
        int filenum = 0;
        char * imgdata = 0;
        int imgdataw = 0, imgdatah = 0;

        // open the first art file to get the file size used by default
        {
            ARTFile art(makefilename(0));
            tilesperfile = art.getNumTiles();
            if (tilesperfile == 0) {
                return ERR_NO_ART_FILE;
            }
        }

        // load the tile image into memory
        switch (loadimage(filename_, &imgdata, imgdataw, imgdatah)) {
            case 0: break;    // win
            default: return ERR_INVALID_IMAGE;
        }

        // open art files until we find one that encompasses the range we need
        // and when we find it, make the change
        for (filenum = 0; filenum < 1000; filenum++) {
            ARTFile art(makefilename(filenum));
            bool dirty = false, done = false;

            if (art.getNumTiles() == 0) {
                // no file exists, so we treat it as though it does
                art.init(nextstart, tilesperfile);
                dirty = true;
            }

            if (tilenum_ >= art.getFirstTile() && tilenum_ <= art.getLastTile()) {
                art.replaceTile(tilenum_, imgdata, imgdataw * imgdatah);
                art.setTileSize(tilenum_, imgdataw, imgdatah);
                art.setXOfs(tilenum_, xofs_);
                art.setYOfs(tilenum_, yofs_);
                art.setAnimFrames(tilenum_, animframes_);
                art.setAnimSpeed(tilenum_, animspeed_);
                art.setAnimType(tilenum_, animtype_);
                done = true;
                dirty = true;

                imgdata = 0;    // ARTFile.replaceTile took ownership of the pointer
            }

            nextstart += art.getNumTiles();

            if (dirty) {
                art.write();
            }
            if (done) {
                return ERR_NO_ERROR;
            }
        }

        if (imgdata) {
            delete [] imgdata;
        }

        return ERR_NO_ART_FILE;
    }
};

class RmTileOp : public Operation {
private:
    int tilenum_;
public:
    RmTileOp() : tilenum_(-1) { }

    virtual Result setOption(const Bstring &opt ATTRIBUTE((unused)), const Bstring &value ATTRIBUTE((unused)))
    {
        return ERR_BAD_OPTION;
    }

    virtual Result setParameter(const int &number, const Bstring &value)
    {
        switch (number) {
            case 0:
                tilenum_ = atoi(value());
                return ERR_NO_ERROR;
            default:
                return ERR_TOO_MANY_PARAMS;
        }
    }

    virtual Result perform()
    {
        int filenum = 0;

        // open art files until we find one that encompasses the range we need
        // and when we find it, remove the tile
        for (filenum = 0; filenum < 1000; filenum++) {
            ARTFile art(makefilename(filenum));

            if (art.getNumTiles() == 0) {
                // no file exists, so give up
                break;
            }

            if (tilenum_ >= art.getFirstTile() && tilenum_ <= art.getLastTile()) {
                art.removeTile(tilenum_);
                art.write();
                return ERR_NO_ERROR;
            }
        }

        return ERR_NO_ART_FILE;
    }
};

class TilePropOp : public Operation {
private:
    int xofs_, yofs_;
    int animframes_, animtype_, animspeed_;
    int tilenum_;

    int settings_;

    enum {
        SET_XOFS = 1,
        SET_YOFS = 2,
        SET_ANIMFRAMES = 4,
        SET_ANIMTYPE = 8,
        SET_ANIMSPEED = 16,
    };
public:
    TilePropOp()
    : xofs_(0), yofs_(0),
      animframes_(0), animtype_(0), animspeed_(0),
      tilenum_(-1), settings_(0)
    { }

    virtual Result setOption(const Bstring &opt, const Bstring &value)
    {
        if (opt == "x") {
            xofs_ = atoi(value());
            settings_ |= SET_XOFS;
        } else if (opt == "y") {
            yofs_ = atoi(value());
            settings_ |= SET_YOFS;
        } else if (opt == "ann") {
            animframes_ = atoi(value());
            settings_ |= SET_ANIMFRAMES;
            if (animframes_ < 0 || animframes_ > 63) {
                return ERR_BAD_VALUE;
            }
        } else if (opt == "ant") {
            animtype_ = atoi(value());
            settings_ |= SET_ANIMTYPE;
            if (animtype_ < 0 || animtype_ > 3) {
                return ERR_BAD_VALUE;
            }
        } else if (opt == "ans") {
            animspeed_ = atoi(value());
            settings_ |= SET_ANIMSPEED;
            if (animspeed_ < 0 || animspeed_ > 15) {
                return ERR_BAD_VALUE;
            }
        } else {
            return ERR_BAD_OPTION;
        }
        return ERR_NO_ERROR;
    }

    virtual Result setParameter(const int &number, const Bstring &value)
    {
        switch (number) {
            case 0:
                tilenum_ = atoi(value());
                return ERR_NO_ERROR;
            default:
                return ERR_TOO_MANY_PARAMS;
        }
    }

    virtual Result perform()
    {
        int filenum = 0;

        if (settings_ == 0) {
            return ERR_NO_ERROR;
        }

        // open art files until we find one that encompasses the range we need
        // and when we find it, make the change
        for (filenum = 0; filenum < 1000; filenum++) {
            ARTFile art(makefilename(filenum));

            if (art.getNumTiles() == 0) {
                // no file exists, so give up
                break;
            }

            if (tilenum_ >= art.getFirstTile() && tilenum_ <= art.getLastTile()) {
                if (settings_ & SET_XOFS) {
                    art.setXOfs(tilenum_, xofs_);
                }
                if (settings_ & SET_YOFS) {
                    art.setYOfs(tilenum_, yofs_);
                }
                if (settings_ & SET_ANIMFRAMES) {
                    art.setAnimFrames(tilenum_, animframes_);
                }
                if (settings_ & SET_ANIMSPEED) {
                    art.setAnimSpeed(tilenum_, animspeed_);
                }
                if (settings_ & SET_ANIMTYPE) {
                    art.setAnimType(tilenum_, animtype_);
                }
                art.write();
                return ERR_NO_ERROR;
            }
        }

        return ERR_NO_ART_FILE;
    }
};

int main(int argc, char ** argv)
{
    int showusage = 0;
    Operation * oper = 0;
    Operation::Result err = Operation::ERR_NO_ERROR;

    if (argc < 2) {
        showusage = 1;
    } else {
        Bstring opt(argv[1]);
        Bstring value;

        // create the option handler object according to the first param
        if (opt == "create") {
            oper = new CreateOp;
        } else if (opt == "addtile") {
            oper = new AddTileOp;
        } else if (opt == "rmtile") {
            oper = new RmTileOp;
        } else if (opt == "tileprop") {
            oper = new TilePropOp;
        } else {
            showusage = 2;
        }

        // apply the command line options given
        if (oper) {
            int unnamedParm = 0;
            for (int i = 2; i < argc && !showusage; ++i) {
                if (argv[i][0] == '-') {
                    opt = argv[i] + 1;
                    if (i+1 >= argc) {
                        showusage = 2;
                        break;
                    }
                    value = argv[i+1];
                    ++i;

                    switch (err = oper->setOption(opt, value)) {
                        case Operation::ERR_NO_ERROR: break;
                        default:
                            Bfprintf(stderr, "error: %s\n", Operation::translateResult(err));
                            showusage = 2;
                            break;
                    }
                } else {
                    value = argv[i];
                    switch (oper->setParameter(unnamedParm, value)) {
                        case Operation::ERR_NO_ERROR: break;
                        default:
                            Bfprintf(stderr, "error: %s\n", Operation::translateResult(err));
                            showusage = 2;
                            break;
                    }
                    ++unnamedParm;
                }
            }
        }
    }

    if (showusage) {
        usage();
        if (oper) delete oper;
        return (showusage - 1);
    } else if (oper) {
        err = oper->perform();
        delete oper;

        switch (err) {
            case Operation::ERR_NO_ERROR: return 0;
            default:
                Bfprintf(stderr, "error: %s\n", Operation::translateResult(err));
                return 1;
        }
    }

    return 0;
}
