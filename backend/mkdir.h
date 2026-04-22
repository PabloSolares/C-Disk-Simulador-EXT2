#ifndef MKDIR_H
#define MKDIR_H
#include "journal.h"
#include "mount.h"
#include "structures.h"
#include "session.h"
#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

namespace CmdMkdir {

    inline int freeSpace(fstream& file, int bm_start, int max_items){
        file.clear();
        char bit;
        for(int i = 0; i < max_items; i++){
            file.seekg(bm_start + i, ios::beg);
            file.read(&bit, 1);

            if(bit == '0'){
                bit = '1';
                file.seekp(bm_start + i, ios::beg);
                file.write(&bit, 1);
                return i;
            }
        }

        return -1;
    }

    inline int findInParent(fstream& file, int sb_block_start, const Inode& parentInode, const char* targetName ){
        file.clear();
        for(int i = 0; i < 12; i++){
            if(parentInode.i_block[i] != -1){
                FolderBlock fblock;
                file.seekp(sb_block_start + parentInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                for(int j = 0; j < 4; j++){
                    if(fblock.b_content[j].b_inodo != -1 && strcmp(fblock.b_content[j].b_name, targetName) == 0){
                        return fblock.b_content[j].b_inodo;
                    }
                }
            }
        }

        return -1;
    }

    inline bool hasPermission(const Inode& inode, int uid){
        if(uid == 1) return true; //  es el root que tiene todos los permisos

        int perm = inode.i_perm;
        int ownerPerm = perm / 100;
        int otherPerm = perm % 10;

        if(inode.i_uid == uid){
            return (ownerPerm == 2 || ownerPerm == 3 || ownerPerm == 6 || ownerPerm == 7);
        } else {
            return (otherPerm == 2 || otherPerm == 3 || otherPerm == 6 || otherPerm == 7);

        }

    }

    inline int makeDir(fstream& file, Superblock& sb, int parentInodeNum, Inode& parentInode, const char* folderName, int uid){
        if (!hasPermission(parentInode, uid)) {
            return -2;
        }
        // buscamos el inodo y el bloque libre
        int newInodeNum = freeSpace(file, sb.s_bm_inode_start, sb.s_inodes_count);
        int newBlockNum = freeSpace(file, sb.s_bm_block_start, sb.s_blocks_count);

        // si no hay bloque o inodo libre 
        if (newInodeNum == -1 || newBlockNum == -1) return -1;

        Inode newFolderInode;
        for(int i = 0; i < 15; i++){
            newFolderInode.i_block[i] = -1;
        }

        newFolderInode.i_uid = uid;
        newFolderInode.i_gid = uid;

        newFolderInode.i_size = 0;
        newFolderInode.i_type = '0';
        newFolderInode.i_perm = 664;
        newFolderInode.i_block[0] = newBlockNum;

        FolderBlock newFolderBlock;
        for(int i = 0; i < 4; i++) { 
            newFolderBlock.b_content[i].b_inodo = -1;
            memset(newFolderBlock.b_content[i].b_name, 0, 12);
        }
        strncpy(newFolderBlock.b_content[0].b_name, ".", 12);
        newFolderBlock.b_content[0].b_inodo = newInodeNum;

        strncpy(newFolderBlock.b_content[1].b_name, "..", 12);
        newFolderBlock.b_content[1].b_inodo = parentInodeNum;


        //vincular la carpeta a su padre
        bool linked = false;

        for(int i = 0; i < 12 && !linked; i++){
            if(parentInode.i_block[i] != -1){
                FolderBlock parentBlock;
                file.seekg(sb.s_block_start + parentInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&parentBlock), sizeof(FolderBlock));

                for(int j = 0; j < 4; j++){
                    if(parentBlock.b_content[j].b_inodo == -1){
                        parentBlock.b_content[j].b_inodo = newInodeNum;
                        strncpy(parentBlock.b_content[j].b_name, folderName, 12);

                        file.seekp(sb.s_block_start + parentInode.i_block[i] * 64, ios::beg);
                        file.write(reinterpret_cast<char*>(&parentBlock), sizeof(FolderBlock));
                        linked = true;
                        break;
                    }
                }
            } else {
                int newParentBlock = freeSpace(file,sb.s_bm_block_start, sb.s_blocks_count);
                if(newParentBlock == -1) {
                    break;
                };

                parentInode.i_block[i] = newParentBlock;
                sb.s_free_blocks_count--; // Restamos del superbloque

                FolderBlock newParentFolderB;
                for(int k = 0; k < 4; k++){
                    newParentFolderB.b_content[k].b_inodo = -1;
                    memset(newParentFolderB.b_content[k].b_name, 0, 12);
                }

               
                newParentFolderB.b_content[0].b_inodo = newInodeNum;
                strncpy(newParentFolderB.b_content[0].b_name , folderName, 12);

                file.seekp(sb.s_block_start + newParentBlock * 64, ios::beg);
                file.write(reinterpret_cast<char*>(&newParentFolderB), sizeof(FolderBlock));
                linked = true;
            }
        }

        if(!linked){
            return -3; 
        }

        // se escribe todo en el disco
        file.seekp(sb.s_inode_start + newInodeNum * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&newFolderInode), sizeof(Inode));

