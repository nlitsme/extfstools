/*
 *  Author: Willem Hengeveld <itsme@xs4all.nl>
 */
#include <stdio.h>
#include <map>
#include <vector>
#include <list>
#include <algorithm>
#include <functional>
#include <string>
#include <ctime>                // gmtime, strftime
#include <string.h>              // memcpy
#ifdef USE_CPPUTILS
#include "mmfile.h"
#include "util/rw/MemoryReader.h"
#else
#include "util/rw/MmapReader.h"
#endif
#include "util/rw/FileReader.h"
#include "args.h"
#include <sys/stat.h>

#ifdef _WIN32
void lutimes(const char*path, const struct timeval *times)
{
}
int symlink(const char *src, const char *dst)
{
    return 0;
}
int mkdir(const char *src, int mode)
{
    return 0;
}
#endif

std::string timestr(uint32_t t32)
{
    time_t t = t32;
    struct tm tm = *std::gmtime(&t);
    char str[40];
    std::strftime(str, 40, "%Y-%m-%d %H:%M:%S", &tm);

    return str;
}
//  ~/gitprj/repos/linux/fs/ext2/ext2.h
//  todo: add 64bit support from ext4

// todo: create ExtentsFileReader and BlocksFileReader
struct SuperBlock  {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;     // 0 or 1
    uint32_t s_log_block_size;       // blocksize = 1024<<s_log_block_size
    int32_t s_log_frag_size;         // fragsize = 1024<<s_log_frag_size
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;            // ef53
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;

        uint32_t s_first_ino;
        uint16_t s_inode_size;
        uint16_t s_block_group_nr;
        uint32_t s_feature_compat;
        uint32_t s_feature_incompat;
        uint32_t s_feature_ro_compat;
        uint8_t s_uuid[16];
        uint8_t s_volume_name[16];
        uint8_t s_last_mounted[64];
        uint32_t s_algo_bitmap;

    const uint8_t *fsbase;

    void parse(const uint8_t *first, size_t size)
    {
        const uint8_t *p= first;

        s_inodes_count = get32le(p);      p+=4;    // 0000;
        s_blocks_count = get32le(p);      p+=4;    // 0004;
        s_r_blocks_count = get32le(p);    p+=4;    // 0008;
        s_free_blocks_count = get32le(p); p+=4;    // 000c;
        s_free_inodes_count = get32le(p); p+=4;    // 0010;
        s_first_data_block = get32le(p);  p+=4;    // 0014;
        s_log_block_size = get32le(p);    p+=4;    // 0018;
        s_log_frag_size = get32le(p);     p+=4;    // 001c;
        s_blocks_per_group = get32le(p);  p+=4;    // 0020;
        s_frags_per_group = get32le(p);   p+=4;    // 0024;
        s_inodes_per_group = get32le(p);  p+=4;    // 0028;
        s_mtime = get32le(p);             p+=4;    // 002c;
        s_wtime = get32le(p);             p+=4;    // 0030;
        s_mnt_count = get16le(p);         p+=2;    // 0034;
        s_max_mnt_count = get16le(p);     p+=2;    // 0036;
        s_magic = get16le(p);             p+=2;    // 0038;
        s_state = get16le(p);             p+=2;    // 003a;
        s_errors = get16le(p);            p+=2;    // 003c;
        s_minor_rev_level = get16le(p);   p+=2;    // 003e;
        s_lastcheck = get32le(p);         p+=4;    // 0040;
        s_checkinterval = get32le(p);     p+=4;    // 0044;
        s_creator_os = get32le(p);        p+=4;    // 0048;
        s_rev_level = get32le(p);         p+=4;    // 004c;
        s_def_resuid = get16le(p);        p+=2;    // 0050;
        s_def_resgid = get16le(p);        p+=2;    // 0052;
                                                   
        s_first_ino= get32le(p);          p+=4;    // 0054;
        s_inode_size= get16le(p);         p+=2;    // 0058;
        s_block_group_nr= get16le(p);     p+=2;    // 005a;
        s_feature_compat= get32le(p);     p+=4;    // 005c;
        s_feature_incompat= get32le(p);   p+=4;    // 0060;
        s_feature_ro_compat= get32le(p);  p+=4;    // 0064;
        memcpy(s_uuid, p, 16);            p+=16;   // 0068;
        memcpy(s_volume_name, p, 16);     p+=16;   // 0078;
        memcpy(s_last_mounted, p, 64);    p+=64;   // 0088;
        s_algo_bitmap= get32le(p);        p+=4;    // 00c8;
    }
    uint64_t blocksize() const { return 1024<<s_log_block_size; }
    uint64_t fragsize() const { 
        if (s_log_frag_size <0)
            return 1024>>(-s_log_frag_size);
        else
            return 1024<<s_log_frag_size; 
    }
    uint64_t bytespergroup() const { return s_blocks_per_group*blocksize(); }
    size_t ngroups() const { return s_inodes_count/s_inodes_per_group; }

