#ifndef LOSS_H
#define LOSS_H
#include <iostream>
#include <fstream>
#include <string>
#include "structures.h"
#include "mount.h"

using namespace std;

namespace CmdLoss {
    inline string executeCmd(const string& id){
        if(id.empty()){
            return "Error: el comnando loss requiere el parametro -id";
        }

        CmdMount::MountedPartition partition;

        if(!CmdMount::getMountedPar(id, partition)){
            return "Error: la particion con ID " + id + " no existe o no esta montada.";
        }

        fstream file(partition.path , ios::binary | ios::in | ios::out);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;
        }

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        if(sb.s_filesystem_type != 3){
            file.close();
            return "Error: El comando loss solo esta soportado para particiones EXT3.";
        }

        char zero = '\0';

        file.seekp(sb.s_bm_inode_start, ios::beg);
        for(int i = 0; i < sb.s_inodes_count; i++){
            file.write(&zero, 1);
        }

        file.seekp(sb.s_block_start, ios::beg);
        for(int i = 0; i < sb.s_blocks_count; i++){
            file.write(&zero, 1);
        }
        
        file.seekp(sb.s_inode_start, ios::beg);
        for(int i = 0; i < (sb.s_inodes_count * sizeof(Inode)); i++){
            file.write(&zero, 1);
        }

        file.seekp(sb.s_block_start, ios::beg);
        for(int i = 0; i < (sb.s_blocks_count * 64); i++){
            file.write(&zero, 1);
        }

        file.close();
        return "Comando LOSS ejecutado exitosamente. Se ha simulado un fallo en la particion " + id + ".";
    }
}

#endif // !LOSS_H