        file.seekp(sb.s_block_start + newBlockNum * 64, ios::beg);
        file.write(reinterpret_cast<char*>(&newFolderBlock), sizeof(FolderBlock));

        parentInode.i_mtime = time(nullptr);
        file.seekp(sb.s_inode_start + parentInodeNum * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&parentInode), sizeof(Inode));

        // se actualiza el sistema 
        sb.s_free_inodes_count--;
        sb.s_free_blocks_count--;

        file.seekp(sb.s_bm_inode_start - sizeof(Superblock), ios::beg); 
        file.write(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        return newInodeNum;
    }
    inline string executeCmd( const string& path, bool createParents){
        

        if(!currentUser.active){
            return "Error: no ya hay una sesion activa";
        }
        // se busca la particion con el nombre de id
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(currentUser.partitionId,partition)){
            return "Error: la particion  no existe";
        }
        
        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::out | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        // Separar la ruta por diagonales '/'
        char pathCopy[256];
        strncpy(pathCopy, path.c_str(), 255);
        pathCopy[255] = '\0';

        char* context;
        char* token = strtok_r(pathCopy, "/", &context);
        
        int currentInodeN = 0; // Empezamos en la raíz
        Inode currentInode;


        while(token != nullptr){
            file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

            int nxtInodeNum = findInParent(file, sb.s_block_start, currentInode, token);
           
            if(nxtInodeNum == -1){

                char* peek = strtok_r(nullptr, "/", &context);
            
                if(peek == nullptr || createParents){
                    int result = makeDir(file, sb, currentInodeN, currentInode, token, currentUser.uid);
                    if(result == -2){
                        file.close();
                        return "Error: permiso denegado para escribir en la carpeta padre";
                    } 

                    if(result < 0 ){
                        file.close();
                        return "Error: no se pudo crear la carpeta porque el disco se encuentra lleno";
                    }

                    currentInodeN = result;
                } else {
                    file.close();
                    return  "Error: la ruta intermedia " + string(token) + " no existe. Faltan carpetas padres";
                }
                token = peek;
            } else {
                char* peek = strtok_r(nullptr, "/", &context);

                if(peek == nullptr){
                    file.close();
                    return "Error: la carpeta destino ya existe";
                }

                currentInodeN = nxtInodeNum;
                token = peek;
            }
        }

       file.close();
       if(sb.s_filesystem_type == 3){
            string contenidoBitacora = createParents ? "-p" : "";
            FsJournal::addJournalEntry(currentUser.partitionId, "mkdir", path, contenidoBitacora);
        }

        file.close();
        return "Directorio creado exitosamente";
       return "Directorio creado exitosamente";
        
    }

}


#endif // !MKDIR_H