    void dump() const
    {
        printf("s_inodes_count=%08x\n", s_inodes_count);
        printf("s_blocks_count=%08x\n", s_blocks_count);
        printf("s_r_blocks_count=%08x\n", s_r_blocks_count);
        printf("s_free_blocks_count=%08x\n", s_free_blocks_count);
        printf("s_free_inodes_count=%08x\n", s_free_inodes_count);
        printf("s_first_data_block=%08x\n", s_first_data_block);
        printf("s_log_block_size=%d\n", s_log_block_size);
        printf("s_log_frag_size=%d\n", s_log_frag_size);
        printf("s_blocks_per_group=%08x\n", s_blocks_per_group);
        printf("s_frags_per_group=%08x\n", s_frags_per_group);
        printf("s_inodes_per_group=%08x\n", s_inodes_per_group);
        printf("s_mtime=%08x\n", s_mtime);
        printf("s_wtime=%08x\n", s_wtime);
        printf("s_mnt_count=%04x\n", s_mnt_count);
        printf("s_max_mnt_count=%04x\n", s_max_mnt_count);
        printf("s_magic=%04x\n", s_magic);
        printf("s_state=%04x\n", s_state);
        printf("s_errors=%04x\n", s_errors);
        printf("s_minor_rev_level=%04x\n", s_minor_rev_level);
        printf("s_lastcheck=%08x\n", s_lastcheck);
        printf("s_checkinterval=%08x\n", s_checkinterval);
        printf("s_creator_os=%08x\n", s_creator_os);
        printf("s_rev_level=%08x\n", s_rev_level);
        printf("s_def_resuid=%04x\n", s_def_resuid);
        printf("s_def_resgid=%04x\n", s_def_resgid);

        printf("i->%d, b->%d groups\n", s_inodes_count/s_inodes_per_group, (s_blocks_count+s_blocks_per_group-1)/s_blocks_per_group);
        printf("s_first_ino=%d\n", s_first_ino);
        printf("s_inode_size=%d\n", s_inode_size);
        printf("s_block_group_nr=%d\n", s_block_group_nr);
        printf("s_feature_compat=%08x\n", s_feature_compat);
        printf("s_feature_incompat=%08x\n", s_feature_incompat);
        printf("s_feature_ro_compat=%08x\n", s_feature_ro_compat);
        printf("s_uuid=%s\n", hexdump(s_uuid, 16).c_str());
        printf("s_volume_name=%s\n", ascdump(s_volume_name, 16).c_str());
        printf("s_last_mounted=%s\n", ascdump(s_last_mounted, 16).c_str());
        printf("s_algo_bitmap=%08x\n", s_algo_bitmap);
    }

    const uint8_t *getptrforblock(int n) const
    {
        if (n>=s_blocks_count)
            return NULL;
        return blocksize()*n+fsbase;
    }
};
typedef std::function<bool(const uint8_t*)> BLOCKCALLBACK;
struct ExtentNode {
    virtual ~ExtentNode() { }
    virtual void parse(const uint8_t *first)= 0;
    virtual void dump() const= 0;

    virtual bool enumblocks(const SuperBlock &super, BLOCKCALLBACK cb) const= 0;
};
struct ExtentLeaf : ExtentNode {
    uint32_t ee_block;
    uint16_t ee_len;
    uint16_t ee_start_hi;
    uint32_t ee_start_lo;

    ExtentLeaf(const uint8_t*first)
    {
        parse(first);
    }
    virtual void parse(const uint8_t *first)
    {
        const uint8_t *p= first;
        ee_block        = get32le(p); p+=4; 
        ee_len          = get16le(p); p+=2; 
        ee_start_hi     = get16le(p); p+=2; 
        ee_start_lo     = get32le(p); p+=4; 
    }
    virtual void dump() const
    {
        printf("blk:%08x, l=%d, %010llx\n", ee_block, ee_len, (unsigned long long)startblock());
    }
    uint64_t startblock() const
    {
        return (uint64_t(ee_start_hi)<<32)|ee_start_lo;
    }
    virtual bool enumblocks(const SuperBlock &super, BLOCKCALLBACK cb) const
    {
        uint64_t blk= startblock();
        for (int i=0 ; i<ee_len ; i++)
            if (!cb(super.getptrforblock(blk++)))
                return false;
        return true;
    }
};
struct ExtentInternal : ExtentNode {
    uint32_t ei_block;
    uint32_t ei_leaf_lo;
    uint16_t ei_leaf_hi;
    uint16_t ei_unused;

    ExtentInternal(const uint8_t*first)
    {
        parse(first);
    }

    virtual void parse(const uint8_t *first)
    {
        const uint8_t *p= first;
        ei_block        = get32le(p); p+=4; 
        ei_leaf_lo      = get32le(p); p+=4; 
        ei_leaf_hi      = get16le(p); p+=2; 
        ei_unused       = get16le(p); p+=2; 
    }
    virtual void dump() const
    {
        printf("blk:%08x, [%d] %010llx\n", ei_block, ei_unused, (unsigned long long)leafnode());
    }
    uint64_t leafnode() const
    {
        return (uint64_t(ei_leaf_hi)<<32)|ei_leaf_lo;
    }

    virtual bool enumblocks(const SuperBlock &super, BLOCKCALLBACK cb) const;

};
struct ExtentHeader {
	uint16_t eh_magic;	/* probably will support different formats */
	uint16_t eh_entries;	/* number of valid entries */
	uint16_t eh_max;		/* capacity of store in entries */
	uint16_t eh_depth;	/* has tree real underlying blocks? */
	uint32_t eh_generation;	/* generation of the tree */

    void parse(const uint8_t *first)
    {
        const uint8_t *p= first;
        eh_magic	= get16le(p); p+=2;
        eh_entries	= get16le(p); p+=2;
        eh_max		= get16le(p); p+=2;
        eh_depth	= get16le(p); p+=2;
        eh_generation= get32le(p);p+=4;
    }
    void dump() const
    {
        printf("M%04x, N%d,max%d, d:%d, g:%d\n", eh_magic, eh_entries, eh_max, eh_depth, eh_generation);
    }
};

struct Extent {
    ExtentHeader eh;
    std::vector< std::shared_ptr<ExtentNode> > extents;

    void parse(const uint8_t *first)
    {
        const uint8_t *p= first;
        eh.parse(p);   p+=12;

//      if (eh.eh_magic != 0xf30a)
//          throw "invalid extent hdr magic";

        for (int i=0 ; i<eh.eh_entries ; i++) {
            if (eh.eh_depth==0)
                extents.push_back(std::make_shared<ExtentLeaf>(p));
            else
                extents.push_back(std::make_shared<ExtentInternal>(p));
            p+=12;
        }
    }
    virtual bool enumblocks(const SuperBlock &super, BLOCKCALLBACK cb) const
    {
        for (int i=0 ; i<eh.eh_entries ; i++)
            if (!extents[i]->enumblocks(super, cb))
                return false;
        return true;
    }

