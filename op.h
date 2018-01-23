
#ifndef OP_H
#define OP_H

#include "ufs.h"

extern int op_readblock(int blockno, BYTE buff[BLOCK_BYTES]);
extern int op_writeblock(int blockno, BYTE buff[BLOCK_BYTES]);
extern int op_search_freeblock(int n, int *block_array);
extern int op_set_blockstatus(int blockno, int status);
extern int op_readinode(int inodeno, struct inode *tmp);
extern int op_writeinode(int inodeno, struct inode *tmp);
//extern int op_inode_modify(const char *path, struct inode *tmp);
extern int op_search_freeinode(int n, int *inode_array);
extern int op_set_inodestatus(int inodeno, int status);
extern int op_div_subpath(char *path, char **name, char **subpath);
extern int op_div_parentpath(char *path, char **parentpath, char **name);
extern int op_path_parse(const char *path, struct inode *tmp);
extern int op_isexist(const char *path, const char *name);
extern int op_isempty(const char *path);
extern int op_rm(const char *path, int flag);
extern int op_create(const char *path, int flag);
#endif
    
~                                                                                                                                            
~                        


