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

#ifdef USE_CPPUTILS
#include "mmfile.h"
#include "util/rw/MemoryReader.h"
#else
#include "util/rw/MmapReader.h"
#endif
#include "util/rw/FileReader.h"

#include "util/rw/BlockDevice.h"
#include "util/rw/OffsetReader.h"
#include "args.h"
#include <memory>
#include <system_error>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>

void lutimes(const char*path, const struct timeval *times)
{
}
int symlink(const char *src, const char *dst)
{
    return 0;
}
int mkdir(const char *src, int mode)
{
    return _mkdir(src);
}
#endif




//  ~/gitprj/repos/linux/fs/ext2/ext2.h
//  todo: add 64bit support from ext4
// todo: create ExtentsFileReader and BlocksFileReader

#define ROOTDIRINODE  2

// see linux/fs/ext4/ext4.h
enum {EXT4_FT_UNKNOWN, EXT4_FT_REG_FILE, EXT4_FT_DIR, EXT4_FT_CHRDEV, EXT4_FT_BLKDEV, EXT4_FT_FIFO, EXT4_FT_SOCK, EXT4_FT_SYMLINK };
#define EXT4_FEATURE_COMPAT_DIR_PREALLOC	0x0001
#define EXT4_FEATURE_COMPAT_IMAGIC_INODES	0x0002
#define EXT4_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#define EXT4_FEATURE_COMPAT_EXT_ATTR		0x0008
#define EXT4_FEATURE_COMPAT_RESIZE_INODE	0x0010
#define EXT4_FEATURE_COMPAT_DIR_INDEX		0x0020

#define EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#define EXT4_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
#define EXT4_FEATURE_RO_COMPAT_BTREE_DIR	0x0004
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE        0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM		0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK	0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE	0x0040
#define EXT4_FEATURE_RO_COMPAT_QUOTA		0x0100
#define EXT4_FEATURE_RO_COMPAT_BIGALLOC		0x0200
/*
 * METADATA_CSUM also enables group descriptor checksums (GDT_CSUM).  When
 * METADATA_CSUM is set, group descriptor checksums use the same algorithm as
 * all other data structures' checksums.  However, the METADATA_CSUM and
 * GDT_CSUM bits are mutually exclusive.
 */
#define EXT4_FEATURE_RO_COMPAT_METADATA_CSUM	0x0400

#define EXT4_FEATURE_INCOMPAT_COMPRESSION	0x0001
#define EXT4_FEATURE_INCOMPAT_FILETYPE		0x0002
#define EXT4_FEATURE_INCOMPAT_RECOVER		0x0004 /* Needs recovery */
#define EXT4_FEATURE_INCOMPAT_JOURNAL_DEV	0x0008 /* Journal device */
#define EXT4_FEATURE_INCOMPAT_META_BG		0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS		0x0040 /* extents support */
#define EXT4_FEATURE_INCOMPAT_64BIT		0x0080
#define EXT4_FEATURE_INCOMPAT_MMP               0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG		0x0200
#define EXT4_FEATURE_INCOMPAT_EA_INODE		0x0400 /* EA in inode */
#define EXT4_FEATURE_INCOMPAT_DIRDATA		0x1000 /* data in dirent */
#define EXT4_FEATURE_INCOMPAT_BG_USE_META_CSUM	0x2000 /* use crc32c for bg */
#define EXT4_FEATURE_INCOMPAT_LARGEDIR		0x4000 /* >2GB or 3-lvl htree */
#define EXT4_FEATURE_INCOMPAT_INLINE_DATA	0x8000 /* data in inode */

std::string timestr(uint32_t t32)
{
    time_t t = t32;
    struct tm tm = *std::gmtime(&t);
    char str[40];
    std::strftime(str, 40, "%Y-%m-%d %H:%M:%S", &tm);

    return str;
}


