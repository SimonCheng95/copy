#define FUSE_USE_VERSION 26

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fuse.h>
#include <fcntl.h>
#include "ufs.h"
#include "op.h"
#include "syslog.h"

static int fs_getattr(const char *path, struct stat *stbuf)
{
    //syslog(LOG_INFO, "call fs_getattr path=%s", path);
    int res;
    struct inode *tmp = malloc(sizeof(struct inode));
    if(tmp == NULL) {
        res = -1;
        goto end;
    }
    if (op_path_parse(path, tmp) == -1) {
        res = -ENOENT;
        goto end;
    }
    memset(stbuf, 0, sizeof(struct stat));
    if (tmp->flag == 1) {
        stbuf->st_mode = S_IFDIR | 0666;
    } else if (tmp->flag == 0) {
        stbuf->st_mode = S_IFREG | 0666;
    }
    stbuf->st_size = tmp->file_size;
    res = 0;
end:
    free(tmp);
    tmp = NULL;
    //syslog(LOG_INFO,"fs_getattr over");
    return res;
}
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    //syslog(LOG_INFO, "call fs_readdir path=%s", path);
    struct inode *tmp = malloc(sizeof(struct inode));
    BYTE buf_tmp[BLOCK_BYTES];
    struct item *p_item;
    int res, pos, i;
    char name[MAX_FILENAME + 1];
    if (tmp == NULL) {
        res =-1;
        goto end;
    }
    if (op_isempty(path) == 1) {
        res = 0;
        goto end;
    }
    if (op_path_parse(path, tmp) == -1) {
        res = -ENOENT;
        goto end;
    }
    if (tmp->flag == 0) {
        res = -ENOTDIR;
        goto end;
    }
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    i = 0;
    while (tmp->datablock_no[i] != -1 && i < MAX_FILESIZE) {
        op_readblock(tmp->datablock_no[i], buf_tmp);
        p_item = (struct item*)buf_tmp;
        pos = 0;
		while (pos < BLOCK_ITEMS) {
			if (p_item->item_flag == 1) {
				strcpy(name, p_item->filename);
				//syslog(LOG_INFO, "readdir itemname=%s", name);
				if (filler(buf, name, NULL, 0)) {
					res = 1;
					goto end;
				}
			}
			p_item++;
			pos++;
		}
		i++;
	}
	res = 0;
end:
	free(tmp);
	tmp = NULL;
	return res;
}

static int fs_mkdir(const char *path, mode_t mode) {
    syslog(LOG_INFO, "fs_mkdir path=%s", path);
    int res;
    res = op_create(path, 1);

    //syslog(LOG_INFO, "fs_mkdir finished return=%d", res);        
    return res;
}

static int fs_mknod(const char *path, mode_t mode, dev_t dev) {
    //syslog(LOG_INFO, "call fs_mknod ");
    int res;
    res = op_create(path, 0);

    //syslog(LOG_INFO, "op_mknod return=%d", res);        
    return res;
}

static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    //syslog(LOG_INFO, "call fs_create path=%s", path);
    int res;
    res = op_create(path, 0);
    //syslog(LOG_INFO, "fs_create op_create return=%d", res);     
    return res;
}

static int fs_unlink(const char *path) {
    //syslog(LOG_INFO, "call fs_unlink");
    int res;
    res = op_rm(path, 0);
    //syslog(LOG_INFO, "op_unlink return=%d", res);
    return res;
}

static int fs_rmdir(const char *path) {
    //syslog(LOG_INFO, "call fs_rmdir");
    int res;
    res = op_rm(path, 1);
    //syslog(LOG_INFO, "fs_rmdir op_rmdir return=%d", res);
    return res;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
    struct inode *tmp = malloc(sizeof(struct inode));
    int res;
    if(tmp == NULL) {
        return -1;
    }
    res = op_path_parse(path, tmp);
    free(tmp);
    tmp = NULL;
    return res;
}

