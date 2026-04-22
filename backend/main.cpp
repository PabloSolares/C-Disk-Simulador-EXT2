#include <exception>
#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <sys/types.h>
#include "cat.h"
#include "chgrp.h"
#include "mkdir.h"
#include "mkfile.h"
#include "mkfs.h"
#include "mkgpr.h"
#include "mkusr.h"
#include "rep.h"
#include "rmgrp.h"
#include "rmusr.h"
#include "session.h"
#include "structures.h"
#include "mkdisk.h" 
#include "rmdisk.h"    
#include "fkdisk.h"     
#include "mount.h"
#include "mkfs.h"  


using namespace std;

string toLowerString(const string& str){
	string result = str;

	transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

string removeQt(const string& str){
	if(str.length() >= 2 && ((str.front() == '"' && str.back() == '"') || (str.front() == '\'' && str.back() == '\''))) {
		return str.substr(1, str.length() - 2);
	}
	return str;
}

bool hasFlag(const string& line, const string& flag) {
    string lowerLine = toLowerString(line);
    size_t pos = lowerLine.find(flag);
    while (pos != string::npos) {
        bool spaceBefore = (pos == 0 || lowerLine[pos - 1] == ' ');
        bool spaceAfter = (pos + flag.length() == lowerLine.length() || lowerLine[pos + flag.length()] == ' ' || lowerLine[pos + flag.length()] == '\r');
        
        if (spaceBefore && spaceAfter) return true; 
        
        pos = lowerLine.find(flag, pos + 1);
    }
    return false;
}

string parseParameter(const string& cmdLine, const string& paramName){
	string lowerCmdLine = toLowerString(cmdLine);
	string lowerParam = toLowerString(paramName);

	size_t pos = lowerCmdLine.find(lowerParam + "=");
	if(pos == string::npos){
		return "";
	}

	size_t firstValue = pos + paramName.length() + 1;
	if( firstValue >= cmdLine.length()){
		return "";
	}

	size_t lastValue = cmdLine.length();

	if(cmdLine[firstValue] == '"' || cmdLine[firstValue] == '\''){
		char qt = cmdLine[firstValue];
		size_t qtEnd = cmdLine.find(qt, firstValue + 1) ;

		if(qtEnd != string::npos){
			return cmdLine.substr(firstValue + 1, qtEnd - firstValue - 1);
		} else {
			size_t spacePos = cmdLine.find(' ', firstValue + 1);
			if(spacePos != string::npos){
				lastValue = spacePos;
			}

			return cmdLine.substr(firstValue + 1, lastValue - firstValue - 1);

		}
	} else {
		size_t spacePos = cmdLine.find(' ', firstValue);
		if(spacePos != string::npos ){
			lastValue = spacePos;
		}
		
		return cmdLine.substr(firstValue, lastValue - firstValue);
	}
	

}

string exeCmd(const string& linea){
	istringstream iss(linea);
	string cmd;
	iss >> cmd;

	cmd = toLowerString(cmd);
	
	
	if(cmd == "mkdisk"){
		string sizeString = parseParameter(linea, "-size");
		string unitString = parseParameter(linea, "-unit");
		string pathString = parseParameter(linea, "-path");
		
		if(sizeString.empty() || pathString.empty()){
			return "Error: mkdisk require obligatoriamente los parametros -size y -path \n "
			"Ejemplo de uso correcto: mkdisk -size=N -unit=[k|m] -path=ruta\n"
			"No importa el orden de los parametros";
		}
		

		int size;
		try {
			size = stoi(sizeString);
		}
		catch (const std::exception& e){
			return "Error: el parametro -size debe ser entero";
		}

		if(size <= 0){
			return "Error: el tamaño -size debe ser entero positivo";

		}

		if(unitString.empty() ){
			unitString = "m";
		} else {
			unitString = toLowerString(unitString);
		}

		if(unitString != "k" && unitString != "m"){
			return "Error: unit debe ser 'k' (kilobytes) o 'm' (megabytes)";
		}

		return CmdMkDisk::mkdisk(size, unitString, pathString);

	} else if(cmd == "rmdisk"){
		string path = parseParameter(linea, "-path");

		if(path.empty()){
			return "Error: rmdisk require obligatoriamente los parametros -path \n "
			"Ejemplo de uso correcto: rmdisk -path=ruta";
		}
		return CmdRmDisk::rmdisk(path);
		
	} else if(cmd == "mount"){
		string path = parseParameter(linea, "-path");
		string name = parseParameter(linea,"-name");

		if(path.empty() || name.empty()){
			return "Error: mount require obligatoriamente los parametros -path y -name \n "
			"Ejemplo de uso correcto: mount -path=ruta -name=nombre";
		}

		return CmdMount::mount(path, name);
	} else if(cmd  == "mounted"){
		return CmdMount::listMountedPart();
	} else if(cmd == "fdisk"){
		string path = parseParameter(linea, "-path");
		string name = parseParameter(linea, "-name");
		string deleteName = parseParameter(linea, "-delete");

		if(path.empty()){
			return "Error: fdisk require el parametro -path \n "
			"Ejemplo de uso correcto: fdisk -size=N -unit=[k|m] -type=[P|E|L] -fit=[BF|FF|WF] -name=nombre \n" 
			" fdisk -delete=nombre -path=ruta";
		}

		if(!deleteName.empty()){
			return CmdFkDisk::executeCmd(0, "", path, "", "", deleteName, "");
		}

		if(name.empty()){
			return "Error: fdisk require el parametros -name o -delete \n "
				"Ejemplo de uso correcto: fdisk -size=N -unit=[k|m] -type=[P|E|L] -fit=[BF|FF|WF] -name=nombre \n" 
				" fdisk -delete=nombre -path=ruta";
		}

		string sizeStr = parseParameter(linea, "-size");
		if(sizeStr.empty()){
			return "Error: fdisk require el parametro -size \n "
				"Ejemplo de uso correcto: fdisk -size=N -unit=[k|m] -type=[P|E|L] -fit=[BF|FF|WF] -name=nombre" ;
		}

		int size ;
		try {
			size = stoi(sizeStr);
		} catch(exception& e){
			return "Error: el valor de size debe ser un numero entero positvo";
		}

		if( size <= 0){
			return "Error: el tamaño debe ser un entero positivo";
		}

		string unit = parseParameter(linea,"-unit");
		if(unit.empty()){
			unit = "k";
		} else {
			unit = toLowerString(unit);
		}


		if (unit != "k" && unit != "m" && unit != "b") {
            return "Error: unit debe ser k (kilobytes) o m (megabytes)";
        }

		string type = parseParameter(linea, "-type");

		if (type.empty()) {
            type = "P";
        } else {
            type = toLowerString(type);
        }

        std::string fit = parseParameter(linea, "-fit");
        if (fit.empty()) {
            fit = "WF"; 
        } else {
            fit = toLowerString(fit);
        }

		return CmdFkDisk::executeCmd(size, unit, path, type, fit, "", name); 


	}else if(cmd == "mkfs"){
		string id = parseParameter(linea, "-id");
		string type = parseParameter(linea, "-type");

		if(id.empty()){
			return "Error: mkfs requiere el parametro -id \n"
					"Ejemplo de uso: mkfs -id=id [-type= full] (type es opcional)";
		}

		return CmdMkfs::CmdExecute(id, type);
	} else if(cmd == "login"){
		string user = parseParameter(linea, "-user");
		string pass = parseParameter(linea, "-pass");
		string id = parseParameter(linea, "-id");

		if(user.empty() || pass.empty() || id.empty() ){
			return "Error: login requiere los parametros -user, -pass, -id \n"
					"Ejemplo de uso: login -user=root -pass=123 -id=id";
		}

		return CmdSession::login(removeQt(user), removeQt(pass), id);
	} else if(cmd == "logout"){
		return CmdSession::logout();

	} else if(cmd == "mkgrp"){
		string name = parseParameter(linea, "-name");
		
		if(name.empty()){
			return "Error: mkgrp requiere los parametros -name \n"
					"Ejemplo de uso: mkgrp -name=usuarios ";
		}

		return CmdMkgrp::executeCmd(name);
	} else if(cmd == "mkusr"){
		string user = parseParameter(linea, "-user");
		string pass = parseParameter(linea, "-pass");
		string grp = parseParameter(linea, "-grp");

		if(user.empty() || pass.empty() || grp.empty() ){
			return "Error: mkusr requiere los parametros -user, -pass, -grp \n"
					"Ejemplo de uso: mkusr -user=root -pass=123 -grp=grupo1";
		}

		 return CmdMkusr::excuteCmd(user, pass, grp);
	} else if(cmd == "rmgrp"){
		string name = parseParameter(linea, "-name");
		
		if(name.empty() ){
			return "Error: rmgpr requiere los parametros -name \n"
					"Ejemplo de uso: rmgpr -name=grupo1 ";
		}
		return CmdRmgrp::CmdExecuteRmgrp(name);
	}else if(cmd == "rmusr"){
		string user = parseParameter(linea, "-user");
		
		if(user.empty() ){
			return "Error: rmusr requiere los parametros -user \n"
					"Ejemplo de uso: rmusr -user=user1 ";
		}
		return CmdRmUsr::excuteCmd(user);
	} else if(cmd == "chgrp"){
		string user = parseParameter(linea, "-user");
		string grp = parseParameter(linea, "-grp");

		if(user.empty() || grp.empty() ){
			return "Error: chgrp requiere los parametros -user, -grp \n"
					"Ejemplo de uso: chgrp -user=user1 -grp=id";
		}

		return CmdChgrp::executeCmd(user, grp);
	} else if(cmd == "mkdir"){
		string path = parseParameter(linea, "-path");
        string pParam = parseParameter(linea, "-p"); 
        
       	string lowerLine = toLowerString(linea);
        bool createParents = hasFlag(linea, "-p");

        if (path.empty()) return "Error: mkdir requiere el parametro obligatorio -path";
        return CmdMkdir::executeCmd(removeQt(path), createParents);
	} else if(cmd == "mkfile"){
		string path = parseParameter(linea, "-path");
        string rParam = parseParameter(linea, "-r"); 
        string sizeStr = parseParameter(linea, "-size");
        string contPath = parseParameter(linea, "-cont"); 


		string lowerLine = toLowerString(linea);
        bool createParents = (hasFlag(linea, "-r") || hasFlag(linea, "-p"));
        
        
        int sizeVal = 0;
        if (!sizeStr.empty()) {
            sizeVal = stoi(sizeStr);
            if(sizeVal < 0) return "Error: el parametro -size no puede ser negativo."; 
        }

        if (path.empty()) return "Error: mkfile requiere el parametro obligatorio -path";
        
        return CmdMkfile::executeCmd(removeQt(path), createParents, sizeVal, removeQt(contPath));
	} else if(cmd == "cat"){
		string result = "";
        bool foundAnyFile = false;
		for(int i = 1; i <= 20; i++){
            string paramName = "-file" + to_string(i);
            string path = parseParameter(linea, paramName);
            
            if(!path.empty()){
                foundAnyFile = true;
                string content = CmdCat::CmdExecuteCat(removeQt(path));
                
                result += content + "\n";
            }
        }

        if(!foundAnyFile){
            return "Error: cat requiere al menos el parametro obligatorio -file1";
        }

		if(!result.empty() && result.back() == '\n'){
			result.pop_back();
		}

		return result;
		
	} else if(cmd == "rep"){
		std::string name = parseParameter(linea, "-name");
        std::string path = parseParameter(linea, "-path");
        std::string id = parseParameter(linea, "-id");
        std::string pathFileLs = parseParameter(linea, "-path_file_ls");
        
        if (name.empty()) {
            return "Error: rep requiere el parametro -name\n"
                   "Uso: rep -name=name1 -path=ruta -id=id  y opcional path_file_ls=ruta";
        }
        if (path.empty()) {
            return "Error: rep requiere el parametro -path";
        }
        if (id.empty()) {
            return "Error: rep requiere el parametro -id";
        }
        
        return CmdRep::execute(name, path, id, pathFileLs);
	} else if(cmd == "exit"){
		return "EXIT";
	} else if(cmd.empty()){
		return "";
	} else {
		return "Error: comando no reconcido";
	}
};

int main (int argc, char* argv[]){
	srand(time(nullptr));

	cout << "MIA PROYECTO 1 2026 - 202400044 \n";

	string cmdLine;

	while(true){
		cout << "> " ;
		cout.flush();

		if(!getline(cin, cmdLine)){
			cout << "\n SALIENDO... \n";
			break;
		}

		if (cmdLine.empty()) {
			continue;
		}

		string result = exeCmd(cmdLine);
		 if (result == "EXIT") {
			std::cout << "Saliendo del programa...\n";
			break;
		}

		if (!result.empty()) {
			std::cout << result << "\n\n";
		}

	}
	return 0;
}