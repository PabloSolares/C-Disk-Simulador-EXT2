#ifndef CHMOD_H
#define CHMOD_H


#include <ctime>
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include "structures.h"
#include "mount.h"
#include "journal.h"
#include "session.h"

using namespace std;

namespace CmdChmod {

    inline void recursiveChmod(fstream& file, Superblock& sb, int inodeNum, int ugo) {
        Inode currentInode;
        file.seekg(sb.s_inode_start + inodeNum * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

        currentInode.i_perm = ugo;
        currentInode.i_mtime = time(nullptr);
        file.seekp(sb.s_inode_start +inodeNum * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

        if(currentInode.i_type == '0'){
            for(int i = 0; i < 12; i++){
                if (currentInode.i_block[i] != -1) {
                    FolderBlock fblock;
                    file.seekg(sb.s_block_start + currentInode.i_block[i] * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                    for(int j  = 0; j < 4; j++){
                        if(fblock.b_content[j].b_inodo != -1 && 
                           strcmp(fblock.b_content[j].b_name, ".") != 0 && 
                           strcmp(fblock.b_content[j].b_name, ".." ) != 0){
                            
                            recursiveChmod(file, sb, fblock.b_content[j].b_inodo, ugo);
                        }
                    }
                }
            }
        }
    }

    inline string executeCmd(const string& path, int ugo, bool recursivo) {
        if(!currentUser.active) return "Error: no hay una sesion activa";
        if (path.empty()) return "Error: chmod requiere el parametro -path";

        CmdMount::MountedPartition partition;
        if (!CmdMount::getMountedPar(currentUser.partitionId, partition)) return "Error: la particion no esta montada.";

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

        while (token != nullptr) {
            file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

            char* peek = strtok_r(nullptr, "/", &context);
            
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

            if (!foundNext) { file.close(); return "Error: La ruta '" + path + "' no existe."; }
            token = peek;
        }
            
        Inode targetInode;
        file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&targetInode), sizeof(Inode));

        if (currentUser.uid != 1 && targetInode.i_uid != currentUser.uid) {
            file.close(); return "Error: Solo el propietario o root pueden cambiar los permisos.";
        }

        if (recursivo) {
            recursiveChmod(file, sb, currentInodeN, ugo);
        } else {
            targetInode.i_perm = ugo;
            targetInode.i_mtime = time(nullptr);
            file.seekp(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.write(reinterpret_cast<char*>(&targetInode), sizeof(Inode));
        }

        file.close();

        if(sb.s_filesystem_type == 3){
            string contenidoBitacora = "-ugo=" + to_string(ugo);
            if (recursivo) contenidoBitacora += " -r";
            FsJournal::addJournalEntry(currentUser.partitionId, "chmod", path, contenidoBitacora);
        }

        return "Permisos cambiados a " + to_string(ugo) + " en '" + path + "' exitosamente.";
    
    }
}

#endif // !CHMOD_H