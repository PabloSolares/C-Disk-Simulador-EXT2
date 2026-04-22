#ifndef STRUCTURES_H
#define STRUCTURES_H 
# include <ctime>
# include <cstring>

#include <iostream>

struct Partition
{
	char part_status;
	char part_type;
	char part_fit;
	int part_start;
	int part_size;
	char part_name[16];

	Partition(){ 
		part_status = '0';
		part_type = '\0';
		part_fit = '\0';
		part_start = -1;
		part_size = 0;
		memset(part_name, 0 , sizeof(part_name));
	
	}
};

struct MBR
{
	int mbr_size;
	time_t mbr_creation_date;
	int mbr_disk_signature;
	char disk_fit;
	Partition mbr_partitions[4];
	
	MBR(){ 
		mbr_size = 0;
		mbr_creation_date = time(nullptr);
		mbr_disk_signature = rand();
		disk_fit = 'F';
	}
};

struct EBR {
    char part_status;
    char part_fit;
    int part_start;     
    int part_size;      
    int part_next;
    char part_name[16];
    
    EBR() { 
        part_status = '0';
        part_fit = '\0';
        part_start = -1;
        part_size = 0;
        part_next = -1;
        memset(part_name, 0 , sizeof(part_name));
    }
};

struct Superblock{
	int s_filesystem_type;
	int s_inodes_count;
	int s_blocks_count;
	int s_free_blocks_count;
	int s_free_inodes_count;

	time_t s_mtime;
	time_t s_umtime;
	int s_inode_size;
	int s_block_size;
	int s_first_ino;

	int s_mnt_count;
	int s_magic;
	int s_inode_s;
	int s_block_s;
	int s_firts_ino;
	int s_first_blo;
	int s_bm_inode_start;
	int s_bm_block_start;
	int s_inode_start;
	int s_block_start;
	Superblock(){
		s_filesystem_type = 0;
        s_inodes_count = 0;
        s_blocks_count = 0;
        s_free_blocks_count = 0;
        s_free_inodes_count = 0;
        s_mtime = time(nullptr);
        s_umtime = time(nullptr);
        s_mnt_count = 0;
        s_magic = 0xEF53;
        s_inode_size = 0;  // Se inicializa en mkfs
        s_block_size = 64;
        s_first_ino = 0;
        s_first_blo = 0;
        s_bm_inode_start = 0;
        s_bm_block_start = 0;
        s_inode_start = 0;
        s_block_start = 0;
	}
};

struct Inode {
	int i_uid;                     
    int i_gid;                    
    int i_size;                    
    time_t i_atime;             
    time_t i_ctime;                
    time_t i_mtime;                
    int i_block[15];
    char i_type;                   
    int i_perm;       

	Inode() {
        i_uid = 0;
        i_gid = 0;
        i_size = 0;
        i_atime = time(nullptr);
        i_ctime = time(nullptr);
        i_mtime = time(nullptr);
        for (int i = 0; i < 15; i++) {
            i_block[i] = -1;
        }
        i_type = '0';
        i_perm = 664;
    }

};

struct Content {
	char b_name[12];
	int b_inodo;

	Content(){
		memset(b_name, 0, sizeof(b_name));
		b_inodo = -1;
	}
};

struct FolderBlock {
	Content b_content[4];

	FolderBlock(){

	}
};



struct FileBlock {
	char b_content[64];
	FileBlock (){
		memset(b_content, 0, sizeof(b_content));
	}
};

struct PointerBlock{
	int b_pointers[16];

	PointerBlock(){
		for(int i = 0; i < 16; i++){
			b_pointers[i] = -1;
		}
	}
};



struct LoggedUser {
	bool active;
	int uid ;
	char username[12];
	char group[12];
	char partitionId[12];

	LoggedUser(){
		active = false;
		uid = -1;
		memset(username, 0, sizeof(username));
		memset(group, 0, sizeof(username));
		memset(partitionId, 0, sizeof(username));

	}
};

struct Information {
	char i_operation[10];
	char i_path[32];    
    char i_content[64];
	time_t i_date;

	Information(){
		memset(i_operation, 0 ,sizeof(i_operation));
		memset(i_path, 0 ,sizeof(i_path));
		memset(i_content, 0 ,sizeof(i_content));
		i_date = 0;
	}

};


struct Journal {
	int j_count;
	Information j_content;

	Journal(){
		j_count = 0;
	}
};

#endif // !STRUCTURES_H



