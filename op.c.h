#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <ctype.h>
#include "op.h"
#include "ufs.h"
#include "syslog.h"

int op_readblock(int blockno, BYTE buff[BLOCK_BYTES]) {
    //syslog(LOG_INFO,"blockid=%d", blockno);
    FILE *fp = fopen(DISK,"r+");
    if(fp == NULL) {
        return -1;
    }
    fseek(fp, BLOCK_BYTES * blockno, SEEK_SET);
    fread(buff, BLOCK_BYTES, 1, fp);
    fclose(fp);

    return 0;
}

int op_writeblock(int blockno, BYTE buff[BLOCK_BYTES]) {
    FILE *fp = fopen(DISK, "r+");
    if(fp == NULL) {
        return -1;
    }
    fseek(fp, BLOCK_BYTES * blockno, SEEK_SET);
    fwrite(buff, BLOCK_BYTES, 1, fp);
    fclose(fp);

    return 0;
}

int op_search_freeblock(int n, int *block_array) {
    FILE *fp = fopen(DISK, "r+");
    int res;
    if (fp == NULL) {
        res =  -1;
        goto end;
    }
    int byte_no, bit_no;
    BYTE bmap_data, mask = 0x80;
    int i = 0, j = 131;
l1:
    while (i < n) {
l2:
        while (j < 10240) {
            byte_no = j / 8;
            bit_no = j % 8;
            mask >>= bit_no;
            //syslog(LOG_INFO, "byte_no=%d,bit_no=%d,mask=%X", byte_no, bit_no, mask);
            fseek(fp, BLOCK_BYTES + byte_no, SEEK_SET);
            fread(&bmap_data, sizeof(BYTE), 1, fp);
            bmap_data &= mask;
            if (bmap_data == mask) {
                //syslog(LOG_INFO, "occupied,find another blk");
                mask = 0x80;
                j++;
                goto l2;
            } else {
                block_array[i] = j;
                op_set_blockstatus(j, 1);
                i++;
                j++;
                mask = 0x80;
                goto l1;
		    }
		}
		res = -1;
		goto end;
	}
	res  = 0;
	//syslog(LOG_INFO, "find n=%d free blocks", n);
end:
	fclose(fp);
	return res;
}

int op_set_blockstatus(int blockno, int status) {
    //syslog(LOG_INFO, "setblkstatus blkno=%d,status=%d",blockno, status);
    FILE *fp = fopen(DISK, "r+");
    if (fp == NULL) {
        return -1;
    }
    int byte_no, bit_no;
    byte_no = blockno / 8;
    bit_no = blockno % 8;
    BYTE bmap_data, mask = 0x80;
    mask >>= bit_no;
    fseek(fp, BLOCK_BYTES + byte_no, SEEK_SET);
    fread(&bmap_data, sizeof(BYTE), 1, fp);
    //syslog(LOG_INFO, "op_setblkstatus before set bmap_data=%X", bmap_data);
    if (status)
        bmap_data |=  mask;
    else {
        mask = ~mask;
        bmap_data &= mask;
    }
    //syslog(LOG_INFO, "op_setblkstatus after set bmap_data=%X", bmap_data);
    fseek(fp, BLOCK_BYTES + byte_no, SEEK_SET);
    fwrite(&bmap_data, sizeof(BYTE), 1, fp);
    fclose(fp);
    fp = NULL;
    return 0;
}

int op_readinode(int inodeno, struct inode *tmp) {
    FILE *fp = fopen(DISK, "r+");
    if(fp == NULL) {
        return -1;
    }
    BYTE buff[BLOCK_BYTES];
    struct inode *p_inode;
    fseek(fp, BLOCK_BYTES * (5 + inodeno / 4), SEEK_SET);
    fread(buff, BLOCK_BYTES, 1, fp);
    p_inode = (struct inode*)buff;
    p_inode += inodeno % 4;

    tmp->inode_no = p_inode->inode_no;
    tmp->flag = p_inode->flag;
    strcpy(tmp->name, p_inode->name);
    tmp->file_size = p_inode->file_size;

    int i;
    for(i = 0; i < MAX_FILESIZE; i++)
        tmp->datablock_no[i] = p_inode->datablock_no[i];
    fclose(fp);
    return 0;
}

