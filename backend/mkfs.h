#ifndef MKFS_H
#define MKFS_H

#include "mount.h"
#include "structures.h"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ios>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cctype>
#include <map>
#include <iostream>

using namespace std;

namespace CmdMkfs {
    inline string toLowerString(const string& str){
        string result = str;

        transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    inline string CmdExecute (const string& id, const string& type, const string& param_fs){
        if(id.empty()){
            return "Error: mkfs requiere el parametro -id";
        }

        string formatType = toLowerString(type);

        if(formatType.empty()){
            formatType = "full";
        }

        if(formatType != "full"){
            return "Error: type debe ser full";
        }

        

        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id, partition)){
            return "Error: la particion con ID " + id + " no existe";
        }

        fstream file(partition.path, ios::binary | ios::in | ios::out);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path ;

        }

        int partitionSize = partition.size;

        string fsType = "2fs";

         if(param_fs == "3fs" || param_fs == "3FS"){
            fsType = "3fs";
        }
        int n = 0;

        if(fsType == "3fs"){
            n = floor((partitionSize - sizeof(Superblock) - (50 * sizeof(Journal))) / (4 + sizeof(Inode) + 3 * 64));
        } else {
            n = floor((partitionSize - sizeof(Superblock)) / (4 + sizeof(Inode) + 3 * 64));
        }

        if( n <= 0){
            file.close();
            return "ERROR: la particion es muy pequeña para crear un sistema de archivos";
        }

        Superblock sb;
        sb.s_filesystem_type = (fsType == "3fs") ? 3 : 2;  
        sb.s_inodes_count = n;
        sb.s_blocks_count = 3 * n;
        sb.s_free_blocks_count = 3 * n - 2;  
        sb.s_free_inodes_count = n - 2;     
        sb.s_mtime = time(nullptr);
        sb.s_umtime = time(nullptr);
        sb.s_mnt_count = 1;
        sb.s_magic = 0xEF53;
        sb.s_inode_size = sizeof(Inode);
        sb.s_block_size = 64;
        sb.s_first_ino = 2;  
        sb.s_first_blo = 2;  
        
        int superblock_start = partition.start;
        int journal_start = superblock_start + sizeof(Superblock);

        if(fsType == "3fs" ){
            sb.s_bm_inode_start = journal_start + (50 * sizeof(Journal)); 
        } else {
            sb.s_bm_inode_start = superblock_start + sizeof(Superblock);
        }

        sb.s_bm_block_start = sb.s_bm_inode_start + n;
        sb.s_inode_start = sb.s_bm_block_start +( 3 * n);
        sb.s_block_start = sb.s_inode_start + n * sizeof(Inode);

        file.seekp(partition.start, ios::beg);
        file.write(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        if(fsType == "3fs"){
            Journal emptyJournal;
            file.seekp(journal_start, ios::beg);
            for(int i = 0; i < 50; i++){
                file.write(reinterpret_cast<char*>(&emptyJournal), sizeof(Journal));
            }
        }

        file.seekp(sb.s_bm_inode_start, ios::beg);
        for(int i = 0; i < n ; i++){
            char bit = ( (i == 0) | (i == 1) ) ? '1': '0';
            file.write(&bit, 1);

        }

        file.seekp(sb.s_bm_block_start, ios::beg);
        for(int i = 0; i < 3 * n; i++){
            char bit = (i == 0 || i == 1) ? '1' : '0';
            file.write(&bit, 1);
        }

        Inode rootInode;
        for(int i = 0; i < 15; i++) {
            rootInode.i_block[i] = -1;
        }
        rootInode.i_uid = 1;
        rootInode.i_gid = 1;
        rootInode.i_size = 0;
        rootInode.i_atime = time(nullptr);
        rootInode.i_ctime = time(nullptr);
        rootInode.i_mtime = time(nullptr);

        rootInode.i_type = '0';
        rootInode.i_perm = 664;
        rootInode.i_block[0] = 0;

        file.seekp(sb.s_inode_start, ios::beg);
        file.write(reinterpret_cast<char*>(&rootInode), sizeof(Inode));

        string usersCont = "1, G, root\n1, U, root, root, 123 \n";

        Inode usersInode;
        for(int i = 0; i < 15; i++){ 
            usersInode.i_block[i] = -1;
        }
        usersInode.i_uid = 1;
        usersInode.i_gid = 1;
        usersInode.i_size = usersCont.length();  
        usersInode.i_atime = time(nullptr);
        usersInode.i_ctime = time(nullptr);
        usersInode.i_mtime = time(nullptr);
        usersInode.i_type = '1';  
        usersInode.i_perm = 664;
        usersInode.i_block[0] = 1; 
        
        file.seekp(sb.s_inode_start + sizeof(Inode), std::ios::beg);
        file.write(reinterpret_cast<const char*>(&usersInode), sizeof(Inode));

        FolderBlock rootBlock;
        for(int i = 0; i < 4; i++) { 
            rootBlock.b_content[i].b_inodo = -1;
            memset(rootBlock.b_content[i].b_name, 0, 12);
        }
        strncpy(rootBlock.b_content[0].b_name, ".", 12);
        rootBlock.b_content[0].b_inodo = 0;

        strncpy(rootBlock.b_content[1].b_name, "..", 12);
        rootBlock.b_content[1].b_inodo = 0;

        strncpy(rootBlock.b_content[2].b_name, "users.txt", 12);
        rootBlock.b_content[2].b_inodo = 1;

        rootBlock.b_content[3].b_inodo = -1;

        file.seekp(sb.s_block_start, std::ios::beg);
        file.write(reinterpret_cast<const char*>(&rootBlock), sizeof(FolderBlock));

        FileBlock usersBlock;
        memset(usersBlock.b_content, 0, 64);
        strncpy(usersBlock.b_content, usersCont.c_str(), 64);

        file.seekp(sb.s_block_start + 64, ios::beg);
        file.write(reinterpret_cast<char*>(&usersBlock), sizeof(FileBlock));

        file.close();

        ostringstream result;
        result << "\n ***************** MKFS ***************** \n";
        result << "Sistema de archivos EXT" << ((fsType == "3fs") ? "3" : "2") << " creado exitosamente\n";
        result << "  ID: " << id << "\n";
        result << "  Disco: " << partition.path << "\n";
        result << "  Partición: " << partition.name << "\n";
        result << "  Tamaño: " << partitionSize << " bytes\n";
        result << "  Inodos: " << n << "\n";
        result << "  Bloques: " << (3 * n) << "\n";
        result << "  Archivo users.txt creado en la raíz";
        return result.str();
    }
}
#endif // !MKFS_H