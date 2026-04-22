#ifndef MOVE_H
#define MOVE_H

#include <cstddef>
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include "structures.h"
#include "mount.h"
#include "journal.h"
#include "session.h"
#include "copy.h" 

using namespace std;

namespace CmdMove {

    inline string executeCmd(const string& path, const string& destino){
        if(!currentUser.active){
            return "Error: no hay una sesion activa";
        }
        if (path.empty() || destino.empty()) {
            return "Error: move requiere los parametros -path y -destino";
        }
        if (path == destino) {
            return "Error: El origen y el destino no pueden ser la misma ruta.";
        }

        CmdMount::MountedPartition partition;
        string currentId = currentUser.partitionId;
        if(!CmdMount::getMountedPar(currentId, partition)){
            return "Error: la particion no esta montada";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::binary | ios::in | ios::out);
        if (!file.is_open()) return "Error: No se pudo abrir el disco.";

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        string destName = "";
        int destInodeNum = CmdCopy::getInodeNum(file, sb, destino, destName);
        if (destInodeNum == -1) {
            file.close(); 
            return "Error: La carpeta destino '" + destino + "' no existe.";
        }

        Inode destInode;
        file.seekg(sb.s_inode_start + destInodeNum * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&destInode), sizeof(Inode));

        if (destInode.i_type != '0') {
            file.close(); 
            return "Error: La ruta destino debe ser una carpeta.";
        }

        if (!CmdCopy::hasReadPermission(destInode, currentUser.uid)) {
            file.close(); 
            return "Error: No tienes permiso de escritura en la carpeta destino.";
        }

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
            if(peek == nullptr) {
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
                        if(fblock.b_content[j].b_inodo != -1 && strcmp(fblock.b_content[j].b_name, token) == 0){
                            currentInodeN = fblock.b_content[j].b_inodo;
                            foundNext = true; break;
                        }
                    }
                }
                if(foundNext) break;
            }
            if (!foundNext) { file.close(); return "Error: La ruta de origen no existe."; }
            token = peek;
        }

        Inode parentInode;
        file.seekg(sb.s_inode_start + parentInodeN * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&parentInode), sizeof(Inode));

        if(!CmdCopy::hasReadPermission(parentInode, currentUser.uid )){
            file.close();
            return "Error: no tienes permiso de escritura en la caperta origen";
        }

        int targetInodeNum = -1;
        int parentBlockPos = -1;
        int parentContentIndex = -1;
        
        for(int i = 0; i < 12; i++){
            if(parentInode.i_block[i] != -1){
                FolderBlock fblock;
                int fbPos = sb.s_block_start + parentInode.i_block[i] * 64;
                file.seekg(fbPos, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                for(int j = 0; j < 4; j++){
                    if(fblock.b_content[j].b_inodo != -1 && strcmp(fblock.b_content[j].b_name, targetName.c_str()) == 0){
                        targetInodeNum = fblock.b_content[j].b_inodo;
                        parentBlockPos = fbPos;
                        parentContentIndex = j;
                        break;
                    }
                }
                if (targetInodeNum != -1) break;
            }
        }

        if (targetInodeNum == -1) { file.close(); return "Error: El archivo o carpeta a mover no existe."; }
        
        if (targetInodeNum == destInodeNum) {
            file.close(); return "Error: No puedes mover una carpeta dentro de si misma o a su misma ubicacion.";
        }

        bool linked = CmdCopy::linkItemToFolder(file, sb, destInodeNum, targetInodeNum, targetName);
        if (!linked) {
            file.close(); return "Error: No hay espacio en la carpeta destino para mover el elemento.";
        }

        FolderBlock parentFbToUpdate;
        file.seekg(parentBlockPos, ios::beg);
        file.read(reinterpret_cast<char*>(&parentFbToUpdate), sizeof(FolderBlock));

        memset(parentFbToUpdate.b_content[parentContentIndex].b_name, 0, 12);
        parentFbToUpdate.b_content[parentContentIndex].b_inodo = -1;

        file.seekp(parentBlockPos, ios::beg);
        file.write(reinterpret_cast<char*>(&parentFbToUpdate), sizeof(FolderBlock));

        parentInode.i_mtime = time(nullptr);
        file.seekp(sb.s_inode_start + parentInodeN * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&parentInode), sizeof(Inode));

        destInode.i_mtime = time(nullptr);
        file.seekp(sb.s_inode_start + destInodeNum * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&destInode), sizeof(Inode));

        file.seekp(partition.start, ios::beg);
        file.write(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        file.close();

        if(sb.s_filesystem_type == 3){
            string contenidoBitacora = "-destino=" + destino;
            FsJournal::addJournalEntry(currentUser.partitionId, "move", path, contenidoBitacora);
        }

        return "Elemento '" + targetName + "' movido hacia '" + destino + "' exitosamente.";
    }
}

#endif // !MOVE_H