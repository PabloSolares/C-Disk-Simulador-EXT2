#ifndef MKGPR_H
#define MKGPR_H
#include "journal.h"
#include "mount.h"
#include "structures.h"
#include "session.h" 
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <string>

using namespace std;

namespace CmdMkgrp {
    
    inline string executeCmd(const string& name){
        if(!currentUser.active){
            return "Error:  no hay una sesion activa";
        }
        if(strcmp(currentUser.username, "root") != 0){
            return "Error: Solo el usuario root puede crear grupos";

        }


        // se busca la particion con el nombre de id
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(currentUser.partitionId,partition)){
            return "Error: la particion  ya no existe";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::out | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }

        // obtener einfo del superbloque
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

        if (usersInodeNum == -1) return "Error: no se encontro el archivo users.txt";

        Inode usersInode;

        file.seekg(sb.s_inode_start + usersInodeNum * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&usersInode), sizeof(Inode));


        // arreglo para contener el contenio del archivo users
        char usersContent[2048];
        memset(usersContent, 0, sizeof(usersContent));
        int bytesRead = 0;

        for(int i = 0; i < 12; i++){
            if(usersInode.i_block[i] != -1){
                FileBlock fblock;
                file.seekg(sb.s_block_start + usersInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FileBlock));
                
                // leer y extraer caracter pero solo hasta i_size
                for(int j = 0; j < 64 && bytesRead < usersInode.i_size; j++ ){
                    if(bytesRead < 2047){
                        usersContent[bytesRead] = fblock.b_content[j];
                        bytesRead++;
                    }
                }
            }
        }

        
        
        // se analiza el contenido de users.txt y se calcula el nueov iD
        char contentCopy[2048];
        strncpy(contentCopy, usersContent, sizeof(contentCopy));

        char* lineContext;
        char* line = strtok_r(contentCopy, "\n", &lineContext);
        int maxId = 0;

        while(line != nullptr){
            char lineCopy[256];
            strncpy(lineCopy, line, sizeof(lineCopy));

            char data[5][50];
            for(int k = 0; k < 5; k++){
                memset(data[k], 0, 50);
            }

            int numData = 0;
            char* dataContext;
            char* token = strtok_r(lineCopy, ",", &dataContext);

            while(token != nullptr && numData < 5){
                strncpy(data[numData], token, 49);
                int start = 0;
                while((data[numData][start] == ' ') || (data[numData][start] == '\r')){
                    start++;
                }
                int end = strlen(data[numData]) - 1;

                while(end > start && (data[numData][end] == ' ' || data[numData][end] == '\r')) {
                    end--;
                }

                int idx = 0;
                for(int c = start; c <= end; c++){
                    data[numData][idx++] = data[numData][c];
                }
                data[numData][idx] = '\0';
                
                numData++;
                token = strtok_r(nullptr, ",", &dataContext);
            }

            if(numData >= 3 && strcmp(data[1], "G") == 0 && strcmp(data[0], "0") != 0){
                int currentID = atoi(data[0]);
                if(currentID > maxId){
                    maxId = currentID;
                }

                if(strcmp(data[2], name.c_str()) == 0){
                    file.close();
                    return "Error: el grupo " + name + " ya existe";
                }                
            }
            line = strtok_r(nullptr, "\n", &lineContext);
        }

        string newLine = to_string(maxId + 1) + ",G," + name + "\n";;
        strcat(usersContent, newLine.c_str());
        int newSize = strlen(usersContent);
        
        int bytesWrite = 0;

        for(int i = 0; i < 12; i++){
            if(bytesWrite < newSize){
                
                if(usersInode.i_block[i] == -1){
                    char bit;
                    int freeBlock = -1;
                    for(int b = 0; b < sb.s_blocks_count; b++){
                        file.seekg(sb.s_bm_block_start + b, ios::beg);
                        file.read(&bit, 1);
                        if(bit == '0'){
                            bit = '1';
                            file.seekp(sb.s_bm_block_start + b, ios::beg);
                            file.write(&bit, 1);
                            freeBlock = b;
                            sb.s_free_blocks_count--;
                            break;
                        }
                    }
                    if(freeBlock == -1) { file.close(); return "Error: Disco lleno, no hay bloques disponibles."; }
                    
                    usersInode.i_block[i] = freeBlock; 
                    
                    file.seekp(partition.start, ios::beg);
                    file.write(reinterpret_cast<char*>(&sb), sizeof(Superblock));
                }
                
                FileBlock fblock;
                memset(fblock.b_content, 0, 64);

                for(int j = 0; j < 64 && bytesWrite < newSize; j++){
                    fblock.b_content[j] = usersContent[bytesWrite];
                    bytesWrite++;
                }

                file.seekp(sb.s_block_start + usersInode.i_block[i] * 64, ios::beg);
                file.write(reinterpret_cast<char*>(&fblock), sizeof(FileBlock));

            } else {
                
                break;
            }
        }

        usersInode.i_size = newSize;
        usersInode.i_mtime = time(nullptr);
        file.seekp(sb.s_inode_start + usersInodeNum * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&usersInode), sizeof(Inode));

        file.close();
        
        if(sb.s_filesystem_type == 3){
            string contenidoBitacora = "-name=" + name;
            FsJournal::addJournalEntry(currentUser.partitionId, "mkgrp", "/users.txt", contenidoBitacora);
        }

        file.close();
        return "Grupo " + name + " creado con exito";
        return "Grupo " + name + " creado con exito";
    }
}

#endif // !MKGPR_H