int op_writeinode(int inodeno, struct inode *tmp) {
    FILE *fp = fopen(DISK, "r+");
    if (fp == NULL) {
        return -1;
    }
    BYTE buff[BLOCK_BYTES];
    struct inode *p_inode;

    fseek(fp, BLOCK_BYTES * (5 + inodeno / 4), SEEK_SET);
    fread(buff, BLOCK_BYTES, 1, fp);
    p_inode = (struct inode *)buff;
    p_inode += inodeno % 4;
    p_inode->inode_no = tmp->inode_no;
    p_inode->flag = tmp->flag;
    strcpy(p_inode->name, tmp->name);
    p_inode->file_size = tmp->file_size;
    int i;
    for (i = 0; i < MAX_FILESIZE; i++)
        p_inode->datablock_no[i] = tmp->datablock_no[i];

    fseek(fp, BLOCK_BYTES * (5 + inodeno / 4), SEEK_SET);
    fwrite(buff, BLOCK_BYTES, 1, fp);
    fclose(fp);

    return 0;
}

int op_search_freeinode(int n, int *inode_array) {
    FILE *fp = fopen(DISK, "r+");
    int res, i, j;
    BYTE buff[BLOCK_BYTES];
    if (fp == NULL) {
        res = -1;
        goto end;
    }
    i = 0, j = 0;
l1:
    while (i < n) {
l2:
        while(j < 500) {
            fseek(fp, BLOCK_BYTES * 4 + j, SEEK_SET);
            fread(&(buff[j]), sizeof(BYTE), 1, fp);
            if (buff[j] == '0') {
                inode_array[i] = j;
                //syslog(LOG_INFO, "find free inode_no=%d", j);
                op_set_inodestatus(j, 1);
                i++;
                j++;
                goto l1;
            } else {
                j++;
                goto l2;
            }
       }
       res = -1;
       goto end;
    }
    res = 0;
end:
    fclose(fp);
	return res;
}

int op_set_inodestatus(int inodeno, int status) {
    FILE *fp = fopen(DISK, "r+");
    if (fp == NULL) {
        return -1;
    }
    BYTE ch;
    if (status == 1)
        ch = '1';
    else if (status == 0)
        ch = '0';
    fseek(fp, BLOCK_BYTES * 4 + inodeno, SEEK_SET);
    fwrite(&ch, sizeof(BYTE), 1, fp);
    fclose(fp);
    return 0;
}


int op_div_subpath(char *path, char **name, char **subpath) {
    char *path_tmp, *pos;
    path_tmp = path;
    if (*path_tmp == '/')
        path_tmp++;
    pos = strchr(path_tmp, '/');
    if (pos != NULL) {
        *pos = '\0';
        pos++;
        *name = path_tmp;
        *subpath = pos;
    } else {
        *name = path_tmp;
        *subpath = NULL;
    }
    //syslog(LOG_INFO, "op_div_subpath,name=%s,subpath=%s", *name, *subpath);

    return 0;
}

int op_div_parentpath(char *path, char **parentpath, char **name) {
    char *path_tmp, *pos;
    path_tmp = path;
    if (*path_tmp == '/')
        path_tmp++;
    pos = strrchr(path_tmp,'/');
    if (pos != NULL) {
        *pos = '\0';
        pos++;
        *parentpath = path;
        *name = pos;
    } else {
        *parentpath = "/";
        *name = path_tmp;
    }
    return 0;
}