struct DirectoryEntry {
    uint32_t inode;
    uint8_t  filetype;  // 1=file, 2=dir, 3=chardev, 4=blockdev, 5=fifo, 6=sock, 7=symlink
    std::string name;
    size_t parse(const uint8_t *first)
    {
        const uint8_t *p= first;
        inode= get32le(p);           p+=4;
        uint32_t rec_len= get16le(p);p+=2;
        uint32_t name_len= get8(p);  p+=1;
        filetype= get8(p);           p+=1;
        name= std::string((const char*)p, name_len);
        return rec_len;
    }
    void dump() const
    {
        printf("%8d %c '%s'\n", inode, "0-dcbpsl"[filetype&7], name.c_str());
    }
};

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

    ReadWriter_ptr r;

    void parse(ReadWriter_ptr rd)
    {
        r= rd;
        uint8_t buf[0xCC];
        r->read(buf, 0xCC);
        //printf("%s\n", hexdump(r->getpos()-0xCC, buf, 0xCC).c_str());
        parse(buf, 0xCC);
    }
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
        printf("s_inodes_count=0x%08x\n", s_inodes_count);
        printf("s_blocks_count=0x%08x\n", s_blocks_count);
        printf("s_r_blocks_count=0x%08x\n", s_r_blocks_count);
        printf("s_free_blocks_count=0x%08x\n", s_free_blocks_count);
        printf("s_free_inodes_count=0x%08x\n", s_free_inodes_count);
        printf("s_first_data_block=0x%08x\n", s_first_data_block);
        printf("s_log_block_size=%d\n", s_log_block_size);
        printf("s_log_frag_size=%d\n", s_log_frag_size);
        printf("s_blocks_per_group=0x%08x\n", s_blocks_per_group);
        printf("s_frags_per_group=0x%08x\n", s_frags_per_group);
        printf("s_inodes_per_group=0x%08x\n", s_inodes_per_group);
        printf("s_mtime=0x%08x : %s\n", s_mtime, timestr(s_mtime).c_str());
        printf("s_wtime=0x%08x : %s\n", s_wtime, timestr(s_wtime).c_str());
        printf("s_mnt_count=0x%04x\n", s_mnt_count);
        printf("s_max_mnt_count=0x%04x\n", s_max_mnt_count);
        printf("s_magic=0x%04x\n", s_magic);
        printf("s_state=0x%04x\n", s_state);
        printf("s_errors=0x%04x\n", s_errors);
        printf("s_minor_rev_level=0x%04x\n", s_minor_rev_level);
        printf("s_lastcheck=0x%08x : %s\n", s_lastcheck, timestr(s_lastcheck).c_str());
        printf("s_checkinterval=0x%08x\n", s_checkinterval);
        printf("s_creator_os=0x%08x\n", s_creator_os);
        printf("s_rev_level=0x%08x\n", s_rev_level);
        printf("s_def_resuid=0x%04x\n", s_def_resuid);
        printf("s_def_resgid=0x%04x\n", s_def_resgid);

        printf("i->%d, b->%d groups\n", s_inodes_count/s_inodes_per_group, (s_blocks_count+s_blocks_per_group-1)/s_blocks_per_group);
        printf("s_first_ino=%d\n", s_first_ino);
        printf("s_inode_size=%d\n", s_inode_size);
        printf("s_block_group_nr=%d\n", s_block_group_nr);
        printf("s_feature_compat=0x%08x: %s\n", s_feature_compat, feat_compat2str(s_feature_compat).c_str());
        printf("s_feature_incompat=0x%08x: %s\n", s_feature_incompat, feat_incompat2str(s_feature_incompat).c_str());
        printf("s_feature_ro_compat=0x%08x: %s\n", s_feature_ro_compat, feat_rocompat2str(s_feature_ro_compat).c_str());
        printf("s_uuid=%s\n", hexdump(s_uuid, 16).c_str());
        printf("s_volume_name=%s\n", ascdump(s_volume_name, 16).c_str());
        printf("s_last_mounted=%s\n", ascdump(s_last_mounted, 16).c_str());
        printf("s_algo_bitmap=0x%08x\n", s_algo_bitmap);
    }
    static std::string feat_compat2str(uint32_t feat)
    {
        StringList l;

        uint32_t all = EXT4_FEATURE_COMPAT_DIR_PREALLOC|EXT4_FEATURE_COMPAT_IMAGIC_INODES|EXT4_FEATURE_COMPAT_HAS_JOURNAL|EXT4_FEATURE_COMPAT_EXT_ATTR|EXT4_FEATURE_COMPAT_RESIZE_INODE|EXT4_FEATURE_COMPAT_DIR_INDEX;
        if (feat&EXT4_FEATURE_COMPAT_DIR_PREALLOC) l.push_back("DIR_PREALLOC");
        if (feat&EXT4_FEATURE_COMPAT_IMAGIC_INODES) l.push_back("IMAGIC_INODES");
        if (feat&EXT4_FEATURE_COMPAT_HAS_JOURNAL) l.push_back("HAS_JOURNAL");
        if (feat&EXT4_FEATURE_COMPAT_EXT_ATTR) l.push_back("EXT_ATTR");
        if (feat&EXT4_FEATURE_COMPAT_RESIZE_INODE) l.push_back("RESIZE_INODE");
        if (feat&EXT4_FEATURE_COMPAT_DIR_INDEX) l.push_back("DIR_INDEX");
        if (feat&~all) l.push_back(stringformat("unk_%x", feat&~all));
        return JoinStringList(l, ",");
    }
    static std::string feat_incompat2str(uint32_t feat)
    {
        StringList l;

        uint32_t all = EXT4_FEATURE_INCOMPAT_COMPRESSION|EXT4_FEATURE_INCOMPAT_FILETYPE|EXT4_FEATURE_INCOMPAT_RECOVER|EXT4_FEATURE_INCOMPAT_JOURNAL_DEV|EXT4_FEATURE_INCOMPAT_META_BG|EXT4_FEATURE_INCOMPAT_EXTENTS|EXT4_FEATURE_INCOMPAT_64BIT|EXT4_FEATURE_INCOMPAT_MMP|EXT4_FEATURE_INCOMPAT_FLEX_BG|EXT4_FEATURE_INCOMPAT_EA_INODE|EXT4_FEATURE_INCOMPAT_DIRDATA|EXT4_FEATURE_INCOMPAT_BG_USE_META_CSUM|EXT4_FEATURE_INCOMPAT_LARGEDIR|EXT4_FEATURE_INCOMPAT_INLINE_DATA;
        if (feat&EXT4_FEATURE_INCOMPAT_COMPRESSION) l.push_back("COMPRESSION");
        if (feat&EXT4_FEATURE_INCOMPAT_FILETYPE) l.push_back("FILETYPE");
        if (feat&EXT4_FEATURE_INCOMPAT_RECOVER) l.push_back("RECOVER");
        if (feat&EXT4_FEATURE_INCOMPAT_JOURNAL_DEV) l.push_back("JOURNAL_DEV");
        if (feat&EXT4_FEATURE_INCOMPAT_META_BG) l.push_back("META_BG");
        if (feat&EXT4_FEATURE_INCOMPAT_EXTENTS) l.push_back("EXTENTS");
        if (feat&EXT4_FEATURE_INCOMPAT_64BIT) l.push_back("64BIT");
        if (feat&EXT4_FEATURE_INCOMPAT_MMP) l.push_back("MMP");
        if (feat&EXT4_FEATURE_INCOMPAT_FLEX_BG) l.push_back("FLEX_BG");
        if (feat&EXT4_FEATURE_INCOMPAT_EA_INODE) l.push_back("EA_INODE");
        if (feat&EXT4_FEATURE_INCOMPAT_DIRDATA) l.push_back("DIRDATA");
        if (feat&EXT4_FEATURE_INCOMPAT_BG_USE_META_CSUM) l.push_back("BG_USE_META_CSUM");
        if (feat&EXT4_FEATURE_INCOMPAT_LARGEDIR) l.push_back("LARGEDIR");
        if (feat&EXT4_FEATURE_INCOMPAT_INLINE_DATA) l.push_back("INLINE_DATA");
        if (feat&~all) l.push_back(stringformat("unk_%x", feat&~all));
        return JoinStringList(l, ",");
    }
    static std::string feat_rocompat2str(uint32_t feat)
    {
        StringList l;

        uint32_t all = EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER|EXT4_FEATURE_RO_COMPAT_LARGE_FILE|EXT4_FEATURE_RO_COMPAT_BTREE_DIR|EXT4_FEATURE_RO_COMPAT_HUGE_FILE|EXT4_FEATURE_RO_COMPAT_GDT_CSUM|EXT4_FEATURE_RO_COMPAT_DIR_NLINK|EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE|EXT4_FEATURE_RO_COMPAT_QUOTA|EXT4_FEATURE_RO_COMPAT_BIGALLOC|EXT4_FEATURE_RO_COMPAT_METADATA_CSUM;
        if (feat&EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER) l.push_back("SPARSE_SUPER");
        if (feat&EXT4_FEATURE_RO_COMPAT_LARGE_FILE) l.push_back("LARGE_FILE");
        if (feat&EXT4_FEATURE_RO_COMPAT_BTREE_DIR) l.push_back("BTREE_DIR");
        if (feat&EXT4_FEATURE_RO_COMPAT_HUGE_FILE) l.push_back("HUGE_FILE");
        if (feat&EXT4_FEATURE_RO_COMPAT_GDT_CSUM) l.push_back("GDT_CSUM");
        if (feat&EXT4_FEATURE_RO_COMPAT_DIR_NLINK) l.push_back("DIR_NLINK");
        if (feat&EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE) l.push_back("EXTRA_ISIZE");
        if (feat&EXT4_FEATURE_RO_COMPAT_QUOTA) l.push_back("QUOTA");
        if (feat&EXT4_FEATURE_RO_COMPAT_BIGALLOC) l.push_back("BIGALLOC");
        if (feat&EXT4_FEATURE_RO_COMPAT_METADATA_CSUM) l.push_back("METADATA_CSUM");
        if (feat&~all) l.push_back(stringformat("unk_%x", feat&~all));
        return JoinStringList(l, ",");
    }

    ByteVector getblock(unsigned n) const
    {
        if (n>=s_blocks_count)
            throw "blocknr too large";

        ByteVector buf(blocksize());
        r->setpos(blocksize()*n);
        r->read(&buf[0], blocksize());

        return buf;
    }
};
typedef std::function<bool(const uint8_t*)> BLOCKCALLBACK;
struct ExtentNode {

