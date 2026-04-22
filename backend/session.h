#ifndef SESSION_H
#define SESSION_H

#include "mount.h"
#include "structures.h"
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

// Usuario actual logeado
static LoggedUser currentUser;

namespace CmdSession {

    // Busca el inodo de un archivo que esta en la raiz
    inline int getRootFileNode(fstream& file, int sb_inode_start, int sb_block_start, const char* fileName){
        Inode rootInode;

        // empezamos desde el inodo 0 porque ahi esta el root
        file.seekg(sb_inode_start, ios::beg);
        file.read(reinterpret_cast<char*>(&rootInode), sizeof(rootInode));

        // se itera en los 12 aputadores directos de la raiz
        for(int i = 0; i < 12; i++){
            if(rootInode.i_block[i] != -1 ){ // si es -1 no esta en uso
                FolderBlock fblock;
                // calculamos al bloque exacto y se lee
                file.seekg(sb_block_start + rootInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                for(int j = 0; j < 4; j++){
                    if(fblock.b_content[j].b_inodo != -1 ){
                        if(strcmp(fblock.b_content[j].b_name, fileName) == 0){
                            return fblock.b_content[j].b_inodo; // se retorna el numero del inodo 
                        }
                    }
                }
            }
        } 
        return -1;
    }

    inline string login(const string& user, const string& pass, const string& id){
        if(currentUser.active){
            return "Error: ya hay una sesion activa";
        }

        // se busca la particion con el nombre de id
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id,partition)){
            return "Error: la particion con ID " + id + " no existe ";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }

        // obtener einfo del superbloque
        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        // buscar el inodo donde esta users.txt
        int usersInodeNum = getRootFileNode(file, sb.s_inode_start, sb.s_block_start, "users.txt");
        if(usersInodeNum == -1){
            file.close();
            return "Error: no se encontro el archivo users.txt";

        }

        // se lee el archivo users.txt
        Inode usersInode;
        file.seekg(sb.s_inode_start + usersInodeNum * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&usersInode), sizeof(Inode));

        // arreglo para contener el contenio del archivo users
        char content[2048];
        memset(content, 0, sizeof(content));
        int bytesRead = 0;

        for(int i = 0; i < 12; i++){
            if(usersInode.i_block[i] != -1){
                FileBlock fblock;
                file.seekg(sb.s_block_start + usersInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FileBlock));
                
                // leer y extraer caracter pero solo hasta i_size
                for(int j = 0; j < 64 && bytesRead < usersInode.i_size; j++ ){
                    content[bytesRead++] = fblock.b_content[j];
                }
            }
        }

        file.close();

        // separar al archivo por lineas \n 
        char* lineContext;
        char* line = strtok_r(content, "\n", &lineContext);

        while(line != nullptr){
            char data[5][50];

            for(int k = 0; k < 5; k++){
                memset(data[k], 0, 50);
            }

            int numData = 0;
            char* dataContext;

            char* token = strtok_r(line, ",", &dataContext);

            while(token != nullptr && numData < 5){
                int start = 0;
                while(token[start] == ' ' || token[start] == '\r'){
                    start++;
                }

                int end = strlen(token) - 1;

                while(end > start && (token[end] == ' ' || token[end] == '\r')){
                    end--;
                }

                int idx = 0;

                for(int c = start; c <= end; c++){
                    data[numData][idx++] = token[c];
                }

                data[numData][idx] = '\0';
                numData++;

                token = strtok_r(nullptr, ",", &dataContext);
            }


            if(numData >= 5 && strcmp(data[1], "U") == 0 && strcmp(data[0], "0") != 0){
                if(strcmp(data[3], user.c_str()) == 0 && strcmp(data[4], pass.c_str()) == 0){

                    currentUser.active = true;
                    currentUser.uid = atoi(data[0]);
                    strncpy(currentUser.username, user.c_str(), 12);
                    strncpy(currentUser.group, data[2], 12);
                    strncpy(currentUser.partitionId, id.c_str(), 12);

                    return "Sesion inciada exitosamente para: " + user;
                }
            }

            line = strtok_r(nullptr, "\n", &lineContext);
            

        }
        
        return "Error: Usuario o contrasena incorecta";

    }


    inline string logout(){
        if(!currentUser.active){
            return "Error: no hay una sesion activa";
        }

        string user = currentUser.username;

        currentUser.active = false;
        currentUser.uid = -1;
        memset(currentUser.username, 0, sizeof(currentUser.username));
		memset(currentUser.group, 0, sizeof(currentUser.group));
		memset(currentUser.partitionId, 0, sizeof(currentUser.partitionId));

        return "Sesion cerrada exitosamente para: " + user;
    }

}



#endif // !SESSION_H