    void dump() const
    {
        eh.dump();
        for (int i=0 ; i<extents.size() ; i++) {
            printf("EXT %d: ", i);
            extents[i]->dump();
        }
    }

};
bool ExtentInternal::enumblocks(const SuperBlock &super, BLOCKCALLBACK cb) const
{
    Extent e;
    e.parse(super.getptrforblock(leafnode()));

    return e.enumblocks(super, cb);
}

/*
 mode  uid     size    atime    ctime    mtime    dtime  gid  lnk   blocks   flags     osd1     block                       ext0                       ext1                       ext2                       ext3               gen  fileacl  diracl   faddr
    0    2        4        8        c       10       14   18   1a       1c       20       24       28       2c       30       34       38       3c       40       44       48       4c       50       54       58       5c       60       64       68       6c       70       74       78       7c       80       84       88       8c       90       94  98
 41fd 03e8 00001000 00000000 52578d78 52578d78 00000000 03e8 0008 00000008 00080080 00000022 0001f30a 00000003 00000000 00000000 00000001 00000614 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 46555f10 46555f10 00000000 00000000 00000000 ...
 8180 0000 03c0c000 00000000 00000000 00000000 00000000 0000 0001 000000f8 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000613 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 ...
 8180 0000 00400000 00000000 00000000 00000000 00000000 0000 0001 00002000 00080000 00000000 0001f30a 00000003 00000000 00000000 00000400 00000213 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 ...
 41fd 03e8 00001000 4effa236 52578d78 4effa236 00000000 03e8 0002 00000008 00080080 00000001 0001f30a 00000004 00000000 00000000 00000001 00008213 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 b15c6b45 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 46555f10 405f7e0c 405f7e0c 4effa236 405f7e0c ...
 41fd 03e8 00001000 4effa236 52578d78 4effa236 00000000 03e8 0002 00000008 00080080 00000001 0001f30a 00000004 00000000 00000000 00000001 00008214 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 b15c6b46 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 46555f10 405f7e0c 405f7e0c 4effa236 405f7e0c ...
 41fd 03e8 00001000 4effa236 52578d78 4effa236 00000000 03e8 0002 00000008 00080080 00000001 0001f30a 00000004 00000000 00000000 00000001 00008215 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 b15c6b47 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 46555f10 405f7e0c 405f7e0c 4effa236 405f7e0c ...
 41fd 03e8 00001000 4effa236 52578d78 4effa236 00000000 03e8 0002 00000008 00080080 00000001 0001f30a 00000004 00000000 00000000 00000001 00008216 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 b15c6b48 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 46555f10 4190ab0c 4190ab0c 4effa236 4190ab0c ...
 41f9 03e8 00001000 4effa236 52578d78 4effa238 00000000 03e8 0002 00000008 00080080 00000006 0001f30a 00000004 00000000 00000000 00000001 00008217 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 b15c6b49 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 46555f10 62f19710 4190ab0c 4effa236 4190ab0c ...
 41f9 03e8 00001000 52578d78 52578d78 52578d78 00000000 03e8 0002 00000008 00080080 00000001 0001f30a 00000004 00000000 00000000 00000001 0000851b 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 1903bd1f 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 46555f10 46555f10 46555f10 52578d78 46555f10 ...
 81b6 03e8 00300000 4effa236 52578d7f 52578d7f 00000000 03e9 0001 00001800 00080080 00000001 0001f30a 00000004 00000000 00000000 00000300 00002c00 00000200 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 b15c6b4b 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 5bca891c 5bca891c 99c7ad0c 4effa236 99c7ad0c ...
 8180 03e8 00300028 4effa236 52578d7e 52578d7e 00000000 03e9 0001 00001808 00080080 00000001 0001f30a 00000004 00000000 00000000 00000301 00001200 00000200 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 b15c6b4c 00000000 00000000 00000000 00000000 00000000 00000000 0000001c c7145b1c c7145b1c db58580c 4effa236 db58580c ...
 8180 03e8 00300028 4effa237 52578d7f 52578d7f 00000000 03e9 0001 00001808 00080080 00000001 0001f30a 00000004 00000000 00000000 00000301 00002800 00000200 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 b15c6b4d 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 2aea541c 2aea541c d7c4d110 4effa237 d7c4d110 ...
 8180 03e8 00300028 4effa238 52578d7f 52578d7f 00000000 03e9 0001 00001808 00080080 00000001 0001f30a 00000004 00000000 00000000 00000301 00003000 00000200 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 b15c6b4e 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 7866c11c 7866c11c 3b9aca10 4effa238 3b9aca10 ...
 8180 03e8 00200028 4effa238 52578d7f 52578d7f 00000000 03e9 0001 00001008 00080080 00000001 0001f30a 00000004 00000000 00000000 00000201 00003400 00000200 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 b15c6b4f 00000000 00000000 00000000 00000000 00000000 00000000 0000001c 9896801c 9896801c 62f19710 4effa238 62f19710 ...

*/

#define EXT4_EXTENTS_FL 0x80000

#define EXT4_S_IFLNK 0xa000 
#define EXT4_S_IFDIR 0x4000

#define EXT4_FT_UNKNOWN 0
#define EXT4_FT_REG_FILE 1
#define EXT4_FT_DIR 2

#define ROOTDIRINODE 2
struct Inode {
    uint16_t i_mode;       //  000
    uint16_t i_uid;        //  002
    uint32_t i_size;       //  004
    uint32_t i_atime;      //  008
    uint32_t i_ctime;      //  00c
    uint32_t i_mtime;      //  010
    uint32_t i_dtime;      //  014
    uint16_t i_gid;        //  018
    uint16_t i_links_count;//  01a
    uint32_t i_blocks;     //  01c// 512 byte blocks
    uint32_t i_flags;      //  020
    uint32_t i_osd1;       //  024

