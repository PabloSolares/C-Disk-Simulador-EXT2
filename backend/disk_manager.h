/*#ifndef DISK_MANAGER_H


#define DISK_MANAGER_H

#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <structures.h>

using namespace std;
class DiskManager {
	
	
	
	static string getDiskInfo(const string& path){ 
		try{
			string expandedPath = expandPath(path);
			ifstream diskFile(expandedPath, ios::binary);
			if(!diskFile.is_open()){ 
				return "Error: no se pudo acceder al disco ";
			}
			
			MBR mbr;
			diskFile.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
			diskFile.close();
			
			char timeStr[20];
			struct tm* timeInfo = localtime(&mbr.mbr_creation_date);
			strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeInfo);
			
			string info = "\n------------INFORMACION DEL DISCO---------:\n" +
					string("  Ruta: ") + expandedPath + "\n" +
					string("  Fecha creación: ") + timeStr + "\n" +
					string("  Tamaño: ") + to_string(mbr.mbr_size) + " bytes\n" +
					string("  Firma: ") + to_string(mbr.mbr_disk_signature) + "\n" + 
					string("\n-----------PARTICIONES-----------------------");
	
			for (int i = 0; i < 4; i++){
				info += " [" + to_string(i + 1 ) + "] ";
				if(mbr.mbr_partitions[i].part_status == '1'){ 
					info += "PARTICION ACTIVA - " + string(mbr.mbr_partitions[i].part_name) + 
						" (" + to_string(mbr.mbr_partitions[i].part_size) + " bytes) \n";
				} else {
					info += "PARTICION INACTIVA \n";
				}
			}
			
			info += "\n ------- ------ \n";
			diskFile.open(expandedPath, ios::binary);
			char buffer[100];
			
			diskFile.read(buffer, 100);
			diskFile.close();
			
			for(int i = 0 ; i< 100; i++){ 
				if(i % 16 == 0 ){ 
					info += string("\n") + (i < 16 ? "0" : "" ) + to_string(i/16) + "0: ";
					char hex[4];
					sprintf(hex, "%02X ", (unsigned char ) buffer[i]);
					info += hex;
				}
				
			}
			return info;					
			

		}
		catch (const std::exception& e){
			return string("Error al acceder al disco: ") + e.what();

		}
	}
};


#endif // !DISK_MANAGER_H
*/