    virtual ~ExtentNode() { }
    void parse(ReadWriter_ptr r)
    {
        uint8_t buf[12];
        r->read(buf, 12);
        parse(buf);
    }
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
        printf("blk:%08x, l=%d, %010" PRIx64 "\n", ee_block, ee_len, startblock());
    }
    uint64_t startblock() const
    {
        return (uint64_t(ee_start_hi)<<32)|ee_start_lo;
    }
    virtual bool enumblocks(const SuperBlock &super, BLOCKCALLBACK cb) const
    {
        uint64_t blk= startblock();
        for (unsigned i=0 ; i<ee_len ; i++)
            if (!cb(&super.getblock(blk++).front()))
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
        printf("blk:%08x, [%d] %010" PRIx64 "\n", ei_block, ei_unused, leafnode());
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

        if (eh.eh_magic != 0xf30a) {
            printf("ehmagic=%04x - %s\n", eh.eh_magic, hexdump(p-12, 12).c_str());
            //throw "invalid extent hdr magic";
            return;
        }

        for (unsigned i=0 ; i<eh.eh_entries ; i++) {
            if (eh.eh_depth==0)
                extents.push_back(std::make_shared<ExtentLeaf>(p));
            else
                extents.push_back(std::make_shared<ExtentInternal>(p));
            p+=12;
        }
    }
    virtual bool enumblocks(const SuperBlock &super, BLOCKCALLBACK cb) const
    {
        for (unsigned i=0 ; i<eh.eh_entries ; i++)
            if (!extents[i]->enumblocks(super, cb))
                return false;
        return true;
    }

    void dump() const
    {
        eh.dump();
        for (unsigned i=0 ; i<extents.size() ; i++) {
            printf("EXT %d: ", i);
            extents[i]->dump();
        }
    }

};
bool ExtentInternal::enumblocks(const SuperBlock &super, BLOCKCALLBACK cb) const
{
    Extent e;
    e.parse(&super.getblock(leafnode()).front());

    return e.enumblocks(super, cb);
}