    uint32_t i_block[15];  //  028

    std::string  symlink;
    Extent   e;

    uint32_t i_generation; //  064
    uint32_t i_file_acl;   //  068
    uint32_t i_dir_acl;    //  06c
    uint32_t i_faddr;      //  070
    uint8_t i_osd2[12];    //  074

    bool _empty;
    void parse(const uint8_t *first, size_t size)
    {
        _empty = std::find_if(first, first+size, [](uint8_t b) { return b!=0; })==first+size;

        const uint8_t *p= first;
        i_mode= get16le(p);  p+=2;
        i_uid= get16le(p);  p+=2;
        i_size= get32le(p);  p+=4;
        i_atime= get32le(p);  p+=4;
        i_ctime= get32le(p);  p+=4;
        i_mtime= get32le(p);  p+=4;
        i_dtime= get32le(p);  p+=4;
        i_gid= get16le(p);  p+=2;
        i_links_count= get16le(p);  p+=2;
        i_blocks= get32le(p);  p+=4;
        i_flags= get32le(p);  p+=4;
        i_osd1= get32le(p);  p+=4;
        if (issymlink()) {
            symlink= std::string((const char*)p, 60);
            p+=60;
        }
        else if (i_flags&EXT4_EXTENTS_FL) {
            e.parse(p);      p+=60;
        }
        else {
            for (int i=0 ; i<15 ; i++) {
                i_block[i]= get32le(p);  p+=4;
            }
        }
        i_generation= get32le(p);  p+=4;
        i_file_acl= get32le(p);  p+=4;
        i_dir_acl= get32le(p);  p+=4;
        i_faddr= get32le(p);  p+=4;
        memcpy(i_osd2, p, 12); p+=12;
    }

    bool issymlink() const {
      return (i_mode&0xf000)==EXT4_S_IFLNK && i_size<60;
    }

    void dump() const
    {
        printf("m:%06o %4d o[%5d %5d] t[%10d %10d %10d %10d]  %12lld [b:%8d] F:%05x X:%08x %s\n",
                i_mode, i_links_count, i_gid, i_uid, i_atime, i_ctime, i_mtime, i_dtime, (unsigned long long)datasize(), i_blocks, i_flags, i_file_acl, hexdump(i_osd2, 12).c_str());
        if (issymlink()) {
            printf("symlink: %s\n", symlink.c_str());
        }
        else if (i_flags&EXT4_EXTENTS_FL) {
            e.dump();
        }
        else {
            printf("blocks: ");
            for (int i=0 ; i<12 ; i++)
                printf(" %08x", i_block[i]);
            printf("  i1:%08x, i2:%08x, i3:%08x\n", i_block[12], i_block[13], i_block[14]);
        }
    }
    uint64_t datasize() const {
        return uint64_t(i_size); // +(uint64_t(i_dir_acl)<<32);
    }

    bool enumblocks(const SuperBlock &super, BLOCKCALLBACK cb) const
    {
        if (issymlink()) {
            // no blocks
        }
        else if (i_flags&EXT4_EXTENTS_FL) {
            return enumextents(super, cb);
        }
        uint64_t bytes= 0;
        for (int i=0 ; i<12 && bytes < i_size ; i++) {
            if (i_block[i]) {
                if (!cb(super.getptrforblock(i_block[i])))
                    return false;
                bytes += super.blocksize();
            }
        }
        if (i_block[12])
            if (!enumi1block(super, bytes, super.getptrforblock(i_block[12]), cb))
                return false;
        if (i_block[13])
            if (!enumi2block(super, bytes, super.getptrforblock(i_block[13]), cb))
                return false;
        if (i_block[14])
            if (!enumi3block(super, bytes, super.getptrforblock(i_block[14]), cb))
                return false;
        return true;
    }
    bool enumi1block(const SuperBlock &super, uint64_t& bytes, const uint8_t *pblock, BLOCKCALLBACK cb) const
    {
        if (pblock==NULL)
            return true;
        const uint32_t *p= (const uint32_t *)pblock;
        const uint32_t *last= (const uint32_t *)(pblock+super.blocksize());
        while (p<last && bytes < i_size)
        {
            uint32_t blocknr= *p++;
            if (blocknr)
                if (!cb(super.getptrforblock(blocknr)))
                    return false;
            bytes += super.blocksize();
        }
        return true;
    }
    bool enumi2block(const SuperBlock &super, uint64_t& bytes, const uint8_t *pblock, BLOCKCALLBACK cb) const
    {
        if (pblock==NULL)
            return true;
        const uint32_t *p= (const uint32_t *)pblock;
        const uint32_t *last= (const uint32_t *)(pblock+super.blocksize());
        while (p<last && bytes < i_size)
            if (!enumi1block(super, bytes, super.getptrforblock(*p++), cb))
                return false;
        return true;
    }
    bool enumi3block(const SuperBlock &super, uint64_t& bytes, const uint8_t *pblock, BLOCKCALLBACK cb) const
    {
        if (pblock==NULL)
            return true;
        const uint32_t *p= (const uint32_t *)pblock;
        const uint32_t *last= (const uint32_t *)(pblock+super.blocksize());
        while (p<last && bytes < i_size)
            if (!enumi2block(super, bytes, super.getptrforblock(*p++), cb))
                return false;
        return true;
    }

