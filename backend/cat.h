#ifndef CAT_H
#define CAT_H
#include "mount.h"
#include "structures.h"
#include "session.h"
#include "mkdir.h" 
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>

using namespace std;

namespace CmdCat {
    inline bool hasPermission(const Inode& inode, int uid){
        if(uid == 1) return true; //  es el root que tiene todos los permisos

        int perm = inode.i_perm;
        int ownerPerm = perm / 100;
        int otherPerm = perm % 10;

        if(inode.i_uid == uid){
            return (ownerPerm == 4 || ownerPerm == 5 || ownerPerm == 6 || ownerPerm == 7);
        } else {
            return (otherPerm == 4 || otherPerm == 5 || otherPerm == 6 || otherPerm == 7);

        }
    }

    inline string CmdExecuteCat(const string& path){
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

        // Separar la ruta por diagonales /
        char pathCopy[256];
        strncpy(pathCopy, path.c_str(), 255);
        pathCopy[255] = '\0';

        char* context;
        char* token = strtok_r(pathCopy, "/", &context);

        int currentInodeN = 0;
        Inode currentInode; 

        while(token != nullptr){
            file.clear();
            file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

            char* peek = strtok_r(nullptr, "/", &context);

            if(peek == nullptr){
                int fileInodeNum = CmdMkdir::findInParent(file, sb.s_block_start, currentInode, token);

                if(fileInodeNum == -1){
                    file.close();
                    return "Error: el archivo " + string(token) + " no existe";

                }
                currentInodeN = fileInodeNum;
                break;
            } else {
                int nxtInodeNum = CmdMkdir::findInParent(file, sb.s_block_start, currentInode, token);
                if(nxtInodeNum == -1){
                    file.close();
                    return "Error: La ruta intermedia '" + string(token) + "' no existe.";
                }
                currentInodeN = nxtInodeNum;
                token = peek;
            }
        }

        Inode targetFielInode;
        file.clear();
        file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&targetFielInode), sizeof(Inode));

        if(targetFielInode.i_type != '1'){
            file.close();
            return "Error: La ruta indicada apunta a una carpeta, no a un archivo de texto.";
        }

        if(!hasPermission(targetFielInode, currentUser.uid)){
            file.close();
            return "Error: El usuario actual no tiene permisos de lectura para este archivo.";
        }

        string fileContent = "";
        int bytesRead = 0;

        for(int i = 0; i < 12; i++){
            if(targetFielInode.i_block[i] != -1){
                FileBlock fblock;
                file.clear();
                file.seekg(sb.s_block_start + targetFielInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FileBlock));

                for(int j = 0; j < 64 && bytesRead < targetFielInode.i_size; j++){
                    fileContent += fblock.b_content[j];
                    bytesRead++;
                }
            }
        }

        file.close();
        return "\n--- CONTENIDO DE " + path + " ---\n" + fileContent + "\n-----------------------------------";
    }
}

#endif // !CAT_H