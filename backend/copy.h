#ifndef COPY_H
#define COPY_H

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include "structures.h"
#include "mount.h"
#include "journal.h"
#include "session.h"

using namespace std;

namespace CmdCopy {
    
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

    inline bool hasWritePermission(const Inode& inode, int uid) {
        if(uid == 1) return true; // Root siempre tiene permiso

        int ownerPerm = inode.i_perm / 100; 
        int otherPerm = inode.i_perm % 10;  

        if(inode.i_uid == uid) {
            return (ownerPerm == 2 || ownerPerm == 3 || ownerPerm == 6 || ownerPerm == 7);
        } else {
            return (otherPerm == 2 || otherPerm == 3 || otherPerm == 6 || otherPerm == 7);
        }
    }

    inline bool hasReadPermission(const Inode& inode, int uid) {
        if(uid == 1) return true; // root
        int ownerPerm = inode.i_perm / 100;
        int otherPerm = inode.i_perm % 10;
        if(inode.i_uid == uid) return (ownerPerm == 4 || ownerPerm == 5 || ownerPerm == 6 || ownerPerm == 7);
        else return (otherPerm == 4 || otherPerm == 5 || otherPerm == 6 || otherPerm == 7);
    }

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

    inline int getInodeNum(fstream& file, Superblock& sb, const string& path, string& targetName) {
        if(path == "/"){
            targetName = "/";
            return 0;
        }

        char pathCopy[256];
        strncpy(pathCopy, path.c_str(), 255);
        pathCopy[255] = '\0';
        char* context;
        char* token = strtok_r(pathCopy, "/", &context);
        
        int currentInodeN = 0; 
        Inode currentInode;
        while(token != nullptr){
            file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

            char* peek = strtok_r(nullptr, "/", &context);
            if (peek == nullptr) targetName = string(token);

            bool foundNext = false;
            for(int i = 0; i < 12; i++) {
                if(currentInode.i_block[i] != -1) {
                    FolderBlock fblock;
                    file.seekg(sb.s_block_start + currentInode.i_block[i] * 64 , ios::beg);
                    file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                    for(int j = 0; j < 4; j++) {
                        if(fblock.b_content[j].b_inodo != -1 && strcmp(fblock.b_content[j].b_name, token) == 0) {
                            currentInodeN = fblock.b_content[j].b_inodo;
                            foundNext = true;
                            break;
                        }
                    }
                }
                if(foundNext) break;
            }

            if (!foundNext) return -1;
            token = peek;
        }
        return currentInodeN;
    }

   inline bool linkItemToFolder(fstream& file, Superblock& sb, int destFolderInodeNum, int newChildInodeNum, const string& itemName) {
        Inode destFolder;
        file.seekg(sb.s_inode_start + destFolderInodeNum * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&destFolder), sizeof(Inode));

        for(int i = 0 ; i < 12; i++){
            if(destFolder.i_block[i] != -1){
                FolderBlock fb;
                int fbPos = sb.s_block_start + destFolder.i_block[i] * 64;
                file.seekg(fbPos, ios::beg);
                file.read(reinterpret_cast<char*>(&fb), sizeof(FolderBlock));

                for(int j = 0; j < 4; j++){
                    if(fb.b_content[j].b_inodo == -1){
                        fb.b_content[j].b_inodo = newChildInodeNum;
                        strncpy(fb.b_content[j].b_name, itemName.c_str(), 11);
                        file.seekp(fbPos, ios::beg);
                        file.write(reinterpret_cast<char*>(&fb), sizeof(FolderBlock));
                        return true;
                    }
                }
            } else { // si el bloque no existe
                int newBlock = freeSpace(file, sb.s_bm_block_start, sb.s_blocks_count);
                if (newBlock == -1) return false;
                sb.s_free_blocks_count--;
                destFolder.i_block[i] = newBlock;
                
                FolderBlock fb;
                for(int k = 0; k < 4; k++) {
                    fb.b_content[k].b_inodo = -1;
                    memset(fb.b_content[k].b_name, 0, 12);
                }

                fb.b_content[0].b_inodo = newChildInodeNum;
                strncpy(fb.b_content[0].b_name, itemName.c_str(), 11);
                
                file.seekp(sb.s_block_start + newBlock * 64, ios::beg);
                file.write(reinterpret_cast<char*>(&fb), sizeof(FolderBlock));

                file.seekp(sb.s_inode_start + destFolderInodeNum * sizeof(Inode), ios::beg);
                file.write(reinterpret_cast<char*>(&destFolder), sizeof(Inode));
                return true;
            }
        }

