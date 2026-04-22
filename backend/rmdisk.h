#ifndef RMDISK_H
#define RMDISK_H
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <cstring>
#include <fstream>
#include "structures.h"

using namespace std;
namespace CmdRmDisk
{
	
	inline string expandPath(const string& path){
		if(path.empty() || path[0] != '~'){
			return path;
		}

		const char* home = getenv("HOME");

		if(!home){
			cerr << "Error: el directorio HOME no existe" << endl;
			return path;
		}

		return string(home) + path.substr(1);
	}
	
	inline string rmdisk(const string& path){ 
		try{
			string expandedPath = expandPath(path);
			
			ifstream checkFile(expandedPath);
			
			if(!checkFile.good()){ 
				checkFile.close();
				return "Error: en el disco no existe la ruta ";
			
			}
			
			checkFile.close();
			
			ifstream diskFile(expandedPath, ios::binary);
			MBR mbr;
			diskFile.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
			diskFile.close();
			
			char timeStr[20];
			struct tm* timeInfo = localtime(&mbr.mbr_creation_date);
			strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeInfo);
			
			string info = "Disco eliminado:\n" +
					string("  Ruta: ") + expandedPath + "\n" +
					string("  Fecha creación: ") + timeStr + "\n" +
					string("  Tamaño: ") + to_string(mbr.mbr_size) + " bytes\n" +
					string("  Firma: ") + to_string(mbr.mbr_disk_signature);

			if (remove(expandedPath.c_str()) != 0 ){ 
				return "Error: no se pudo eliminar el archivo del disco";
			}
			
			return info;
		}
		catch (const std::exception& e){
			return string("Error al intentar eliminar el disco: ") + e.what();
			
		}
	}
}

#endif // !RMDISK_H