// see linux/include/uapi/linux/stat.h
enum { EXT4_S_IFIFO=0x1000, EXT4_S_IFCHR=0x2000, EXT4_S_IFDIR=0x4000, EXT4_S_IFBLK=0x6000, EXT4_S_IFREG=0x8000, EXT4_S_IFLNK=0xa000, EXT4_S_IFSOCK=0xc000 };
// see linux/fs/ext4/ext4.h


#define	EXT4_SECRM_FL			0x00000001 /* Secure deletion */
#define	EXT4_UNRM_FL			0x00000002 /* Undelete */
#define	EXT4_COMPR_FL			0x00000004 /* Compress file */
#define EXT4_SYNC_FL			0x00000008 /* Synchronous updates */
#define EXT4_IMMUTABLE_FL		0x00000010 /* Immutable file */
#define EXT4_APPEND_FL			0x00000020 /* writes to file may only append */
#define EXT4_NODUMP_FL			0x00000040 /* do not dump file */
#define EXT4_NOATIME_FL			0x00000080 /* do not update atime */
/* Reserved for compression usage... */
#define EXT4_DIRTY_FL			0x00000100
#define EXT4_COMPRBLK_FL		0x00000200 /* One or more compressed clusters */
#define EXT4_NOCOMPR_FL			0x00000400 /* Don't compress */
#define EXT4_ECOMPR_FL			0x00000800 /* Compression error */
/* End compression flags --- maybe not all used */
#define EXT4_INDEX_FL			0x00001000 /* hash-indexed directory */
#define EXT4_IMAGIC_FL			0x00002000 /* AFS directory */
#define EXT4_JOURNAL_DATA_FL	0x00004000 /* file data should be journaled */
#define EXT4_NOTAIL_FL			0x00008000 /* file tail should not be merged */
#define EXT4_DIRSYNC_FL			0x00010000 /* dirsync behaviour (directories only) */
#define EXT4_TOPDIR_FL			0x00020000 /* Top of directory hierarchies*/
#define EXT4_HUGE_FILE_FL       0x00040000 /* Set to each huge file */
#define EXT4_EXTENTS_FL			0x00080000 /* Inode uses extents */
#define EXT4_EA_INODE_FL	    0x00200000 /* Inode used for large EA */
#define EXT4_EOFBLOCKS_FL		0x00400000 /* Blocks allocated beyond EOF */
#define EXT4_INLINE_DATA_FL		0x10000000 /* Inode has inline data. */
#define EXT4_RESERVED_FL		0x80000000 /* reserved for ext4 lib */
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
        printf("m:%06o %4d o[%5d %5d] t[%10d %10d %10d %10d]  %12" PRIu64 " [b:%8d] F:%05x(%s) X:%08x %s\n",
                i_mode, i_links_count, i_gid, i_uid, i_atime, i_ctime, i_mtime, i_dtime, datasize(), i_blocks, i_flags, fl2str(i_flags).c_str(), i_file_acl, hexdump(i_osd2, 12).c_str());
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
    static std::string fl2str(uint32_t fl)
    {
        StringList l;

        uint32_t all = EXT4_SECRM_FL|EXT4_UNRM_FL|EXT4_COMPR_FL|EXT4_SYNC_FL|EXT4_IMMUTABLE_FL|EXT4_APPEND_FL|EXT4_NODUMP_FL|EXT4_NOATIME_FL|EXT4_DIRTY_FL|EXT4_COMPRBLK_FL|EXT4_NOCOMPR_FL|EXT4_ECOMPR_FL|EXT4_INDEX_FL|EXT4_IMAGIC_FL|EXT4_JOURNAL_DATA_FL|EXT4_NOTAIL_FL|EXT4_DIRSYNC_FL|EXT4_TOPDIR_FL|EXT4_HUGE_FILE_FL|EXT4_EXTENTS_FL|EXT4_EA_INODE_FL|EXT4_EOFBLOCKS_FL|EXT4_INLINE_DATA_FL|EXT4_RESERVED_FL;


        if (fl&EXT4_SECRM_FL) l.push_back("SECRM");
        if (fl&EXT4_UNRM_FL) l.push_back("UNRM");
        if (fl&EXT4_COMPR_FL) l.push_back("COMPR");
        if (fl&EXT4_SYNC_FL) l.push_back("SYNC");
        if (fl&EXT4_IMMUTABLE_FL) l.push_back("IMMUTABLE");
        if (fl&EXT4_APPEND_FL) l.push_back("APPEND");
        if (fl&EXT4_NODUMP_FL) l.push_back("NODUMP");
        if (fl&EXT4_NOATIME_FL) l.push_back("NOATIME");
        if (fl&EXT4_DIRTY_FL) l.push_back("DIRTY");
        if (fl&EXT4_COMPRBLK_FL) l.push_back("COMPRBLK");
        if (fl&EXT4_NOCOMPR_FL) l.push_back("NOCOMPR");
        if (fl&EXT4_ECOMPR_FL) l.push_back("ECOMPR");
        if (fl&EXT4_INDEX_FL) l.push_back("INDEX");
        if (fl&EXT4_IMAGIC_FL) l.push_back("IMAGIC");
        if (fl&EXT4_JOURNAL_DATA_FL) l.push_back("JOURNAL_DATA");
        if (fl&EXT4_NOTAIL_FL) l.push_back("NOTAIL");
        if (fl&EXT4_DIRSYNC_FL) l.push_back("DIRSYNC");
        if (fl&EXT4_TOPDIR_FL) l.push_back("TOPDIR");
        if (fl&EXT4_HUGE_FILE_FL) l.push_back("HUGE_FILE");
        if (fl&EXT4_EXTENTS_FL) l.push_back("EXTENTS");
        if (fl&EXT4_EA_INODE_FL) l.push_back("EA_INODE");
        if (fl&EXT4_EOFBLOCKS_FL) l.push_back("EOFBLOCKS");
        if (fl&EXT4_INLINE_DATA_FL) l.push_back("INLINE_DATA");
        if (fl&EXT4_RESERVED_FL) l.push_back("RESERVED");
        if (fl&~all) l.push_back(stringformat("unk_%x", fl&~all));
        return JoinStringList(l, ",");
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
                if (!cb(&super.getblock(i_block[i]).front()))
                    return false;
                bytes += super.blocksize();
            }
        }
        if (i_block[12])
            if (!enumi1block(super, bytes, &super.getblock(i_block[12]).front(), cb))
                return false;
        if (i_block[13])
            if (!enumi2block(super, bytes, &super.getblock(i_block[13]).front(), cb))
                return false;
        if (i_block[14])
            if (!enumi3block(super, bytes, &super.getblock(i_block[14]).front(), cb))
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
                if (!cb(&super.getblock(blocknr).front()))
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
            if (!enumi1block(super, bytes, &super.getblock(*p++).front(), cb))
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
            if (!enumi2block(super, bytes, &super.getblock(*p++).front(), cb))
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

