#define BLOCK_BYTES 512
#define SUPER_BLOCK_SIZE 1
#define BLOCK_BITMAP_SIZE 3
#define INODE_BITMAP_SIZE 1
#define INODE_TABLE_SIZE 125
#define BLOCK_ITEMS 21
#define BYTE unsigned char

struct super_block{
	int rootdir_ino;
	int block_bitmap_start;
	int inode_bitmap_start;
	int inode_table_start; 
	int data_block_start;
};        
         
struct inode{
    int inode_no;
    int flag;     /*file=1:directory,file=0:file*/
    char name[MAX_FILENAME];
    int file_size;
    int datablock_no[MAX_FILESIZE];
};  
    
struct item{ 
    int item_flag;  /*free 0;busy 1*/
    int inode_no;
    char filename[MAX_FILENAME];
};  
    
#endif 