    bool enumextents(const SuperBlock &super, BLOCKCALLBACK cb) const
    {
        return e.enumblocks(super, cb);
    }
    static void rwx(char *str, int bits, bool extra, char xchar)
    {
        str[0] = (bits&4) ? 'r' : '-';
        str[1] = (bits&2) ? 'w' : '-';
        if (extra) 
            str[2] = (bits&1) ? (xchar&~0x20) : (xchar|0x20);
        else
            str[2] = (bits&1) ? 'x' : '-';
    }
    std::string modestr() const
    {
        std::string result(10, ' ');
        const char *typechar = "?pc?d?b?-?l?s???";

        result[0] = typechar[i_mode>>12];

        rwx(&result[1], (i_mode>>6)&7, (i_mode>>11)&1, 's');
        rwx(&result[4], (i_mode>>3)&7, (i_mode>>10)&1, 's');
        rwx(&result[7], i_mode&7, (i_mode>>9)&1, 't');

        return result;
    }
};

struct BlockGroupDescriptor {

    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;

    void parse(const uint8_t *first)
    {
        const uint8_t *p= first;

        bg_block_bitmap= get32le(p);  p+=4;
        bg_inode_bitmap= get32le(p);  p+=4;
        bg_inode_table= get32le(p);  p+=4;
        bg_free_blocks_count= get16le(p);  p+=2;
        bg_free_inodes_count= get16le(p);  p+=2;
        bg_used_dirs_count= get16le(p);  p+=2;
        bg_pad= get16le(p);  p+=2;

        if (p-first!=20)
            printf("bgdesc size err: %ld\n", p-first);
    }
    void dump() const
    {
        printf("block group B:%d, I:%d, T:%d, free:B=%d, I=%d, used:D=%d\n",
            bg_block_bitmap, bg_inode_bitmap, bg_inode_table,
            bg_free_blocks_count, bg_free_inodes_count, bg_used_dirs_count);
    }
};
struct Bitmap {
    const uint8_t *bits;
    size_t size;

    void parse(const uint8_t *first, size_t bitmapsize)
    {
        bits= first;
        size= bitmapsize;
    }
};
struct BlockGroup {
    Bitmap bbitmap;
    Bitmap ibitmap;
    std::vector<Inode> inodetable;

    void parse(const uint8_t *first, const SuperBlock &super, BlockGroupDescriptor& bgdesc)
    {
        bbitmap.parse(first+(bgdesc.bg_block_bitmap%super.s_blocks_per_group)*super.blocksize(), super.blocksize());
        ibitmap.parse(first+(bgdesc.bg_inode_bitmap%super.s_blocks_per_group)*super.blocksize(), super.blocksize());
        parseitable(first+(bgdesc.bg_inode_table%super.s_blocks_per_group)*super.blocksize(), super.s_inodes_per_group, super.s_inode_size);
    }
    void parseitable(const uint8_t *first, size_t ninodes, size_t inodesize)
    {
        const uint8_t *p = first;
        inodetable.resize(ninodes);
        for (unsigned i=0 ; i<ninodes ; i++)
        {
            inodetable[i].parse(p, inodesize);
            p += inodesize;
        }

    }

    void dump(const SuperBlock &super, uint32_t inodebase) const
    {
        enuminodes(inodebase, [&](int32_t nr, const Inode& ino) {
            printf("INO %d: ", nr);
            ino.dump();
            if ((ino.i_mode&0xf000)==EXT4_S_IFDIR)
                ino.enumblocks(super, [&](const uint8_t *p)->bool {
                    if (p==NULL) {
                        printf("invalid blocknr\n");
                        return true;
                    }
                    BlockGroup::dumpdirblock(p, super.blocksize());
                    return true;
                });
        });
    }
    template<typename FUNC>
    void enuminodes(uint32_t inodebase, FUNC cb) const
    {
        for (unsigned i=0 ; i<inodetable.size() ; i++)
            if (!inodetable[i]._empty)
                cb(inodebase+i, inodetable[i]);
    }
    static size_t dumpdirent(const uint8_t *first)
    {
        const uint8_t *p= first;
        uint32_t inode= get32le(p);  p+=4;
        uint32_t rec_len= get16le(p);p+=2;
        uint32_t name_len= get8(p);  p+=1;
        uint32_t filetype= get8(p);  p+=1;
        printf("%8d %o '%s'\n", inode, filetype, std::string((const char*)p, name_len).c_str());
        return rec_len;
    }
    static void dumpdirblock(const uint8_t *first, size_t blocksize)
    {
        //printf("%s\n", hexdump(first, blocksize).c_str());
        const uint8_t *p= first;
        while (p<first+blocksize) {
            size_t n= dumpdirent(p);
            if (n==0)
                break;
            p+=n;
        }
    }
};

struct Ext2FileSystem {
    SuperBlock super;
    std::vector<BlockGroupDescriptor> bgdescs;

    std::vector<BlockGroup> groups;

    Ext2FileSystem() { }

    void parse(const uint8_t *first)
    {

        super.fsbase= first;
        super.parse(first+1024, 1024);

        parsegroupdescs(first+super.blocksize(), super.ngroups());

        groups.resize(super.ngroups());
        const uint8_t *p= first;
        for (int i=0 ; i<super.ngroups() ; i++) {
            groups[i].parse(p, super, bgdescs[i]);
            p+=super.bytespergroup();
        }
    }
    void parsegroupdescs(const uint8_t *first, size_t ngroups)
    {
        bgdescs.resize(ngroups);
        const uint8_t *p= first;
        for (unsigned i=0 ; i<ngroups ; i++) {
            bgdescs[i].parse(p);
            p+=32;
        }
    }
    void dump() const
    {
        super.dump();
        uint32_t inodebase= 1;
        for (unsigned i=0 ; i<groups.size() ; i++) {

            bgdescs[i].dump();

            groups[i].dump(super, inodebase);
            inodebase+=super.s_inodes_per_group;
        }
    }