struct BlockGroup {
    uint64_t itableoffset;
    size_t ninodes;
    size_t inodesize;

    ReadWriter_ptr r;

    void parse(uint64_t first, const SuperBlock &super, BlockGroupDescriptor& bgdesc)
    {

        r= super.r;

        itableoffset= first+(bgdesc.bg_inode_table%super.s_blocks_per_group)*super.blocksize();
        ninodes= super.s_inodes_per_group;
        inodesize= super.s_inode_size;
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
        for (unsigned i=0 ; i<ninodes ; i++)
        {
            Inode ino= getinode(i);
            if (!ino._empty)
                cb(inodebase+i, ino);
        }
    }
    static void dumpdirblock(const uint8_t *first, size_t blocksize)
    {
        //printf("%s\n", hexdump(first, blocksize).c_str());
        const uint8_t *p= first;
        while (p<first+blocksize) {
            DirectoryEntry ent;
            size_t n= ent.parse(p);
            if (n==0)
                break;
            ent.dump();
            p+=n;
        }
    }

    Inode getinode(uint32_t nr) const
    {
        uint8_t buf[128];
        r->setpos(itableoffset+inodesize*nr);
        r->read(buf, 128);
        Inode ino;
        ino.parse(buf, 128);
        return ino;
    }

};