int op_path_parse(const char *path, struct inode *tmp) {
    //syslog(LOG_INFO, "op_path_parse, path=%s", path);
    BYTE buff[BLOCK_BYTES];
    struct super_block *super;
    struct item *p_item;
    int res, i, pos;
    char *path_tmp, *p_tmp, *name, *subpath;
    op_readblock(0, buff);
    super = (struct super_block*)buff;
    op_readinode(super->rootdir_ino, tmp);
    //syslog(LOG_INFO, "rootdir_no=%d,rootdir_flag=%d,rootdir_name=%s,rootdir_size=%d,rootdir_blkno=%d",
    //                  tmp->inode_no, tmp->flag, tmp->name, tmp->file_size, tmp->datablock_no[0]);
    path_tmp = strdup(path);
    if (strcmp(path, "/") == 0) {
        res = 0;
        goto end;
    }
    p_tmp = path_tmp;
    i = 0;
	while (1) {
	   op_div_subpath(p_tmp, &name, &subpath);
	   //syslog(LOG_INFO, "inode datablk=%d", tmp->datablock_no[0]);
	   while (tmp->datablock_no[i] != -1 && i < MAX_FILESIZE) {
		   op_readblock(tmp->datablock_no[i], buff);
		   p_item = (struct item*)buff;
		   pos = 0;
		   while (pos < BLOCK_ITEMS) {
			   if (p_item->item_flag == 1) {
				   if (strcmp(name, p_item->filename) != 0) {
					   p_item++;
					   pos++;
				   } else {
					   op_readinode(p_item->inode_no, tmp);
					   //syslog(LOG_INFO, "find it: tmp->inode_no=%d,tmp->name=%s,tmp->datablock_no[0]=%d",
					   //					 tmp->inode_no, tmp->name, tmp->datablock_no[0]);
					   goto outloop;
				   }
			   } else {
				   p_item++;
				   pos++;
			   }
		   }
		   i++;
	   }
	   res = -1;
	   //syslog(LOG_INFO, "can't parse the path");
	   goto end;
outloop:
	   if (subpath == NULL) {
		   res = 0;
		   goto end;
	   } else
		   p_tmp = subpath;
   }
end:
   return res;
}

int op_isempty(const char *path) {
    struct inode *tmp = malloc(sizeof(struct inode));
    int res, pos;
    BYTE buff[BLOCK_BYTES];
    struct item *p_item;
    if (tmp == NULL) {
        res = -1;
        goto end;
    }
    if (op_path_parse(path, tmp) == -1){
        res = -ENOENT;
        goto end;
    }
    if (tmp->datablock_no[0] != -1){
        if (tmp->flag == 1) {
            op_readblock(tmp->datablock_no[0], buff);
            p_item = (struct item*)buff;
            pos = 0;
            while (pos < BLOCK_ITEMS) {
                if (p_item->item_flag == 1) {
                    res = 0;
                    goto end;
                } else {
                    p_item++;
                    pos++;
                }
            }
        }else {
            res = 0;
            goto end;
        }
    }
    res = 1;
end:
	free(tmp);
	tmp = NULL;
	return res;
}