    template<typename FUNC>
    void enuminodes(FUNC cb) const
    {
        uint32_t inodebase= 1;
        for (unsigned i=0 ; i<groups.size() ; i++) {
            groups[i].enuminodes(inodebase, cb);
            inodebase+=super.s_inodes_per_group;
        }
    }
    const Inode& getinode(uint32_t nr) const
    {
        nr--;
        if (nr>=super.s_inodes_count)
            nr= 0;
        return groups[nr/super.s_inodes_per_group].inodetable[nr%super.s_inodes_per_group];
    }
};

struct DirectoryEntry {
    uint32_t inode;
    uint8_t  filetype;  // 1=file, 2=dir, 3=chardev, 4=blockdev, 5=fifo, 6=sock, 7=symlink
    std::string name;
    size_t parse(const uint8_t *first)
    {
        const uint8_t *p= first;
        inode= get32le(p);  p+=4;
        uint32_t rec_len= get16le(p);p+=2;
        uint32_t name_len= get8(p);  p+=1;
        filetype= get8(p);  p+=1;
        name= std::string((const char*)p, name_len);
        return rec_len;
    }
    void dump() const
    {
        printf("%8d %o '%s'\n", inode, filetype, name.c_str());
    }
};

// currently unused.  but i could use this sometime
// to find lost inodes
struct TreeReconstructor {
    std::map<uint32_t,std::vector<DirectoryEntry> >  dirmap;
    std::map<uint32_t,uint8_t >  nodetypes;

    void scanfs(Ext2FileSystem& fs)
    {
        fs.enuminodes([this, &fs](uint32_t nr, const Inode& inode) {
            nodetypes.emplace(nr, (inode.i_mode&0xf000)>>12);
            if ((inode.i_mode&0xf000)==EXT4_S_IFDIR) {
                auto &dir= dirmap[nr];
                inode.enumblocks(fs.super, [&fs, &dir](const uint8_t *first)->bool {
                    if (first==NULL)
                        return true;
                    const uint8_t *p= first;
                    while (p<first+fs.super.blocksize()) {
                        dir.resize(dir.size()+1);
                        
                        size_t n= dir.back().parse(p);
                        if (n==0)
                            break;
                        p+=n;
                    }
                    return true;
                });
            }
        });
    }

    template<typename FN>
    void recursedirs(uint32_t nr, std::string path, FN f)
    {
        for (auto const &e : dirmap[nr])
        {
            if (e.name=="." || e.name=="..")
                continue;
            f(e, path);
            if (e.filetype==2/* && e.name!="." && e.name!=".." */)
                recursedirs(e.inode, path+"/"+e.name, f);
        }
    }
};

template<typename FN>
void recursedirs(Ext2FileSystem&fs, uint32_t nr, std::string path, FN f)
{
    const Inode &i= fs.getinode(nr);
    if ((i.i_mode&0xf000)!=EXT4_S_IFDIR)
        return;
    i.enumblocks(fs.super, [&](const uint8_t *first)->bool {
        if (first==NULL)
            return true;
        const uint8_t *p= first;
        while (p<first+fs.super.blocksize()) {
            DirectoryEntry e;
            size_t n= e.parse(p);
            p+=n;
            if (n==0)
                break;
            // last dir record has type=0 && inde=0, 
            // size is rest of block
            if (e.filetype==EXT4_FT_UNKNOWN)
                continue;

            if (e.name=="." || e.name=="..")
                continue;

            f(e, path);
            if (e.filetype==EXT4_FT_DIR)
                recursedirs(fs, e.inode, path+"/"+e.name, f);
        }
        return true;
    });
}

uint32_t searchpath(Ext2FileSystem&fs, uint32_t nr, std::string path)
{
    const Inode &i= fs.getinode(nr);
    if ((i.i_mode&0xf000)!=EXT4_S_IFDIR) {
        printf("#%d is not a dir\n", nr);
        return 0;
    }
    uint32_t found= 0;
    i.enumblocks(fs.super, [&] (const uint8_t *first)->bool {
        if (first==NULL)
            return true;
        const uint8_t *p= first;
        while (p<first+fs.super.blocksize()) {
            DirectoryEntry e;
            size_t n= e.parse(p);
            p+=n;
            if (n==0)
                break;
            if (e.filetype==EXT4_FT_UNKNOWN)
                continue;

            if (path.size()<e.name.size())
                continue;

            if (path.size()==e.name.size() && path==e.name)
            {
                found= e.inode;
                return false;
            }
            // path.size() > e.name.size()
            if (path[e.name.size()]=='/' && std::equal(e.name.begin(), e.name.end(), path.begin())) {
                if (e.filetype==EXT4_FT_DIR) {
                    found= searchpath(fs, e.inode, path.substr(e.name.size()+1));
                    return false;
                }
                else {
                    printf("Not a directory: %s\n", path.c_str());
                    return false;
                }
            }
        }
        return true;
    });
    return found;
}
// =============================================================================
struct action {
    typedef std::shared_ptr<action> ptr;
    static ptr parse(const std::string& arg);
    virtual ~action() { }

    virtual void perform(Ext2FileSystem &fs)=0;
};
struct hexdumpinode : action {
    uint32_t nr;
    hexdumpinode(uint32_t nr) : nr(nr) { }

    void perform(Ext2FileSystem &fs) override
    {
        const Inode &i= fs.getinode(nr);
        if (!i._empty) {
            uint64_t ofs= 0;
            i.enumblocks(fs.super, [&](const uint8_t *first)->bool {
                printf("%s\n", hexdump(ofs, first, fs.super.blocksize()).c_str());
                ofs += fs.super.blocksize();
                return true;
            });
        }
    }
};
struct exportinode : action {
    uint32_t nr;
    std::string savepath;

    exportinode(uint32_t nr, const std::string& savepath) : nr(nr), savepath(savepath) { }