struct Ext2FileSystem {
    SuperBlock super;
    std::vector<BlockGroupDescriptor> bgdescs;

    std::vector<BlockGroup> groups;
    uint64_t sb_offset;
    uint64_t rootdir_in;

    Ext2FileSystem() { }

    void parse(ReadWriter_ptr r)
    {
        r->setpos(sb_offset);
        super.parse(r);

        if (super.s_magic != 0xef53)
            throw "not an ext2 fs";

        // groupdescs follow the superblock in the 2nd block
        parsegroupdescs(r);

        groups.resize(super.ngroups());
        uint64_t off= 0;
        for (unsigned i=0 ; i<super.ngroups() ; i++) {
            groups[i].parse(off, super, bgdescs[i]);
            off+=super.bytespergroup();
        }
    }
    void parsegroupdescs(ReadWriter_ptr r)
    {
        uint64_t bgdescpos;

        // When the blocksize == 1024, the superblock fills block 1
        // and the block group descriptors start with block 2.
        // For larger sizes the superblock fits inside block 0
        // and the block group descriptors start with block 1.
        if (super.blocksize() == 1024)
            bgdescpos = 2048;
        else
            bgdescpos = super.blocksize();

        r->setpos(bgdescpos);

        bgdescs.resize(super.ngroups());

        ByteVector buf(super.ngroups()*32);
        r->read(&buf[0], buf.size());

        for (unsigned i=0 ; i<super.ngroups() ; i++)
            bgdescs[i].parse(&buf[i*32]);
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
    Inode getinode(uint32_t nr) const
    {
        nr--;
        if (nr>=super.s_inodes_count)
            nr= 0;
        return groups[nr/super.s_inodes_per_group].getinode(nr%super.s_inodes_per_group);
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
        if (i._empty) {
          return;
        }
#ifndef _WIN32
        if(i.issymlink()) {
            if(symlink(i.symlink.c_str(), savepath.c_str()) != 0) {
                std::string err_msg = "Failed to create symlink: " + savepath + " -> " + i.symlink;
                perror(err_msg.c_str());
            }
            return;
        }
#endif
        if (true) {
            FileReader w(savepath, FileReader::opencreate);

            i.enumblocks(fs.super, [&](const uint8_t *first)->bool {
                w.write(first, fs.super.blocksize());
                return true;
            });
            w.setpos(i.datasize());
            w.truncate(i.datasize());

            // end of block closes the file
        }

        // restore mode bits
        chmod(savepath.c_str(), i.i_mode);

        // restore last-access and modification timestamps.
        struct timeval tv[2];
        tv[0].tv_sec = i.i_atime; tv[0].tv_usec = 0;
        tv[1].tv_sec = i.i_mtime; tv[1].tv_usec = 0;
        lutimes(savepath.c_str(), tv);
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
        uint32_t ino= searchpath(fs, fs.rootdir_in, ext2path);
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
        uint32_t ino= searchpath(fs, fs.rootdir_in, ext2path);
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
        struct stat st;
        if (-1==stat(savepath.c_str(), &st)) {
            if (-1==mkdir(savepath.c_str(), 0777))
                throw std::system_error(errno, std::generic_category(), "mkdir");
        }
        else if (!S_ISDIR(st.st_mode))
            throw std::runtime_error("savepath is not a directory");
    }

    void perform(Ext2FileSystem &fs) override
    {
        uint32_t ino= searchpath(fs, fs.rootdir_in, ext2path);
        if (ino==0) {
            printf("exportdir: path not found\n");
            return;
        }
        recursedirs(fs, ino, ".", [&](const DirectoryEntry& e, const std::string& path) {
            if (e.filetype==EXT4_FT_DIR) {
                if (-1==mkdir((savepath+"/"+path+"/"+e.name).c_str(), 0777) && errno!=EEXIST) {
                    perror("mkdir(savepath)");
                }
            }
            else if (e.filetype==EXT4_FT_REG_FILE || e.filetype==EXT4_FT_SYMLINK) {
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
        recursedirs(fs, fs.rootdir_in, "", [](const DirectoryEntry& e, const std::string& path) {
            printf("%9d %s%c %s/%s\n", e.inode,  e.filetype>=8 ? "**":"", "0-dcbpsl"[e.filetype&7], path.c_str(), e.name.c_str());
        });
    }
};
struct verboselistfiles : action {
    void perform(Ext2FileSystem &fs) override
    {
        //TreeReconstructor tree;
        //tree.scanfs(fs);
        recursedirs(fs, fs.rootdir_in, "", [&fs](const DirectoryEntry& e, const std::string& path) {
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
            printf("%s\n", hexdump(0, &fs.super.getblock(nr).front(), fs.super.blocksize()).c_str());
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



class SparseReader : public ReadWriter {
    ReadWriter_ptr r;

    uint32_t magic;
    uint32_t version;
    uint16_t hdrsize;
    uint16_t cnkhdrsize;
    uint32_t blksize;
    uint32_t blkcount;
    uint32_t chunkcount;
    uint32_t checksum;

    struct sparserecord {
        bool isfill;
        uint32_t ndwords;
        union {
            uint32_t value;
            uint64_t offset;
        };

        sparserecord(uint32_t ndwords, bool isfill, uint64_t val)
            : isfill(isfill), ndwords(ndwords)
        {
            if (isfill)
                value= val;
            else
                offset= val;
        }
        void dump(uint64_t off) const
        {
            if (isfill)
                printf("%" PRIx64 "-%" PRIx64 ": fill with %08x\n", off, off+ndwords*4, value);
            else
                printf("%" PRIx64 "-%" PRIx64 ": copy from %" PRIx64 "\n", off, off+ndwords*4, offset);
        }
    };
    std::map<uint64_t, sparserecord> _map;

    uint64_t _off;
public:
    SparseReader(ReadWriter_ptr r)
        : r(r), _off(0)
    {
        readheader();
        scansparse();

        printf(" %lu entries in sparse map\n", _map.size());
        if (_map.size()) {
            auto i= _map.begin();
            printf("first: "); i->second.dump(i->first);
        }
        if (_map.size()>1) {
            auto i= _map.end();  --i;
            printf("last:  "); i->second.dump(i->first);
        }
    }
    static bool issparse(ReadWriter_ptr r)
    {
        return r->size()>32 && r->read32le(0)==0xed26ff3a;
    }

private:
    void readheader()
    {
        r->setpos(0);

        magic= r->read32le();
        if (magic!=0xed26ff3a)
            throw "invalid sparse magic";
        version= r->read32le();
        hdrsize= r->read16le();
        cnkhdrsize= r->read16le();
        blksize= r->read32le();
        blkcount= r->read32le();
        chunkcount= r->read32le();
        checksum= r->read32le();
        printf("sparse: v%08x h:%04x c:%04x b:%08x nblk:%08x ncnk:%08x,  cksum:%08x\n",
                version, hdrsize, cnkhdrsize, blksize, blkcount, chunkcount, checksum);
        if (cnkhdrsize!=12) {
            printf("unexpected chunkhdr size: %hu\n", cnkhdrsize);
        }
    }

    void scansparse()
    {
        r->setpos(hdrsize);

        uint64_t ofs= 0;
        while (!r->eof()) {

            uint16_t chunktype= r->read16le();
            /*uint16_t unused=*/ r->read16le();
            uint32_t nblocks= r->read32le();
            uint32_t chunksize= r->read32le();

            if (cnkhdrsize!=12)
                r->setpos(r->getpos() + cnkhdrsize-12);

            switch(chunktype)
            {
                case 0xcac1: // copy
                    copydata(r->getpos(), (chunksize-cnkhdrsize)/4, ofs);
                    ofs += chunksize-cnkhdrsize;
                    break;
                case 0xcac2: // fill
                    {
                    uint32_t fill= r->read32le();
                    filldata(fill, (chunksize-cnkhdrsize)/4, ofs);

                    ofs += chunksize-cnkhdrsize;
                    }
                    break;
                case 0xcac3:
                    filldata(0, nblocks*blksize/4, ofs);
                    ofs += nblocks*blksize;
                    break;
                case 0xcac4:
                    {
                    /*uint32_t crc=*/ r->read32le();
                    }
                    break;
            }

            r->setpos(r->getpos() + chunksize-cnkhdrsize);
        }
        printf("end of sparse: %" PRIx64 "\n", ofs);
    }
    void copydata(uint64_t sparseofs, uint32_t ndwords, uint64_t expandedofs)
    {
        _map.emplace(expandedofs, sparserecord{ndwords, 0, sparseofs});
    }
    void filldata(uint32_t value, uint32_t ndwords, uint64_t expandedofs)
    {
        _map.emplace(expandedofs, sparserecord{ndwords, 1, value});
    }

    std::map<uint64_t, sparserecord>::const_iterator findofs(uint64_t ofs) const
    {
        auto i= _map.upper_bound(ofs);
        if (i==_map.begin()) {
            //printf("did not find %llx\n", ofs);
            return _map.end();
        }
        i--;
        if (i->first + i->second.ndwords*4 < ofs) {
            //printf("%llx outside of block: ", ofs); i->second.dump(i->first);
            return _map.end();
        }

        return i;
    }

    void memfill(uint8_t *p, uint32_t val, size_t n, int rot)
    {
        uint8_t b[4];  *(uint32_t*)b= val;

        while (n--) 
            *p++ = b[(rot++)&3];
    }
public:
    virtual size_t read(uint8_t *p, size_t n)
    {
        size_t total= 0;
        while (n) {
            auto i= findofs(_off);
            if (i==_map.end())
                break;
            size_t want= std::min((uint64_t)n, i->second.ndwords*4 - (_off - i->first));

            if (i->second.isfill) {
                //printf("found %llx : fill %zx bytes with %08x\n", _off, want, i->second.value);
                if (i->second.value==0 || i->second.value==0xFFFFFFFF)
                    memset(p, i->second.value, want);
                else {
                    memfill(p, i->second.value, want, (_off - i->first)&3);

                }
            }
            else {
                //printf("found %llx : %zx bytes from %llx+%llx-%llx\n", _off, want, i->second.offset, _off, i->first);
                r->setpos(i->second.offset + _off-i->first);
                r->read(p, want);
            }

            p += want;
            n -= want;
            total += want;
            _off += want;
        }
        return total;
    }
    virtual void write(const uint8_t *p, size_t n)
    {
        throw "writing not implemented";
    }
    virtual void setpos(uint64_t off)
    {
        _off= off;
    }
    virtual void truncate(uint64_t off)
    {
        throw "truncate not implemented";
    }
    virtual uint64_t size()
    {
        return blkcount*blksize;
    }
    virtual uint64_t getpos() const
    {
        return _off;
    }
    virtual bool eof()
    {
        return getpos()>=size();
    }
};
void usage()
{
    printf("Usage: ext2rd [-l] [-v] <fsname> [exports...]\n");
    printf("     -l       lists all files\n");
    printf("     -B       open as block device\n");
    printf("     -d       verbosely lists all inodes\n");
    printf("     -o OFS1/OFS2  specify offset to efs/sparse image\n");
    printf("     -b from[-until]   hexdump blocks\n");
    printf("     -S OFS   specify superblock offset\n");
    printf("     -R inode specify root inode for -l and -d\n");
    printf("   #123       hexdump inode 123\n");
    printf("   #123:path  save inode 123 to path\n");
    printf("   ext2path   hexdump ext2fs path\n");
    printf("   ext2path:path   save ext2fs path to path\n");
    printf("   ext2path/:path  recursively save ext2fs dir to path\n");
    printf("note: ext2path must not start with a slash\n");
    printf("note2: /dev/rdiskNN is much faster than /dev/diskNN\n");
}

int main(int argc,char**argv)
{
    std::list<uint64_t> offsets;

    std::string fsfile;
    std::vector<action::ptr> actions;
    bool openasblockdev= false;
    uint64_t sb_offset = 0x400;
    uint64_t rootdir_in = ROOTDIRINODE;

    try {

    for (int i=1 ; i<argc ; i++)
    {
        if (argv[i][0]=='-') switch(argv[i][1])
        {
            case 'l': actions.push_back(std::make_shared<listfiles>()); break;
            case 'v': actions.push_back(std::make_shared<verboselistfiles>()); break;
            case 'd': actions.push_back(std::make_shared<dumpfs>()); break;
            case 'o': offsets.push_back(getintarg(argv, i, argc)); break;
            case 'B': openasblockdev= true; break;
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
            case 'S':
                      {
                      std::string arg= getstrarg(argv,i,argc);
                      char *q;
                      sb_offset= strtoul(arg.c_str(), &q, 0);
                      if (*q!=0) {
                          printf("invalid -S offset spec\n");
                          return 1;
                      }
		      }
		      break;
            case 'R':
                      {
                      std::string arg= getstrarg(argv,i,argc);
                      char *q;
                      rootdir_in = strtoul(arg.c_str(), &q, 0);
                      if (*q!=0) {
                          printf("invalid inode spec\n");
                          return 1;
                      }
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


#ifdef USE_CPPUTILS
    std::shared_ptr<mappedfile> mm;
#endif
    ReadWriter_ptr r;
    if (openasblockdev)
        r= std::shared_ptr<BlockDevice>(new BlockDevice(fsfile, BlockDevice::readonly));
    else {
#ifdef USE_CPPUTILS
        mm = std::make_shared<mappedfile>(fsfile);
        r = std::make_shared<MemoryReader>(mm->begin(), mm->size());
#else
        r= std::shared_ptr<MmapReader>(new MmapReader(fsfile, MmapReader::readonly));
#endif
    }
    if (!offsets.empty()) {
        r= std::make_shared<OffsetReader>(r, offsets.front(), r->size()-offsets.front());
        offsets.pop_front();
    }

    Ext2FileSystem fs;
    ByteVector expanded;

    if (SparseReader::issparse(r)) {
        r= std::make_shared<SparseReader>(r);
        if (!offsets.empty()) {
            r= std::make_shared<OffsetReader>(r, offsets.front(), r->size()-offsets.front());
            offsets.pop_front();
        }
    }
    fs.sb_offset = sb_offset;
    fs.rootdir_in = rootdir_in;
    fs.parse(r);

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
    catch(const std::exception& e) {
        printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
    catch(...) {
        printf("EXCEPTION - did you specify -B for a block device?\n");
        return 1;
    }
    return 0;
}