int op_isexist(const char *path, const char *name) {
    //syslog(LOG_INFO, "call op_isexist");
    struct inode *tmp = malloc(sizeof(struct inode));
    struct item *p_item;
    int i, res, pos;
    char *parentpath, *tmp_name;
    BYTE buff[BLOCK_BYTES];
    if (tmp == NULL) {
        res = -1;
        goto end;
    }
    char *p_path;
    p_path = strdup(path);
    op_div_parentpath(p_path, &parentpath, &tmp_name);
    if(op_path_parse(parentpath, tmp) == -1) {
        res = -1;
        goto end;
    }
    i = 0;
    while (tmp->datablock_no[i] != -1 && i < MAX_FILESIZE) {
        op_readblock(tmp->datablock_no[i], buff);
        p_item = (struct item*)buff;
        pos = 0;
        while (pos < BLOCK_ITEMS) {
            if (p_item->item_flag == 1 && strcmp(p_item->filename, name) == 0) {
                res = 1;
                goto end;
            } else {
                p_item++;
                pos++;
            }
        }
        i++;
	res = 0;
end:
	free(tmp);
	tmp = NULL;
	return res; 		   /*if exist return 1*/
}

int op_rm(const char *path, int flag) {
    //syslog(LOG_INFO, "op_rm path=%s", path);
    struct inode *tmp = malloc(sizeof(struct inode));
    struct inode *inode_tmp = malloc(sizeof(struct inode));
    int res, mark, i, pos1, pos2, del_no;
    struct item *pitem1, *pitem2;
    char *parentpath, *name, *path_tmp;
    path_tmp = strdup(path);
    BYTE resetbuff[BLOCK_BYTES], buff[BLOCK_BYTES];
    memset(resetbuff, 0x00, BLOCK_BYTES);
    if (tmp == NULL || inode_tmp == NULL){
        res = -1;
        goto end;
    }
    if (flag != 0 && flag != 1) {
        res = -1;
        goto end;
    }
    if (op_path_parse(path, tmp) == -1) {
        res = -ENOENT;
        goto end;
    }
    if (flag == 1 && op_isempty(path) == 1){
        op_set_inodestatus(tmp->inode_no, 0);
        op_set_blockstatus(tmp->datablock_no[0], 0);
        tmp->datablock_no[0] = -1;
    } else if (flag == 1 && op_isempty(path) != 1) {
        res = -1;
        goto end;
    } else if (flag == 0 && op_isempty(path) == 1) {
        op_set_inodestatus(tmp->inode_no, 0);
    } else {
        i = 0;
		while (tmp->datablock_no[i] != -1 && i < MAX_FILESIZE) {
				 tmp->datablock_no[i] = -1;
				 op_writeblock(tmp->datablock_no[i], resetbuff);
				 op_set_blockstatus(tmp->datablock_no[i], 0);
				 i++;
			 }
			 op_set_inodestatus(tmp->inode_no, 0);
		 }
		 op_div_parentpath(path_tmp, &parentpath, &name);
		 if (op_path_parse(parentpath, inode_tmp) == -1) {
			 res = -ENOENT;
			 goto end;
		 }
		 i = 0;
		 while (inode_tmp->datablock_no[i] != -1 && i < MAX_FILESIZE) {
			 op_readblock(inode_tmp->datablock_no[i], buff);
			 pitem1 = (struct item*)buff;
			 pos1 = 0;
			 while (pos1 < BLOCK_ITEMS) {
				 if (pitem1->item_flag == 1 && strcmp(pitem1->filename, name) == 0) {
					 pitem1->item_flag = 0;
					 inode_tmp->file_size -= sizeof(struct item);
					 pitem2 = (struct item *)buff;
					 mark = 0;
					 pos2 = 0;
					 while (pos2 < BLOCK_ITEMS) {
						 if (pitem2->item_flag == 1) {
							 mark = 1;
							 op_writeblock(inode_tmp->datablock_no[i], buff);
							 op_writeinode(inode_tmp->inode_no, inode_tmp);
							 res = 0;
							 goto end;
						 } else {
							 pitem2++;
							 pos2++;
						 }
					 }
					 
					 if (mark == 0) {
						del_no = inode_tmp->datablock_no[i];
						int j = i + 1;
						while(j < MAX_FILESIZE && inode_tmp->datablock_no[j] != -1) {
							inode_tmp->datablock_no[j-1] = inode_tmp->datablock_no[j];
							j++;
						}
						inode_tmp->datablock_no[j] = -1;
						op_set_blockstatus(del_no, 0);
						op_writeblock(del_no, resetbuff);
						op_writeinode(inode_tmp->inode_no, inode_tmp);
						res = 0;
						goto end;
					}
			} else {
				pitem1++;
				pos1++;
			}
		}
		i++;
	}
	res = -1;
end:
	free(tmp);
	free(inode_tmp);
	tmp = NULL;
	inode_tmp = NULL;
	return res;
}

int op_create(const char *path, int flag) {
    //syslog(LOG_INFO, "op_create  path=%s, flag=%d", path, flag);
    struct inode *tmp_inode1 = malloc(sizeof(struct inode));
    struct inode *tmp_inode2 = malloc(sizeof(struct inode));
    int i, pos, res, ino, blkno, *ino_array, *block_array;
    char *path_tmp, *parentpath, *name;
    BYTE buff[BLOCK_BYTES];
    struct item *p_item;
    if (tmp_inode1 == NULL || tmp_inode2 == NULL) {
        res = -1;
        goto end;
    }
    if(flag != 0 && flag != 1){
        res = -1;
        goto end;
    }
    path_tmp = strdup(path);
    op_div_parentpath(path_tmp, &parentpath, &name);
    //syslog(LOG_INFO, "op_create  name=%s,  parentpath=%s", name, parentpath);
    if ((strlen(name) + 1) > MAX_FILENAME) {
        res = -ENAMETOOLONG;
        syslog(LOG_INFO, "the filename is too long to create!");
        goto end;
    }
    if(op_isexist(path, name) == 1) {
        res = -EEXIST;
        syslog(LOG_INFO, "the filename exists!");
        goto end;
    }
    op_path_parse(parentpath, tmp_inode1);
    //syslog(LOG_INFO, "parent inode ino=%d,flag=%d,name=%s,file_size=%d,datablk[0]=%d",
    //                       tmp_inode1->inode_no, tmp_inode1->flag, tmp_inode1->name, tmp_inode1->file_size, 
    //                            tmp_inode1->datablock_no[0]);
    ino_array = &ino;
    block_array = &blkno;
    i = 0;
    while (tmp_inode1->datablock_no[i] != -1 && i < MAX_FILESIZE) {
        op_readblock(tmp_inode1->datablock_no[i], buff);
        p_item = (struct item*)buff;
        pos = 0;
        while (pos < BLOCK_ITEMS) {
            if (p_item->item_flag == 0) {
                if (op_search_freeinode(1, ino_array) == -1) {
                    res = -1;
                    goto end;
                }
                op_set_inodestatus(*ino_array, 1);
                //syslog(LOG_INFO, "ino_array=%d", *ino_array);
                tmp_inode1->file_size += sizeof(struct item);
                op_writeinode(tmp_inode1->inode_no, tmp_inode1);
                //syslog(LOG_INFO, "afte update inode->size=%d", tmp_inode1->file_size);
                p_item->item_flag = 1;
                p_item->inode_no = *ino_array;
                strcpy(p_item->filename, name);
                op_writeblock(tmp_inode1->datablock_no[i], buff);
                tmp_inode2->inode_no = *ino_array;
                tmp_inode2->flag = flag;
                strcpy(tmp_inode2->name, name);
                if(flag == 0) {
                    tmp_inode2->file_size = 0;
                    int w = 0;
                    while(w < MAX_FILESIZE) {
                        tmp_inode2->datablock_no[w] = -1;
                        w++;
					}
				}else {
					op_search_freeblock(1, block_array);
					//syslog(LOG_INFO, "op_create allocated blkno=%d", *block_array);
					tmp_inode2->file_size = 0;
					tmp_inode2->datablock_no[0] = *block_array;
					int y = 1;
					while(y < MAX_FILESIZE) {
						tmp_inode2->datablock_no[y] = -1;
						y++;
					}
				}
				op_writeinode(*ino_array, tmp_inode2);
				
				//struct inode* test = malloc(sizeof(struct inode));
				//op_readinode(*ino_array, test);
				//syslog(LOG_INFO, "test inode_no=%d,flag=%d,name=%s,size=%d,datablkno[0]=%d", 
				//			 test->inode_no, test->flag, test->name, test->file_size, test->datablock_no[0]);
				/*
				BYTE n_buff[BLOCK_BYTES];
				op_readblock(tmp_inode1->datablock_no[0], n_buff);
				struct item *n_item;  int x = 0;
				n_item = (struct item *)n_buff;
				while(x < BLOCK_ITEMS) {
					syslog(LOG_INFO, "after write itemname=%s", n_item->filename);
					n_item++;
					x++;
				} */
				res = 0;
				goto end;

			} else {
				pos++;
				p_item++;
			}
		}
		i++;
	}
	res = -1;
	//syslog(LOG_INFO, "failed to create....");
end:
	free(tmp_inode1);
	free(tmp_inode2);
	tmp_inode1 = NULL;
	tmp_inode2 = NULL;
	return res;
}

	



