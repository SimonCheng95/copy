#include "ufs.h"
#include "syslog.h"

int  main(void){
    int res;
    struct super_block *super = malloc(sizeof(struct super_block));
    struct inode *rootdir_inode = malloc(sizeof(struct inode));
    if (super == NULL) {
        res = -1;
        goto end;
    } 
    if (rootdir_inode == NULL) {
        res = -1;
        goto end;
    }     
    FILE *fp; 
    fp = fopen(DISK,"r+");
    if (fp == NULL) {
        res = -1;
        goto end;
    }
    super->rootdir_ino = 0;
    super->block_bitmap_start = 1;
    super->inode_bitmap_start = 4;
    super->inode_table_start = 5;
    super->data_block_start = 130;
    if (fseek(fp, 0, SEEK_SET) != 0) {
        res = -1;
        goto end;
    }
    fwrite(super, sizeof(struct super_block), 1, fp);
    //printf("super->rootdir_ino=%d,super->blkmap=%d,super->inodemap=%d,super->inodetable=%d,super->datablk=%d \n",
                       //super->rootdir_ino, super->block_bitmap_start, super->inode_bitmap_start, super->inode_table_start, 
                       //   super->data_block_start);

    BYTE fill_one = 0xff;
    BYTE fill_a = 0xe0;
    BYTE fill_zero = 0x00;
    BYTE fill_1 = '1';
    BYTE fill_2 = '0';
    int count;
    //printf("current fp=%ld  \n", ftell(fp));
    if (fseek(fp, BLOCK_BYTES * 1, SEEK_SET) != 0) {
        res = -1;
        goto end;
    }
    //printf("current fp=%ld \n", ftell(fp));
    count = 16;
    while (count > 0) {
        fwrite(&fill_one, sizeof(char), 1, fp);
        count--;
    }
    fwrite(&fill_a, sizeof(char), 1, fp);
    count = BLOCK_BYTES * 3 - 17;
    while (count > 0) {
        fwrite(&fill_zero, sizeof(char), 1, fp);
        count --;
    }
    //---------------------test-----------------------
    /*fseek(fp, BLOCK_BYTES, SEEK_SET);
    int i = BLOCK_BYTES * 3;
    BYTE a;
    while(i > 0) { 
        fread(&a, 1, 1, fp);
        printf("%X\t ", a);
        if(i % 10 == 0){
            printf("\n");
        }
        i--;
    } 
    printf("\n");  */

    /*init inode_bitmap*/
    if (fseek(fp, BLOCK_BYTES * 4, SEEK_SET) != 0) {
        res = -1;
        goto end;
    }
    //printf("current fp=%ld \n", ftell(fp));
    fwrite(&fill_1, sizeof(char), 1, fp);
    count = BLOCK_BYTES - 1;
    while (count > 0) {
        fwrite(&fill_2, sizeof(char), 1, fp);
        count--;
    }
    //--------------------------test-----------------------
    /*BYTE a;
    fseek(fp, BLOCK_BYTES * 4, SEEK_SET);
    printf("current fp=%ld \n", ftell(fp));  
    count = BLOCK_BYTES;  int z = 0;
	while (count) {
	   fread(&a, 1, 1, fp);
	   printf("%c\t", a);
	   if(z % 10 == 0) printf("\n");
	   z++;
	   count--;
    }
    printf("\n");
    printf("z=%d \n", z); */

    /*init rootdir_inode*/
    rootdir_inode->inode_no = 0;
    rootdir_inode->flag = 1;
    strcpy(rootdir_inode->name, "/");
    rootdir_inode->file_size = 0;
    count = MAX_FILESIZE;
    rootdir_inode->datablock_no[0] = 130;
    count--;
    while(count > 0) {
 	    rootdir_inode->datablock_no[count] = -1;
	    count--;
    }
    if (fseek(fp, BLOCK_BYTES * 5, SEEK_SET) != 0) {
	    res = -1;
	    goto end;
    }
    fwrite(rootdir_inode, sizeof(struct inode), 1, fp);
   
    //--------------------test------------------------
    /*struct inode *test = malloc(sizeof(struct inode));
    fseek(fp, BLOCK_BYTES * 5, SEEK_SET);
    fread(test, sizeof(struct inode), 1, fp);
    printf("inodeno=%d,flag=%d,name=%s,size=%d,datablkno[0]=%d \n",test->inode_no, test->flag, test->name, test->file_size,
 								   test->datablock_no[0]);
    res = 0;  */
    /*	 
    BYTE reset[BLOCK_BYTES];
    memset(reset, 0x00, BLOCK_BYTES);
    fseek(fp, BLOCK_BYTES * 130, SEEK_SET); 
    fwrite(reset, BLOCK_BYTES, 1, fp);	  
	  
    BYTE a;
    fseek(fp, BLOCK_BYTES * 130, SEEK_SET);
    printf("current fp=%ld \n", ftell(fp));	
    count = BLOCK_BYTES;  int z = 0;
    while (count) {
	    fread(&a, 1, 1, fp);
	    printf("%X\t", a);
	    if(z % 10 == 0) printf("\n");
	    z++;
	    count--;
    }
    printf("\n");
    printf("z=%d \n", z);   */
end:
    fclose(fp);
    free(super);
    free(rootdir_inode);
	
    return res;
}
                                                 


