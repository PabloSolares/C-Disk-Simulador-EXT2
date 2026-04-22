#ifndef CHOWN_H
#define CHOWN_H

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include "structures.h"
#include "mount.h"
#include "journal.h"
#include "session.h"

using namespace std;

namespace CmdChown {
    inline void recursiveChown(fstream& file, Superblock& sb, int inodeNum, int newUid) {
        Inode currentInode;
        file.seekg(sb.s_inode_start + inodeNum * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

        currentInode.i_uid = newUid;
        currentInode.i_mtime = time(nullptr);
        file.seekp(sb.s_inode_start + inodeNum * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

        if (currentInode.i_type == '0') {
            for (int i = 0; i < 12; i++) {
                if (currentInode.i_block[i] != -1) {
                    FolderBlock fBlock;
                    file.seekg(sb.s_block_start + currentInode.i_block[i] * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&fBlock), sizeof(FolderBlock));

                    for (int j = 0; j < 4; j++) {
                        if (fBlock.b_content[j].b_inodo != -1 && 
                            strcmp(fBlock.b_content[j].b_name, ".") != 0 && 
                            strcmp(fBlock.b_content[j].b_name, "..") != 0) {
                            
                            recursiveChown(file, sb, fBlock.b_content[j].b_inodo, newUid);
                        }
                    }
                }
            }
        }
    }

    inline string executeCmd(const string& path, const string& userStr, bool recursivo) {
        if(!currentUser.active) return "Error: no hay una sesion activa";
        if (path.empty() || userStr.empty()) return "Error: chown requiere -path y -usr";

        if (strcmp(currentUser.username, "root") != 0) {
            return "Error: Solo el usuario root puede cambiar de propietario (chown).";
        }

        CmdMount::MountedPartition partition;
        if (!CmdMount::getMountedPar(currentUser.partitionId, partition)) return "Error: particion no montada.";

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::binary | ios::in | ios::out);
        if (!file.is_open()) return "Error: No se pudo abrir el disco.";

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

     
        Inode rootInode;
        file.seekg(sb.s_inode_start, ios::beg);
        file.read(reinterpret_cast<char*>(&rootInode), sizeof(Inode));

        int usersInodeNum = -1;
        for(int i = 0; i < 12; i++){
            if(rootInode.i_block[i] != -1){
                FolderBlock fblock;
                file.seekg(sb.s_block_start + rootInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));
                for(int j = 0; j < 4; j++){
                    if(fblock.b_content[j].b_inodo != -1 && strcmp(fblock.b_content[j].b_name, "users.txt") == 0){
                        usersInodeNum = fblock.b_content[j].b_inodo;
                        break;
                    }
                }
            }
            if(usersInodeNum != -1) break;
        }

        if (usersInodeNum == -1) { file.close(); return "Error: no se encontro el archivo users.txt"; }

        Inode usersInode;
        file.seekg(sb.s_inode_start + usersInodeNum * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&usersInode), sizeof(Inode));

        char usersContent[2048];
        memset(usersContent, 0, sizeof(usersContent));
        int bytesRead = 0;

        for(int i = 0; i < 12; i++){
            if(usersInode.i_block[i] != -1){
                FileBlock fblock;
                file.seekg(sb.s_block_start + usersInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FileBlock));
                for(int j = 0; j < 64 && bytesRead < usersInode.i_size; j++ ){
                    if(bytesRead < 2047){
                        usersContent[bytesRead] = fblock.b_content[j];
                        bytesRead++;
                    }
                }
            }
        }

        int targetUid = -1;
        char* lineContext;
        char* line = strtok_r(usersContent, "\n", &lineContext);

        while(line != nullptr){
            char lineCopy[256];
            strncpy(lineCopy, line, sizeof(lineCopy));

            char data[5][50];
            for(int k = 0; k < 5; k++) memset(data[k], 0, 50);

            int numData = 0;
            char* dataContext;
            char* token = strtok_r(lineCopy, ",", &dataContext);

            while(token != nullptr && numData < 5){
                strncpy(data[numData], token, 49);
                int start = 0;
                while(data[numData][start] == ' ' || data[numData][start] == '\r') start++;
                int end = strlen(data[numData]) - 1;
                while(end > start && (data[numData][end] == ' ' || data[numData][end] == '\r')) end--;
                
                int idx = 0;
                for(int c = start; c <= end; c++) data[numData][idx++] = data[numData][c];
                data[numData][idx] = '\0';
                
                numData++;
                token = strtok_r(nullptr, ",", &dataContext);
            }

            if(numData >= 5 && strcmp(data[1], "U") == 0 && strcmp(data[0], "0") != 0){
                if(strcmp(data[3], userStr.c_str()) == 0){
                    targetUid = atoi(data[0]); 
                    break;
                }                
            }
            line = strtok_r(nullptr, "\n", &lineContext);
        }

        if (targetUid == -1) { file.close(); return "Error: El usuario '" + userStr + "' no existe en el sistema."; }

        char pathCopy[256];
        strncpy(pathCopy, path.c_str(), 255);
        pathCopy[255] = '\0';
        char* context;
        char* pToken = strtok_r(pathCopy, "/", &context);
        
        int currentInodeN = 0; 
        Inode currentInode;

        while (pToken != nullptr) {
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
                        if(fblock.b_content[j].b_inodo != -1 && strcmp(fblock.b_content[j].b_name, pToken) == 0){
                            currentInodeN = fblock.b_content[j].b_inodo;
                            foundNext = true; break;
                        }
                    }
                }
                if(foundNext) break;
            }
            if (!foundNext) { file.close(); return "Error: La ruta '" + path + "' no existe."; }
            pToken = peek;
        }

        if (recursivo) {
            recursiveChown(file, sb, currentInodeN, targetUid);
        } else {
            Inode targetInode;
            file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.read(reinterpret_cast<char*>(&targetInode), sizeof(Inode));

            targetInode.i_uid = targetUid;
            targetInode.i_mtime = time(nullptr);
            file.seekp(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.write(reinterpret_cast<char*>(&targetInode), sizeof(Inode));
        }

        file.close();

        if(sb.s_filesystem_type == 3){
            string contenidoBitacora = "-usr=" + userStr;
            if (recursivo) contenidoBitacora += " -r";
            FsJournal::addJournalEntry(currentUser.partitionId, "chown", path, contenidoBitacora);
        }

        return "Propietario de '" + path + "' cambiado a " + userStr + " (UID: " + to_string(targetUid) + ").";
    }
}
#endif // !CHOWN_H