    void perform(Ext2FileSystem &fs) override
    {
        const Inode &i= fs.getinode(nr);
        if (!i._empty) {
            FileReader w(savepath, FileReader::opencreate);
            i.enumblocks(fs.super, [&](const uint8_t *first)->bool {
                w.write(first, fs.super.blocksize());
                return true;
            });
            w.setpos(i.datasize());
            w.truncate(i.datasize());
        }
    }
};
struct hexdumpfile : action {
    std::string ext2path;

    hexdumpfile(const std::string& ext2path)
        : ext2path(ext2path)
    {
    }

    void perform(Ext2FileSystem &fs) override
    {
        uint32_t ino= searchpath(fs, ROOTDIRINODE, ext2path);
        if (ino==0) {
            printf("hexdumpfile: path not found\n");
            return;
        }
        hexdumpinode byino(ino);
        byino.perform(fs);
    }
};
struct exportfile : action {
    std::string ext2path;
    std::string savepath;

    exportfile(const std::string& ext2path, const std::string& savepath)
        : ext2path(ext2path), savepath(savepath)
    {
    }

    void perform(Ext2FileSystem &fs) override
    {
        uint32_t ino= searchpath(fs, ROOTDIRINODE, ext2path);
        if (ino==0) {
            printf("exportfile: path not found\n");
            return;
        }
        exportinode byino(ino, savepath);
        byino.perform(fs);
    }
};
struct exportdirectory : action {
    std::string ext2path;
    std::string savepath;

    exportdirectory(const std::string& ext2path, const std::string& savepath)
        : ext2path(ext2path), savepath(savepath)
    {
    }

    void perform(Ext2FileSystem &fs) override
    {
        uint32_t ino= searchpath(fs, ROOTDIRINODE, ext2path);
        if (ino==0) {
            printf("exportdir: path not found\n");
            return;
        }
        recursedirs(fs, ino, ".", [&](const DirectoryEntry& e, const std::string& path) {
            if (e.filetype==EXT4_FT_DIR) {
                mkdir((savepath+"/"+path+"/"+e.name).c_str(), 0777);
            }
            else if (e.filetype==EXT4_FT_REG_FILE) {
                exportinode byino(e.inode, savepath+"/"+path+"/"+e.name);
                byino.perform(fs);
            }
        });
    }
};

struct listfiles : action {
    void perform(Ext2FileSystem &fs) override
    {
        //TreeReconstructor tree;
        //tree.scanfs(fs);
        recursedirs(fs, ROOTDIRINODE, "", [](const DirectoryEntry& e, const std::string& path) {
            printf("%9d %s%c %s/%s\n", e.inode,  e.filetype>=8 ? "**":"", "0-dcbpsl"[e.filetype&7], path.c_str(), e.name.c_str());
        });
    }
};
struct verboselistfiles : action {
    void perform(Ext2FileSystem &fs) override
    {
        //TreeReconstructor tree;
        //tree.scanfs(fs);
        recursedirs(fs, ROOTDIRINODE, "", [&fs](const DirectoryEntry& e, const std::string& path) {
            const Inode &i= fs.getinode(e.inode);
            printf("%9d %s %5d %5d %10d %s [%s%c] %s/%s\n", 
                    e.inode,  
                    i.modestr().c_str(), i.i_uid, i.i_gid, i.i_size, timestr(i.i_mtime).c_str(),
                    e.filetype>=8 ? "**":"", "0-dcbpsl"[e.filetype&7], path.c_str(), e.name.c_str());
        });
    }
};
struct dumpfs : action {
    void perform(Ext2FileSystem &fs) override
    {
        fs.dump();
    }
};
struct dumpblocks : action {
    uint32_t first;
    uint32_t last;
    dumpblocks(uint32_t first, uint32_t last)
        : first(first), last(last)
    {
    }
    void perform(Ext2FileSystem &fs) override
    {
        for (uint32_t nr= first ; nr<last ; nr++)
        {
            printf("block %x\n", nr);
            printf("%s\n", hexdump(0, fs.super.getptrforblock(nr), fs.super.blocksize()).c_str());
        }
    }
};
action::ptr action::parse(const std::string& arg)
{
    if (arg.empty())
        throw "empty arg";
    if (arg[0]=='#') {
// #<ino>:outfile
        size_t ix= arg.find(":", 1);

        if (ix==arg.npos) {
            return std::make_shared<hexdumpinode>(strtoul(&arg[1],0,0));
        }
        else {
            return std::make_shared<exportinode>(strtoul(&arg[1],0,0), arg.substr(ix+1));
        }
    }
    else {
        size_t ix= arg.find(":", 0);
        if (ix==arg.npos)
            return std::make_shared<hexdumpfile>(arg);

// /path:outfile
// /path/:outdir
        if (ix>0 && arg[ix-1]=='/')
            return std::make_shared<exportdirectory>(arg.substr(0,ix-1), arg.substr(ix+1));
        else
            return std::make_shared<exportfile>(arg.substr(0,ix), arg.substr(ix+1));
    }
}
bool issparse(const uint8_t *first, const uint8_t *last)
{
    return last-first>32 && get32le(first)==0xed26ff3a;
}