        return false;
    }

    inline int copyItem(fstream& file, Superblock& sb, int srcInodeNum, int destFolderInodeNum, int uid) {
        Inode srcInode;
        file.seekg(sb.s_inode_start + srcInodeNum * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&srcInode), sizeof(Inode));

        if(!hasReadPermission(srcInode, uid)){
            return -1;
        }

        int newInodeNum = freeSpace(file, sb.s_bm_inode_start , sb.s_inodes_count);
        if(newInodeNum == -1)  return -1;
        sb.s_free_inodes_count--;

        Inode newInode = srcInode;
        newInode.i_uid = uid;
        newInode.i_atime = time(nullptr);
        newInode.i_ctime = time(nullptr);
        newInode.i_mtime = time(nullptr);
        for(int i = 0; i < 15; i++){
            newInode.i_block[i] = -1;
        }

        if(srcInode.i_type == '1'){
            for(int i = 0; i < 12; i++){
                if(srcInode.i_block[i] != -1){
                    int newBlockNum = freeSpace(file, sb.s_bm_block_start, sb.s_blocks_count);

                    if (newBlockNum == -1) return -1;
                    sb.s_free_blocks_count--;
                    newInode.i_block[i] = newBlockNum;

                    FileBlock srcBlock;
                    file.seekg(sb.s_block_start + srcInode.i_block[i] * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&srcBlock), sizeof(FileBlock));
                    
                    file.seekp(sb.s_block_start + newBlockNum * 64, ios::beg);
                    file.write(reinterpret_cast<char*>(&srcBlock), sizeof(FileBlock));
                }
            }
        } else if (srcInode.i_type == '0'){
            int newBlockNum = freeSpace(file, sb.s_bm_block_start, sb.s_blocks_count);
            if(newBlockNum == -1){
                return -1;
            }
            sb.s_free_blocks_count--;
            newInode.i_block[0] = newBlockNum;

            FolderBlock newFolderBlock;
            for(int i = 0; i < 4; i++){
                newFolderBlock.b_content[i].b_inodo = -1; 
                memset(newFolderBlock.b_content[i].b_name, 0, 12);
            }

            strncpy(newFolderBlock.b_content[0].b_name, ".", 12);
            newFolderBlock.b_content[0].b_inodo = newInodeNum;
            strncpy(newFolderBlock.b_content[1].b_name, "..", 12);
            newFolderBlock.b_content[1].b_inodo = destFolderInodeNum;

            file.seekp(sb.s_block_start + newBlockNum * 64, ios::beg);
            file.write(reinterpret_cast<char*>(&newFolderBlock), sizeof(FolderBlock));

            for(int i = 0; i < 12; i++){
                if(srcInode.i_block[i] != -1){
                    FolderBlock srcFb;
                    file.seekg(sb.s_block_start + srcInode.i_block[i] * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&srcFb), sizeof(FolderBlock));

                    for (int j = 0; j < 4; j++) {
                        if (srcFb.b_content[j].b_inodo != -1 && 
                            strcmp(srcFb.b_content[j].b_name, ".") != 0 && 
                            strcmp(srcFb.b_content[j].b_name, "..") != 0) {
                            
                            int childCopiedInode = copyItem(file, sb, srcFb.b_content[j].b_inodo, newInodeNum, uid);
                            if (childCopiedInode != -1) {
                                linkItemToFolder(file, sb, newInodeNum, childCopiedInode, string(srcFb.b_content[j].b_name));
                            }
                        }
                    }
                }
            }
        }
      
        file.seekp(sb.s_inode_start + newInodeNum * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&newInode), sizeof(Inode));
        return newInodeNum;
    }

    inline string exucuteCmd(const string& path, const string& destino){
        if(!currentUser.active) return "Error: no hay una sesion activa";
        if(path.empty() || destino.empty()) return "Error: copy requiere -path y -destino";
        if(path == destino) return "Error: el origen y destino no puse ser iguales";

        CmdMount::MountedPartition partition;
        string currentId = currentUser.partitionId;
        if(!CmdMount::getMountedPar(currentId, partition)) return "Error: la particion no esta montada";

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::binary | ios::in | ios::out);
        if (!file.is_open()) return "Error: No se pudo abrir el disco.";

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        string sourceName = "";
        int srcInodeNum = getInodeNum(file, sb, path, sourceName);
        if(srcInodeNum == -1){
            file.close();
            return "Error: el archivo o carpeto origen " + path + " no existe";
        }

        string destName = "";
        int destInodeNum = getInodeNum(file, sb, destino, destName);
        if (destInodeNum == -1) {
            file.close(); return "Error: La carpeta destino '" + destino + "' no existe.";
        }

        int copiedInodeNum = copyItem(file, sb, srcInodeNum, destInodeNum, currentUser.uid);
        if (copiedInodeNum == -1) {
            file.close(); 
            return "Error: No se pudo copiar el elemento (Falta de espacio o permisos de lectura).";
        }

        bool linked = linkItemToFolder(file, sb, destInodeNum, copiedInodeNum, sourceName);
        if (!linked) {
            file.close(); 
            return "Error: No hay espacio en la carpeta destino para agregar el elemento.";
    
        }

        file.seekp(partition.start, ios::beg);
        file.write(reinterpret_cast<char*>(&sb), sizeof(Superblock));
        file.close();

        if(sb.s_filesystem_type == 3){
            string contenidoBitacora = "-destino=" + destino;
            FsJournal::addJournalEntry(currentUser.partitionId, "copy", path, contenidoBitacora);
        }

        return "Copia hacia '" + destino + "' realizada exitosamente.";
    }
}

#endif // !COPY_H