#ifndef CHGRP_H
#define CHGRP_H

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

namespace CmdChgrp {
    inline string executeCmd(const string& user, const string& grp){
        if(!currentUser.active){
            return "Error: no ya hay una sesion activa";
        }
        if(strcmp(currentUser.username, "root") != 0){
            return "Error: Solo el usuario root puede crear usuarios";

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
            if(usersInodeNum != -1) break; // si ya se encontro se deja de leer
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

        // se analiza el contenido de users.txt y se calcula el ID
        char contentCopy[2048];
        strncpy(contentCopy, usersContent, sizeof(contentCopy));

        // se guarada el contenido despues de borrar el usuario
        char newUsersContent[2048]; 
        memset(newUsersContent, 0, sizeof(newUsersContent));

        char* lineContext;
        char* line = strtok_r(contentCopy, "\n", &lineContext);
        bool usersExits = false;
        bool groupExits = false;


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
                while(data[numData][start] == ' ' || data[numData][start] == '\r'){
                    start++;
                }
                int end = strlen(data[numData]) - 1;

                while(end > start && data[numData][end] == ' ') {
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

            string lineKeep = "";

            // si la linea es un grupo, se escribe como estaba
            if(numData >= 3 && strcmp(data[1], "G") == 0){
                if(strcmp(data[2], grp.c_str()) == 0 && strcmp(data[0], "0") != 0){
                    groupExits = true;
                }
                lineKeep = string(data[0]) + ",G," + string(data[2]) + "\n";

            } else if (numData >= 5 && strcmp(data[1], "U") == 0){

                // se valida si es el usuario buscado
                if(strcmp(data[3], user.c_str()) == 0 && strcmp(data[0], "0") != 0){
                    usersExits = true;


                    lineKeep = string(data[0]) + ",U," + grp + "," + string(data[3]) + "," + string(data[4]) + "\n";

                } else {

                    lineKeep = string(data[0]) + ",U," + string(data[2]) + "," + string(data[3]) + "," + string(data[4]) + "\n";
                }
            } 

            // se concatena a el contenido nuevo
            strcat(newUsersContent, lineKeep.c_str());

            line = strtok_r(nullptr, "\n", &lineContext);


        }

        if (!usersExits) {
            file.close();
            return "Error: El usuario '" + user + "' no existe o ya fue eliminado.";
        }

        if (!groupExits) {
            file.close();
            return "Error: El grupo '" + grp + "' no existe ";
        }

        int newSize = strlen(newUsersContent);
        int bytesWrite = 0;

        for(int i = 0; i < 12; i++ ){
            if(usersInode.i_block[i] != -1 || bytesWrite < bytesRead){
                FileBlock fblock;
                memset(fblock.b_content, 0, 64);

                for(int j = 0; j < 64 && bytesWrite < newSize; j++){
                    fblock.b_content[j] = newUsersContent[bytesWrite];
                    bytesWrite++;
                }

                file.seekp(sb.s_block_start + usersInode.i_block[i] * 64, ios::beg);
                file.write(reinterpret_cast<char*>(&fblock), sizeof(FileBlock));
            }

            if(bytesWrite > newSize){
                break;
            }
        }

        usersInode.i_size = newSize;
        usersInode.i_mtime = time(nullptr);
        file.seekp(sb.s_inode_start + usersInodeNum * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&usersInode), sizeof(Inode));

        file.close();

        if(sb.s_filesystem_type == 3){
            string contenidoBitacora = "-user=" + user + " -grp=" + grp;
            FsJournal::addJournalEntry(currentUser.partitionId, "chgrp", "/users.txt", contenidoBitacora);
        }
        return "Usuario " + user + " cambiado a " + grp + " correctamente";
    }
}
#endif // !CHGRP_H