void expandsparse(const uint8_t *first, const uint8_t *last, ByteVector& data)
{
    const uint8_t *p= first;

    printf("sparse filesize= %08lx\n", last-p);

    uint32_t magic= get32le(p);  p+=4;
    if (magic!=0xed26ff3a)
        throw "invalid magic";
    uint32_t version= get32le(p);  p+=4;
    uint16_t hdrsize= get16le(p);  p+=2;
    uint16_t cnkhdrsize= get16le(p);  p+=2;
    uint32_t blksize= get32le(p);  p+=4;
    uint32_t blkcount= get32le(p);  p+=4;
    uint32_t chunkcount= get32le(p);  p+=4;
    uint32_t checksum= get32le(p);  p+=4;
    printf("v%08x h:%04x c:%04x b:%08x nblk:%08x ncnk:%08x,  cksum:%08x\n",
            version, hdrsize, cnkhdrsize, blksize, blkcount, chunkcount, checksum);

    if (p-first!=hdrsize) {
        printf("unexpected header size: %ld, expected %d\n", p-first, hdrsize);
    }

    data.resize(blkcount*blksize);

    uint8_t *q= &data[0];

    while (p+cnkhdrsize<=last) {
        const uint8_t *pchunk= p;

        uint16_t chunktype= get16le(p);  p+=2;
        /*uint16_t unused= get16le(p);*/  p+=2;
        uint32_t nblocks= get32le(p);  p+=4;
        uint32_t chunksize= get32le(p);  p+=4;
        if (p-pchunk!=cnkhdrsize) {
            printf("unexpected chunkhdr size: %ld, expected %d\n", p-pchunk, cnkhdrsize);
        }
        //printf("%08lx:  %04x %08x %08x\n", p-first-cnkhdrsize, chunktype, nblocks, chunksize);

        if (p+chunksize-cnkhdrsize>last) {
            printf("size too large\n");
            break;
        }

        switch(chunktype)
        {
            case 0xcac1:
                if (chunksize-cnkhdrsize!=blksize*nblocks) {
                    printf("unexpected size for raw copy: %08x iso %08x\n",
                            chunksize-cnkhdrsize, blksize*nblocks);
                }
                std::copy_n((const uint32_t*)p, (chunksize-cnkhdrsize)/4, (uint32_t*)q);
                p+=chunksize-cnkhdrsize;
                q+=chunksize-cnkhdrsize;
                break;
            case 0xcac2:
                {
                if (chunksize-cnkhdrsize!=4) {
                    printf("unexpected size for fill: %d iso 4\n", chunksize-cnkhdrsize);
                }
                uint32_t fill= get32le(p); p+=4;
                std::fill_n((uint32_t*)q, (chunksize-cnkhdrsize)/4, fill);
                q += chunksize-cnkhdrsize;
                }
                break;
            case 0xcac3:
                if (chunksize!=cnkhdrsize) {
                    printf("unexpected size for skip: %d\n", chunksize-cnkhdrsize);
                }
                q += nblocks*blksize;
                break;
            case 0xcac4:
                {
                if (chunksize-cnkhdrsize!=4) {
                    printf("unexpected size for crc: %d iso 4\n", chunksize-cnkhdrsize);
                }
                uint32_t crc= get32le(p); p+=4;
                printf("crc=%08x\n", crc);
                }
                break;
            default:
                printf("unhandled chunk %04x\n", chunktype);
        }

        p= pchunk+chunksize;
    }
}
void usage()
{
    printf("Usage: ext2dump [-l] [-d] <fsname> [exports...]\n");
    printf("     -l       lists all files\n");
    printf("     -d       verbosely lists all inodes\n");
    printf("     -o OFS1/OFS2  specify offset to efs/sparse image\n");
    printf("     -b from[-until]   hexdump blocks\n");
    printf("   #123       hexdump inode 123\n");
    printf("   #123:path  save inode 123 to path\n");
    printf("   ext2path   hexdump ext2fs path\n");
    printf("   ext2path:path   save ext2fs path to path\n");
    printf("   ext2path/:path  recursively save ext2fs dir to path\n");
}
int main(int argc,char**argv)
{
    std::list<uint64_t> offsets;

    std::string fsfile;
    std::vector<action::ptr> actions;

    for (int i=1 ; i<argc ; i++)
    {
        if (argv[i][0]=='-') switch(argv[i][1])
        {
            case 'l': actions.push_back(std::make_shared<listfiles>()); break;
            case 'v': actions.push_back(std::make_shared<verboselistfiles>()); break;
            case 'd': actions.push_back(std::make_shared<dumpfs>()); break;
            case 'o': offsets.push_back(getintarg(argv, i, argc)); break;
            case 'b': 
                      {
                      std::string arg= getstrarg(argv,i,argc);
                      char *q;
                      uint32_t first= strtol(arg.c_str(), &q, 0);
                      if (*q!=0 && *q!='-') {
                          printf("invalid -b BLOCK[-BLOCK] spec\n");
                          return 1;
                      }
                      uint32_t last= *q=='-' ? strtol(q+1, &q, 0) : first+1;
                      actions.push_back(std::make_shared<dumpblocks>(first, last));
                      }
                      break;
            default:
                  usage();
                  return 1;
        }
        else if (fsfile.empty())
            fsfile= argv[i];
        else {
            actions.push_back(action::parse(argv[i]));
        }
    }
    if (fsfile.empty())
    {
        usage();
        return 1;
    }
    try {
#ifdef USE_CPPUTILS
    mappedfile mm(fsfile);
    MemoryReader r(mm.begin(), mm.size());
#else
    MmapReader r(fsfile, MmapReader::readonly);
#endif

    Ext2FileSystem fs;
    ByteVector expanded;
    uint64_t offset= 0;
    if (!offsets.empty()) { offset= offsets.front(); offsets.pop_front(); }

    if (issparse(r.begin()+offset, r.end())) {
        expandsparse(r.begin()+offset, r.end(), expanded);

        if (!offsets.empty()) { offset= offsets.front(); offsets.pop_front(); }
        fs.parse(&expanded[offset]);
    }
    else {
        fs.parse(r.begin()+offset);
    }

    for (auto a : actions)
        a->perform(fs);

    }
    catch(const char*msg) {
        printf("EXCEPTION: %s\n", msg);
        return 1;
    }
    catch(const std::string& msg) {
        printf("EXCEPTION: %s\n", msg.c_str());
        return 1;
    }
    catch(std::exception& e) {
        printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
    catch(...) {
        printf("UNKNOWN EXCEPTION\n");
        return 1;
    }
    return 0;
}
