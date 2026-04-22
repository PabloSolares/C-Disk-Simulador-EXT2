#ifndef REMOVE_H 
#define REMOVE_H


#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <fstream>
#include "structures.h"
#include "mount.h"
#include "journal.h"
#include "session.h"

using namespace std;

namespace CmdRemove {

    
    inline bool hasPermission(const Inode& inode, int uid){
        if(uid == 1) return true; // root

        int perm = inode.i_perm;
        int ownerPerm = perm / 100;
        int otherPerm = perm % 10;

        if(inode.i_uid == uid){
            return (ownerPerm == 2 || ownerPerm == 3 || ownerPerm == 6 || ownerPerm == 7);
        } else {
            return (otherPerm == 2 || otherPerm == 3 || otherPerm == 6 || otherPerm == 7);
        }
    }

    inline bool recursiveDelete(fstream& file, int inodeNum,Superblock& sb, int partitionStart, int loggedUid) {
        Inode currentInode;
        file.seekg(sb.s_inode_start + (inodeNum * sizeof(Inode)), ios::beg);
        file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

        if(!hasPermission(currentInode, loggedUid)){
            return false;
        }

        // carpeta
        if(currentInode.i_type == 0){
            for(int i = 0; i < 12; i++){
                if(currentInode.i_block[i] != -1){
                    FolderBlock fBlock;
                    int blockPos = sb.s_block_start + (currentInode.i_block[i] * 64);
                    file.seekg(blockPos, ios::beg);
                    file.read(reinterpret_cast<char*>(&fBlock), sizeof(FolderBlock));

                    for(int j = 0; i < 4; j++){
                        if(fBlock.b_content[j].b_inodo != -1 && strcmp(fBlock.b_content[j].b_name, ".") != 0 && strcmp(fBlock.b_content[j].b_name, "..") != 0){
                            bool childDeleted = recursiveDelete(file, fBlock.b_content[j].b_inodo, sb, partitionStart,loggedUid);
                            if(!childDeleted){
                                return false;
                            }
                        }
                    }

                    char zero = '0';
                    file.seekp(sb.s_bm_block_start + currentInode.i_block[i], ios::beg);
                    file.write(&zero, 1);
                    sb.s_free_blocks_count++;
                }
            }
        } else {
            for(int i = 0; i < 15; i++){
                if(currentInode.i_block[i] != -1){
                    char zero = '0';
                    file.seekp(sb.s_bm_block_start + currentInode.i_block[i], ios::beg);
                    file.write(&zero, 1);
                    sb.s_free_blocks_count++;

                }
            }
        }


        char zero = '0';
        file.seekp(sb.s_bm_inode_start + inodeNum, ios::beg);
        file.write(&zero, 1);
        sb.s_free_inodes_count++;

        return true;
    } 

    inline string exucuteCmd(const string& path){
        if(!currentUser.active){
            return "Error: no hay una sesion activa";
        }
        
        if (path.empty() || path == "/") {
            return "Error: remove requiere el parametro -path y no puede borrar la raiz.";
        }

        CmdMount::MountedPartition partition;
        string currentId = currentUser.partitionId;
        if (!CmdMount::getMountedPar(currentId, partition)) {
            return "Error: la particion con ID " + currentId + " no esta montada.";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::binary | ios::in | ios::out);
        if (!file.is_open()) return "Error: No se pudo abrir el disco.";

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        char pathCopy[256];
        strncpy(pathCopy, path.c_str(), 255);
        pathCopy[255] = '\0';

        char* context;
        char* token = strtok_r(pathCopy, "/", &context);
        
        int currentInodeN = 0; 
        Inode currentInode;
        
        int parentInodeN = -1; 
        string targetName = "";

        while (token != nullptr) {
            file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

            char* peek = strtok_r(nullptr, "/", &context);

            if(peek == nullptr){
                parentInodeN = currentInodeN;
                targetName = string(token);
                break;
            }
            
            bool foundNext = false;
            for(int i = 0; i < 12; i++){
                if(currentInode.i_block[i] != -1){
                    FolderBlock fblock;
                    file.seekg(sb.s_block_start + currentInode.i_block[i] * 64 , ios::beg);
                    file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                    for(int j = 0; j < 4; j++){
                        if(fblock.b_content[j].b_inodo != -1 && strcmp(fblock.b_content[j].b_name,token) == 0 ){
                            currentInodeN = fblock.b_content[j].b_inodo;
                            foundNext = true;
                            break;
                        }
                    }
                }
                if(foundNext) break;
            }

            if (!foundNext) {
                file.close();
                return "Error: La ruta '" + string(token) + "' no existe.";
            }
            token = peek;
        }

        Inode parentInode;
        file.seekg(sb.s_inode_start + parentInodeN * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&parentInode), sizeof(Inode));

        int targetInodeNum = -1;
        int parentBlockPos = -1; 
        int parentContentIndex = -1;

        for(int i = 0; i < 12; i++){
            if(parentInode.i_block[i] != -1){
                FolderBlock fblock;
                int fbPos = sb.s_block_start + parentInode.i_block[i] * 64;
                file.seekg(fbPos , ios::beg);
                file.read(reinterpret_cast<char*>(&fblock) , sizeof(FolderBlock));

                for(int j = 0; j < 4; j++){
                    if(fblock.b_content[j].b_inodo != -1){
                        if(strcmp(fblock.b_content[j].b_name, targetName.c_str()) == 0){
                            targetInodeNum = fblock.b_content[j].b_inodo;
                            parentBlockPos = fbPos;
                            parentContentIndex = j;
                            break;
                        }
                    }
                }
                if(targetInodeNum != -1) {
                    break;
                }
            }
        }

        if (targetInodeNum == -1) {
            file.close(); return "Error: El archivo o carpeta a eliminar no existe.";
        }

        bool success = recursiveDelete(file, targetInodeNum, sb, partition.start, currentUser.uid);

        if (!success) {
            file.close();
            return "Error: Permiso denegado sobre la carpeta o alguno de sus elementos internos.";
        }

        FolderBlock parentFbToUpdate;
        file.seekg(parentBlockPos, ios::beg);
        file.read(reinterpret_cast<char*>(&parentFbToUpdate), sizeof(FolderBlock));

        memset(parentFbToUpdate.b_content[parentContentIndex].b_name, 0, 12);
        parentFbToUpdate.b_content[parentContentIndex].b_inodo = -1;

        file.seekp(parentBlockPos, ios::beg);
        file.write(reinterpret_cast<char*>(&parentFbToUpdate), sizeof(FolderBlock));

        file.seekp(partition.start, ios::beg);
        file.write(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        file.close();

        if(sb.s_filesystem_type == 3){
            FsJournal::addJournalEntry(currentUser.partitionId, "remove", path, "");
        }

        return "Elemento '" + targetName + "' eliminado exitosamente.";
    }
}

#endif // !REMOVE_H 