static int fs_opendir(const char *path, struct fuse_file_info *fi) {
    struct inode *tmp = malloc(sizeof(struct inode));
    int res;
    if(tmp == NULL) {
       return -1;
    }
    res = op_path_parse(path, tmp);
    free(tmp);
    tmp = NULL;
    return res;
}

static int fs_rename(const char *oldpath, const char *newpath) {
    //syslog(LOG_INFO, "oldpath=%s,newpath=%s", oldpath, newpath);
    struct inode *tmp = malloc(sizeof(struct inode));
    struct inode *inode_tmp = malloc(sizeof(struct inode));
    struct item *p_item;
    BYTE buff[BLOCK_BYTES];
    int res, i, pos;
    char *parentpath, *name, *new_parentpath, *newname, *path_tmp1, *path_tmp2;
    if (tmp == NULL || inode_tmp == NULL) {
        res = -1;
        goto end;
    }
    if(op_path_parse(oldpath, tmp) == -1) {
        res = -ENOENT;
        goto end;
    }
    path_tmp1 = strdup(newpath);
    op_div_parentpath(path_tmp1, &new_parentpath, &newname);
    if(strlen(newname) + 1 > MAX_FILENAME) {
        res =  -ENAMETOOLONG;
        goto end;
    }else {
        strcpy(tmp->name, newname);
        op_writeinode(tmp->inode_no, tmp);
    }
    path_tmp2 = strdup(oldpath);
    op_div_parentpath(path_tmp2, &parentpath, &name);
    if (op_path_parse(parentpath, inode_tmp) == -1) {
        res = -ENOENT;
        goto end;
    }
    i = 0;
	while (inode_tmp->datablock_no[i] != -1 && i < MAX_FILESIZE) {
	   op_readblock(inode_tmp->datablock_no[i], buff);
	   p_item = (struct item*)buff;
	   pos = 0;
	   while (pos < BLOCK_ITEMS) {
		   if (p_item->item_flag == 1 && strcmp(name, p_item->filename) == 0) {
			   strcpy(p_item->filename, newname);
			   op_writeblock(inode_tmp->datablock_no[i], buff);
			   res = 0;
			   goto end;
		   }else {
			   p_item++;
			   pos++;
		   }
	   }
	   i++;
	}
	res = -1;
	//syslog(LOG_INFO, "failed to rename ");
end:
	free(tmp);
	free(inode_tmp);
	tmp = NULL;
	inode_tmp = NULL;
	return res;
}

static int fs_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi){
    struct inode *tmp = malloc(sizeof(struct inode));
    BYTE buf_tmp[BLOCK_BYTES], *pos;
    int res, off_block, off_byte, num, size_remain, acc;
    if(tmp == NULL || off >= tmp->file_size){
        res = -1;
        goto end;
    }
    if(tmp->flag == 1) {
        res = -EISDIR;
        goto end;
    }
    if(op_path_parse(path, tmp) == -1){
        res = -ENOENT;
        goto end;
    }
    if(off + size > tmp->file_size)
        size = tmp->file_size - off;

    off_block = off / BLOCK_BYTES;
    off_byte = off % BLOCK_BYTES;
    op_readblock(tmp->datablock_no[off_block], buf_tmp);
    syslog(LOG_INFO,"read first blk,buf_tmp=%s", buf_tmp);
    pos = buf_tmp + off_byte;
    num = off_byte + size > BLOCK_BYTES ? BLOCK_BYTES - off_byte : size;
    memcpy(buf, pos, num);
    syslog(LOG_INFO,"after first read,buf=%s", buf);
    size_remain = size - num;
    acc = 0;
    off_block++;
    while(size_remain > 0){
        acc += num;
        memset(buf_tmp, 0x00, BLOCK_BYTES);
		op_readblock(tmp->datablock_no[off_block], buf_tmp);
		syslog(LOG_INFO,"another read,buf_tmp=%s", buf_tmp);
		num = size_remain > BLOCK_BYTES ? BLOCK_BYTES : size_remain;
		pos = buf_tmp;
		memcpy(buf + acc, pos, num);
		size_remain -= num;
		off_block++;
	}
	res = size;
end:
	free(tmp);
	tmp = NULL;
	return res;
}

