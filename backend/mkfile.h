#ifndef MKFILE_H
#define MKFILE_H
#include "journal.h"
#include "mount.h"
#include "structures.h"
#include "session.h"
#include "mkdir.h" 
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <string>

using namespace std;

namespace CmdMkfile {
    inline string executeCmd(const string& path, bool createParents, int sizeParam, const string& contPath){
        
        if(!currentUser.active){
            return "Error: no ya hay una sesion activa";
        }
        // se busca la particion con el nombre de id
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(currentUser.partitionId,partition)){
            return "Error: la particion  no existe";
        }

        string finalContent = "";

        if(!contPath.empty()){
            ifstream localFile(contPath);
            if(!localFile.is_open()){
                return "Error: el archivo en la ruta -cont no existe en su computadora";
            }

            stringstream buffer;
            buffer << localFile.rdbuf();
            finalContent = buffer.str();
            localFile.close();

        } else if(sizeParam > 0) {
            for (int i = 0; i < sizeParam; i++) {
                finalContent += to_string(i % 10); 
            }
        }

        int fileSize = finalContent.length();

        int requiredBlock = (fileSize == 0) ? 1 : (fileSize / 64 ) + ((fileSize % 64 != 0 ) ? 1 : 0);
        if(requiredBlock > 12){
            return "Error: el texto es muy grande, maximo admitido (768 BITS) ";
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
        string targetFileName = "";
        int parentInodeNum = 0;

        while(token != nullptr){
            file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

           

            char* peek = strtok_r(nullptr, "/", &context);
        
            if(peek == nullptr){
                targetFileName = token;
                parentInodeNum = currentInodeN;
                break;
            } else {
                int nxtInodeNum = CmdMkdir::findInParent(file, sb.s_block_start, currentInode, token);

                if(nxtInodeNum == -1){
                    if(createParents){
                        int result = CmdMkdir::makeDir(file, sb, currentInodeN, currentInode, token, currentUser.uid);
                        if (result < 0){
                            file.close();
                            return "Error: no se pudo crear la carpeta padre";
                        }
                        currentInodeN = result;
                    } else {
                        file.close();
                        return "Error: la carpeta intermedia no existe";
                    }
                } else {
                    currentInodeN = nxtInodeNum;
                }

                token = peek;
            }
        }

        if(!CmdMkdir::hasPermission(currentInode, currentUser.uid)){
            file.close();
            return "Error: el usuario no tiene permisos de escritura en la carpeta ";
        }

        if(CmdMkdir::findInParent(file, sb.s_block_start, currentInode, targetFileName.c_str()) != -1){
            file.close();
            return "Error: El archivo " + targetFileName + " ya existe.";
        }

        int newInodeN = CmdMkdir::freeSpace(file, sb.s_bm_inode_start, sb.s_inodes_count);
        if(newInodeN == -1){
            file.close();
            return "Error: no hay inodos libres";
        }

        Inode newFileInode;
        for(int i = 0; i < 15; i++) {
            newFileInode.i_block[i] = -1;
        }
        newFileInode.i_uid = currentUser.uid;
        newFileInode.i_gid = currentUser.uid;
        newFileInode.i_size = fileSize;
        newFileInode.i_type = '1';
        newFileInode.i_perm = 664;


        int charsWrite = 0;
        for(int i = 0; i < requiredBlock; i++){
            int newBlockNum = CmdMkdir::freeSpace(file, sb.s_bm_block_start, sb.s_blocks_count);

            newFileInode.i_block[i] = newBlockNum;
            sb.s_free_blocks_count--;
            
            FileBlock fblock;
            memset(fblock.b_content, 0 , 64);

            for(int j = 0; j < 64 && charsWrite < fileSize; j++){
                fblock.b_content[j] = finalContent[charsWrite];
                charsWrite++;
            }

            file.seekp(sb.s_block_start + newBlockNum * 64, ios::beg);
            file.write(reinterpret_cast<char*>(&fblock), sizeof(FileBlock));

        }

        file.seekp(sb.s_inode_start + newInodeN * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&newFileInode), sizeof(Inode));

        bool linked = false;

        for(int i = 0; i < 12 && !linked; i++){
            if(currentInode.i_block[i] != -1){
                FolderBlock parentBlock;
                file.seekg(sb.s_block_start + currentInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&parentBlock), sizeof(FolderBlock));

                for(int j = 0; j < 4; j++){
                    if(parentBlock.b_content[j].b_inodo == -1){
                        parentBlock.b_content[j].b_inodo = newInodeN;
                        strncpy(parentBlock.b_content[j].b_name, targetFileName.c_str(), 12);

                        file.seekp(sb.s_block_start + currentInode.i_block[i] * 64, ios::beg);
                        file.write(reinterpret_cast<char*>(&parentBlock), sizeof(FolderBlock));
                        linked = true;
                        break;
                    }
                }
            } else {
                int newParentBlock = CmdMkdir::freeSpace(file,sb.s_bm_block_start, sb.s_blocks_count);
                if(newParentBlock == -1) {
                    break;
                };

                currentInode.i_block[i] = newParentBlock;

                sb.s_free_blocks_count--;
                FolderBlock newParentFolderB;
                for(int k = 0; k < 4; k++){
                    newParentFolderB.b_content[k].b_inodo = -1;
                }

                newParentFolderB.b_content[0].b_inodo = newInodeN;
                strncpy(newParentFolderB.b_content[0].b_name , targetFileName.c_str(), 12);

                file.seekp(sb.s_block_start + newParentBlock * 64, ios::beg);
                file.write(reinterpret_cast<char*>(&newParentFolderB), sizeof(FolderBlock));
                linked = true;
            }
        }

        sb.s_free_inodes_count--;
        file.seekp(partition.start, ios::beg);
        file.write(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        currentInode.i_mtime = time(nullptr);
        file.seekp(sb.s_inode_start  + parentInodeNum * sizeof(Inode), ios::beg);
        file.write(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

        file.close();

        if(sb.s_filesystem_type == 3){
            string contenidoBitacora = "-size=" + to_string(sizeParam);
            
            if (!contPath.empty()) {
                contenidoBitacora += " -cont=" + contPath;
            }
            
            if (createParents) {
                contenidoBitacora += " -r"; 
            }
            FsJournal::addJournalEntry(currentUser.partitionId, "mkfile", path, contenidoBitacora);
        }

        return "Archivo + " + targetFileName + " creado con " + to_string(fileSize ) + " bytes";


    }

}
#endif // !MKFILE_H