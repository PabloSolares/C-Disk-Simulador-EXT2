#ifndef RENAME_H
#define RENAME_H
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "structures.h"
#include "mount.h"
#include "journal.h"
#include "session.h"
using namespace std;

namespace CmdRename {

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

    inline string executeCmd( const string& path, const string& newName) {
        if(!currentUser.active){
            return "Error: no hay una sesion activa";
        }

        if (path.empty() || newName.empty()) {
            return "Error: rename requiere los parametros -path y -name";
        }

        if (newName.length() > 11) {
            return "Error: El nuevo nombre no puede exceder los 11 caracteres.";
        }

        CmdMount::MountedPartition partition;
        string currentId = currentUser.partitionId;
        if (!CmdMount::getMountedPar(currentId, partition)) {
            return "Error: la particion con ID " + currentId + " no esta montada.";
        }

        fstream file(partition.path, ios::binary | ios::in | ios::out);
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
                if(foundNext )break;

            }

            if (!foundNext) {
                file.close();
                return "Error: La ruta intermedia '" + string(token) + "' no existe.";
            }

            token = peek;
        }
            
        Inode parentInode;
        file.seekg(sb.s_inode_start + parentInodeN  * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&parentInode), sizeof(Inode));

        int targetInodeNum = -1;
        int blockToUpdatePos = -1;
        int contentIndexToUpdate = -1;
        bool duplicateFound = false;
        
        for(int i = 0; i < 12; i++){
            if(parentInode.i_block[i] != -1){
                FolderBlock fblock;
                int fbPos = sb.s_block_start + parentInode.i_block[i] * 64;
                file.seekg(fbPos , ios::beg);
                file.read(reinterpret_cast<char*>(&fblock) , sizeof(FolderBlock));

                for(int j = 0; j < 4; j++){
                    if(fblock.b_content[j].b_inodo != -1){
                        if(strcmp(fblock.b_content[j].b_name, targetName.c_str() ) == 0){
                            targetInodeNum = fblock.b_content[j].b_inodo;
                            blockToUpdatePos = fbPos;
                            contentIndexToUpdate = j;
                        }

                        if(strcmp(fblock.b_content[j].b_name, newName.c_str()) == 0){
                            duplicateFound = true;
                        }
                    }
                }
            }
        }

        if (targetInodeNum == -1) {
            file.close(); return "Error: El archivo o carpeta destino no existe.";
        }

        if (duplicateFound) {
            file.close(); return "Error: Ya existe un elemento con el nombre '" + newName + "' en esta carpeta.";
        }

        Inode targetInode;
        file.seekg(sb.s_inode_start + targetInodeNum * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&targetInode), sizeof(Inode));

        if (!hasPermission(targetInode, currentUser.uid)) {
            file.close();
            return "Error: Permiso denegado. No tienes acceso de escritura para renombrar este elemento.";
        }

        FolderBlock fbToUpdate;
        file.seekg(blockToUpdatePos, ios::beg);
        file.read(reinterpret_cast<char*>(&fbToUpdate), sizeof(FolderBlock));

        memset(fbToUpdate.b_content[contentIndexToUpdate].b_name, 0, 12);
        strncpy(fbToUpdate.b_content[contentIndexToUpdate].b_name, newName.c_str(), 11);

        file.seekp(blockToUpdatePos, ios::beg);
        file.write(reinterpret_cast<char*>(&fbToUpdate), sizeof(FolderBlock));


        file.close();

        if(sb.s_filesystem_type == 3){
            string contenidoBitacora = "-name=" + newName;
            FsJournal::addJournalEntry(currentUser.partitionId, "rename", path, contenidoBitacora);
        }

        return "Elemento renombrado a '" + newName + "' exitosamente.";
    }
}

#endif // !RENAME_H