static int fs_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
    syslog(LOG_INFO, "fs_write,path=%s,buf=%s,size=%ld,off=%ld", path, buf, size, off);
    struct inode *tmp = malloc(sizeof(struct inode));
    int i, res, block_num, block_numadd, size_remain, num, acc, off_block, off_byte, *block_add;
    BYTE buf_tmp[BLOCK_BYTES], *pos;
    if (tmp == NULL) {
        res = -1;
        goto end;
    }
    if (op_path_parse(path, tmp) == -1) {
        res = -ENOENT;
        goto end;
    }
    if (tmp->flag == 1) {
        res = -EISDIR;
        goto end;
    }
    if (off >= BLOCK_BYTES * MAX_FILESIZE) {
        res = -1;
        goto end;
    }
    if (off + size > BLOCK_BYTES * MAX_FILESIZE)
        size = BLOCK_BYTES * MAX_FILESIZE - off;
    block_num = tmp->file_size % BLOCK_BYTES == 0 ? tmp->file_size / BLOCK_BYTES : (tmp->file_size / BLOCK_BYTES + 1);
    if ((off + size) > tmp->file_size) {
        block_numadd = ((off + size) % BLOCK_BYTES == 0 ? (off + size) / BLOCK_BYTES : ((off + size) / BLOCK_BYTES + 1)) - block_num;
        //syslog(LOG_INFO, "op_write blocknum=%d,addnum=%d", block_num, block_numadd);     
        block_add = malloc(sizeof(int) * block_numadd);
        op_search_freeblock(block_numadd, block_add);
        //syslog(LOG_INFO, "op_write find free blk,blk[0]=%d", *block_add);
        i = 0;
        while (i < block_numadd) {
			tmp->datablock_no[block_num] = block_add[i];
 		    block_num++;
 		    i++;			
		}
	    tmp->file_size = off + size;
	    op_writeinode(tmp->inode_no, tmp);
    }
	
	off_block = off / BLOCK_BYTES;
    off_byte = off % BLOCK_BYTES;
    size_remain = size;
    op_readblock(tmp->datablock_no[off_block], buf_tmp);
    num = size_remain > (BLOCK_BYTES - off_byte) ? BLOCK_BYTES - off_byte : size_remain;
    acc = 0;
    pos = buf_tmp + off_byte;
    memcpy(pos, buf, num);
    //syslog(LOG_INFO, "buf_tmp=%s", buf_tmp);
    op_writeblock(tmp->datablock_no[off_block], buf_tmp);
    size_remain -= num;
    off_block++;
    while(size_remain > 0) {
	    acc += num;
	    memset(buf_tmp, 0x00, BLOCK_BYTES);
	    op_readblock(tmp->datablock_no[off_block], buf_tmp);
	    pos = buf_tmp;
	    num = size_remain > BLOCK_BYTES ? BLOCK_BYTES : size_remain;
	    memcpy(pos, buf + acc, num);
	    size_remain -= num;
	    op_writeblock(tmp->datablock_no[off_block], buf_tmp);
	    off_block++;
    }
	res = size;
end:
	free(tmp);
	free(block_add);
	tmp =  NULL;
	block_add = NULL;
	return res;
}

static struct fuse_operations fs_oper = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .mkdir = fs_mkdir,
    .mknod = fs_mknod,
    .create = fs_create,
    .unlink = fs_unlink,
    .rmdir = fs_rmdir,
    .open = fs_open,
    .opendir = fs_opendir,
    .rename = fs_rename,
    .write = fs_write,
    .read = fs_read,
};

int main(int argc, char *argv[]){
    syslog(LOG_INFO,"testlogmain");
    return fuse_main(argc, argv, &fs_oper, NULL);
}


      






