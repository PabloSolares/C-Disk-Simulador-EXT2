#include "chmod.h"
#include "chown.h"
#include "crow_all.h"
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
#include "find.h"
#include "journal.h"
#include "loss.h"
#include "mkdir.h"
#include "mkfile.h"
#include "mkfs.h"
#include "mkgpr.h"
#include "mkusr.h"
#include "move.h"
#include "remove.h"
#include "rename.h"
#include "rep.h"
#include "rmgrp.h"
#include "rmusr.h"
#include "session.h"
#include "structures.h"
#include "mkdisk.h" 
#include "rmdisk.h"    
#include "fkdisk.h"     
#include "mount.h"
#include "copy.h"


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
    if(pos == string::npos) return "";

    size_t firstValue = pos + paramName.length() + 1;
    if(firstValue >= cmdLine.length()) return "";

    size_t lastValue = cmdLine.length();

    if(cmdLine[firstValue] == '"' || cmdLine[firstValue] == '\''){
        char qt = cmdLine[firstValue];
        size_t qtEnd = cmdLine.find(qt, firstValue + 1) ;

        if(qtEnd != string::npos){
            return cmdLine.substr(firstValue + 1, qtEnd - firstValue - 1);
        } else {
            size_t spacePos = cmdLine.find(' ', firstValue + 1);
            if(spacePos != string::npos) lastValue = spacePos;
            return cmdLine.substr(firstValue + 1, lastValue - firstValue - 1);
        }
    } else {
        size_t spacePos = cmdLine.find(' ', firstValue);
        if(spacePos != string::npos ) lastValue = spacePos;
        return cmdLine.substr(firstValue, lastValue - firstValue);
    }
}


string exeCmd(const string& linea){
    istringstream iss(linea);
    string cmd;
    iss >> cmd;
    cmd = toLowerString(cmd);
    
    if(cmd == "mkdisk"){
        istringstream check(linea);
        string tk;
        check >> tk; 
        while(check >> tk) {
            if(tk[0] == '-' && !isdigit(tk[1])) { 
                string param = toLowerString(tk.substr(0, tk.find('='))); 
                if(param != "-size" && param != "-unit" && param != "-path" && param != "-fit") {
                    return "Error: parametro no reconocido en mkdisk: '" + param + "'";
                }
            }
        }
        string sizeString = parseParameter(linea, "-size");
        string unitString = parseParameter(linea, "-unit");
        string pathString = parseParameter(linea, "-path");
        
        if(sizeString.empty() || pathString.empty()) return "Error: mkdisk require obligatoriamente -size y -path";
        
        int size;
        try { size = stoi(sizeString); } catch (...) { return "Error: el parametro -size debe ser entero"; }
        if(size <= 0) return "Error: el tamaño -size debe ser entero positivo";
        
        if(unitString.empty() ) unitString = "m"; else unitString = toLowerString(unitString);
        if(unitString != "k" && unitString != "m") return "Error: unit debe ser 'k' o 'm'";

        return CmdMkDisk::mkdisk(size, unitString, removeQt(pathString));

    } else if(cmd == "rmdisk"){
        string path = parseParameter(linea, "-path");
        if(path.empty()) return "Error: rmdisk require obligatoriamente -path";
        return CmdRmDisk::rmdisk(removeQt(path));
        
    } else if(cmd == "mount"){
        string path = parseParameter(linea, "-path");
        string name = parseParameter(linea,"-name");
        if(path.empty() || name.empty()) return "Error: mount require obligatoriamente -path y -name";
        return CmdMount::mount(removeQt(path), removeQt(name));

    } else if(cmd  == "mounted"){
        return CmdMount::listMountedPart();

    } else if(cmd == "fdisk"){
        string path = parseParameter(linea, "-path");
        string name = parseParameter(linea, "-name");
        string deleteType = parseParameter(linea, "-delete");
        string addStr = parseParameter(linea, "-add");
        string sizeStr = parseParameter(linea, "-size");
        string unit = parseParameter(linea, "-unit");
        string type = parseParameter(linea, "-type");
        string fit = parseParameter(linea, "-fit");

        if(path.empty()) return "Error: fdisk requiere el parametro -path";
        if(name.empty()) return "Error: fdisk requiere el parametro -name ";

        int size = 0;
        int add_val = 0;

        if(!deleteType.empty()){
            return CmdFkDisk::executeCmd(0, 0, unit, removeQt(path), type, fit, removeQt(deleteType), removeQt(name));
        } else if (!addStr.empty()) {
            try {
                add_val = stoi(addStr);
            } catch (...) {
                return "Error: el valor de -add debe ser un numero entero";
            }

            return CmdFkDisk::executeCmd(0, add_val, unit, removeQt(path), type, fit, "", removeQt(name));
        } else {
            if(sizeStr.empty()){
                return "Error: fdisk requiere el parametro -size al crear";
            }

            try { 
                size = stoi(sizeStr); 
            } catch(...) { 
                return "Error: el valor de size debe ser un numero entero positivo"; 
            }

            if(size <= 0) return "Error: el tamaño debe ser mayor a 0";

            return CmdFkDisk::executeCmd(size, 0, unit, removeQt(path), type, fit, "", removeQt(name));
        
        }


    } else if(cmd == "mkfs"){
        string id = parseParameter(linea, "-id");
        string type = parseParameter(linea, "-type");
        string fs = parseParameter(linea, "-fs");

        if(id.empty()) return "Error: mkfs requiere el parametro -id";
        return CmdMkfs::CmdExecute(removeQt(id), removeQt(type), removeQt(fs));

    } else if(cmd == "login" || cmd == "logout"){
        return "Error: El comando '" + cmd + "' ha sido deshabilitado de la consola. Por favor, utilice la interfaz grafica para gestionar su sesion.";

    } else if(cmd == "mkgrp"){
        string name = parseParameter(linea, "-name");
        if(name.empty()) return "Error: mkgrp requiere los parametros -name";
        return CmdMkgrp::executeCmd(removeQt(name));

    } else if(cmd == "mkusr"){
        string user = parseParameter(linea, "-user");
        string pass = parseParameter(linea, "-pass");
        string grp = parseParameter(linea, "-grp");
        if(user.empty() || pass.empty() || grp.empty() ) return "Error: mkusr requiere los parametros -user, -pass, -grp";
         return CmdMkusr::excuteCmd(removeQt(user), removeQt(pass), removeQt(grp));

    } else if(cmd == "rmgrp"){
        string name = parseParameter(linea, "-name");
        if(name.empty() ) return "Error: rmgpr requiere los parametros -name";
        return CmdRmgrp::CmdExecuteRmgrp(removeQt(name));

    } else if(cmd == "rmusr"){
        string user = parseParameter(linea, "-user");
        if(user.empty() ) return "Error: rmusr requiere los parametros -user";
        return CmdRmUsr::excuteCmd(removeQt(user));

    } else if(cmd == "chgrp"){
        string user = parseParameter(linea, "-user");
        string grp = parseParameter(linea, "-grp");
        if(user.empty() || grp.empty() ) return "Error: chgrp requiere los parametros -user, -grp";
        return CmdChgrp::executeCmd(removeQt(user), removeQt(grp));

    } else if(cmd == "mkdir"){
        string path = parseParameter(linea, "-path");
        bool createParents = hasFlag(linea, "-p");
        if (path.empty()) return "Error: mkdir requiere el parametro obligatorio -path";
        return CmdMkdir::executeCmd(removeQt(path), createParents);

    } else if(cmd == "mkfile"){
        string path = parseParameter(linea, "-path");
        string sizeStr = parseParameter(linea, "-size");
        string contPath = parseParameter(linea, "-cont"); 
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
                result += CmdCat::CmdExecuteCat(removeQt(path)) + "\n";
            }
        }
        if(!foundAnyFile) return "Error: cat requiere al menos el parametro obligatorio -file1";
        if(!result.empty() && result.back() == '\n') result.pop_back();
        return result;
        
    } else if(cmd == "rep"){
        string name = parseParameter(linea, "-name");
        string path = parseParameter(linea, "-path");
        string id = parseParameter(linea, "-id");
        string pathFileLs = parseParameter(linea, "-path_file_ls");
        if (name.empty() || path.empty() || id.empty()) return "Error: rep requiere -name, -path y -id\n";
        return CmdRep::execute(removeQt(name), removeQt(path), removeQt(id), removeQt(pathFileLs));

    } else if (cmd == "unmount"){
        string id = parseParameter(linea, "-id");
        return CmdMount::unmount(removeQt(id));
    } else if (cmd == "journaling"){
        string id = parseParameter(linea, "-id");
        return FsJournal::exucuteJournaling(removeQt(id));
    } else if (cmd == "loss"){
        string id = parseParameter(linea, "-id");
        return CmdLoss::executeCmd(removeQt(id));
    }  else if (cmd == "remove"){
        string path = parseParameter(linea, "-path");
        return CmdRemove::exucuteCmd(removeQt(path));
    }  else if (cmd == "rename"){

        string path = parseParameter(linea, "-path");
        string name = parseParameter(linea, "-name");

        return CmdRename::executeCmd( removeQt(path), removeQt(name));
    }  else if (cmd == "copy") {
        string path = parseParameter(linea, "-path");
        string destino = parseParameter(linea, "-destino");
        return CmdCopy::exucuteCmd(removeQt(path), removeQt(destino));

    }  else if (cmd == "move") {
        string path = parseParameter(linea, "-path");
        string destino = parseParameter(linea, "-destino");

        return CmdMove::executeCmd(removeQt(path), removeQt(destino));
    } else if (cmd == "chmod") {
        string pathStr = parseParameter(linea, "-path");
        string ugoStr = parseParameter(linea, "-ugo");
        
        bool recursivo = (linea.find("-r") != string::npos || linea.find("-R") != string::npos);

        if (pathStr.empty()) return "Error: chmod requiere el parametro -path";
        if (ugoStr.empty()) return "Error: chmod requiere el parametro -ugo";

       
        if (ugoStr.length() != 3) {
            return "Error: el parametro -ugo debe tener exactamente 3 digitos (ej. 777)";
        }
        for (char c : ugoStr) {
            if (c < '0' || c > '7') {
                return "Error: permisos invalidos. Solo se aceptan digitos del 0 al 7 (base octal)";
            }
        }

        int ugo;
        try { 
            ugo = stoi(ugoStr); 
        } catch(...) { 
            return "Error: el valor de -ugo debe ser un numero (ej. 777)"; 
        }

        return CmdChmod::executeCmd(removeQt(pathStr), ugo, recursivo);

    } else if (cmd == "chown") {
        string pathStr = parseParameter(linea, "-path");
        string usrStr = parseParameter(linea, "-user");
        
        bool recursivo = (linea.find("-r") != string::npos || linea.find("-R") != string::npos);

        if (pathStr.empty()) return "Error: chown requiere el parametro -path";
        if (usrStr.empty()) return "Error: chown requiere el parametro -user";

        return CmdChown::executeCmd(removeQt(pathStr), removeQt(usrStr), recursivo);
    } else if (cmd == "find") {
        string pathStr = parseParameter(linea, "-path");
        string nameStr = parseParameter(linea, "-name");
        

        return CmdFind::executeCmd(removeQt(pathStr), removeQt(nameStr));
    } 

    
    else if(cmd == "exit" || cmd.empty()){
        return "";
    } else {
        return "Error: comando no reconcido: " + cmd;
    }
}

string executeScriptFromText(const string& scriptContent) {
    istringstream stream(scriptContent);
    string line;
    string outputGlobal = "";

    while (getline(stream, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");
        if (start == string::npos) continue; 
        line = line.substr(start, end - start + 1);

        if (line[0] == '#') {
            continue;
        }

        outputGlobal += "\n> " + line + "\n"; 
        
        string result = exeCmd(line); 
        if (!result.empty()) {
            outputGlobal += result + "\n";
        }
    }
    return outputGlobal;
}

int main() {
    srand(time(nullptr));
    
    crow::App<crow::CORSHandler> app;
    
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors
      .global()
      .headers("X-Custom-Header", "Upgrade-Insecure-Requests", "Content-Type", "Accept", "Origin")
      .methods("POST"_method, "GET"_method, "OPTIONS"_method)
      .origin("*"); 

    CROW_ROUTE(app, "/")
    ([]() {
        crow::json::wvalue response;
        response["message"] = "Servidor MIA - Proyecto 1 202400044";
        response["version"] = "1.0.0";
        response["status"] = "running";
        return crow::response(200, response);
    });

    CROW_ROUTE(app, "/exec")
    .methods("POST"_method)
    ([](const crow::request& req) {
        crow::json::wvalue response;
        
        try {
            auto body = crow::json::load(req.body);
            
            if (!body) {
                response["success"] = false;
                response["error"] = "JSON invalido en el request body";
                return crow::response(400, response);
            }
            
            if (!body.has("script")) {
                response["success"] = false;
                response["error"] = "El parametro 'script' es obligatorio";
                return crow::response(400, response);
            }
            
            string scriptContent = string(body["script"].s());
            string consoleOutput = executeScriptFromText(scriptContent);
            
            response["success"] = true;
            response["message"] = "Script procesado";
            response["console"] = consoleOutput;
            
            return crow::response(200, response);
            
        } catch (const std::exception& e) {
            response["success"] = false;
            response["error"] = string("Excepción del servidor: ") + e.what();
            return crow::response(500, response);
        }
    });
    
    CROW_ROUTE(app, "/health")
    ([]() {
        crow::json::wvalue response;
        response["status"] = "healthy";
        return crow::response(200, response);
    });

    CROW_ROUTE(app, "/api/login")
    .methods("POST"_method)
    ([](const crow::request& req) {
        crow::json::wvalue response;
        try {
            auto body = crow::json::load(req.body);
            if (!body) return crow::response(400, "JSON invalido");
            
            string user = body["user"].s();
            string pass = body["password"].s();
            string id = body["id"].s();
            
            string result = CmdSession::login(user, pass, id);
            
            if (result.find("Error") != string::npos) {
                response["success"] = false;
                response["error"] = result;
            } else {
                response["success"] = true;
                response["message"] = result;
            }
            return crow::response(200, response);
        } catch (const std::exception& e) {
            response["success"] = false;
            response["error"] = string("Excepción: ") + e.what();
            return crow::response(500, response);
        }
    });

    
    CROW_ROUTE(app, "/api/logout")
    .methods("POST"_method)
    ([]() {
        crow::json::wvalue response;
        response["message"] = CmdSession::logout();
        response["success"] = true;
        return crow::response(200, response);
    });

 
   
    CROW_ROUTE(app, "/api/journaling")
    .methods("POST"_method)
    ([](const crow::request& req) {
        crow::json::wvalue response;
        try {
            auto body = crow::json::load(req.body);
            if (!body) return crow::response(400, "JSON invalido");
            
            string id = body["id"].s();

            CmdMount::MountedPartition partition;
            if (!CmdMount::getMountedPar(id, partition)) {
                response["error"] = "La particion no esta montada";
                return crow::response(404, response);
            }

            string expandedPath = CmdMount::expandPath(partition.path);
            ifstream file(expandedPath, ios::binary);
            if (!file.is_open()) {
                response["error"] = "No se pudo abrir el disco";
                return crow::response(500, response);
            }

            Superblock sb;
            file.seekg(partition.start, ios::beg);
            file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

            if (sb.s_filesystem_type != 3) {
                response["error"] = "La particion no es EXT3. No tiene bitacora.";
                return crow::response(400, response);
            }

            crow::json::wvalue::list bitacora;

            int journalStart = partition.start + sizeof(Superblock);
            
            for (int i = 0; i < 50; i++) {
                Journal entry; 
                file.seekg(journalStart + (i * sizeof(Journal)), ios::beg);
                file.read(reinterpret_cast<char*>(&entry), sizeof(Journal));

                if (strlen(entry.j_content.i_operation) > 0) { 
                    crow::json::wvalue log;
                    log["operacion"] = string(entry.j_content.i_operation);
                    log["ruta"] = string(entry.j_content.i_path);
                    log["contenido"] = string(entry.j_content.i_content);
                    
                    char timeStr[20];
                    struct tm* timeInfo = localtime(&entry.j_content.i_date);
                    
                    if (timeInfo != nullptr) {
                        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeInfo);
                        log["fecha"] = string(timeStr);
                    } else {
                        log["fecha"] = "Fecha invalida";
                    }
                    
                    bitacora.push_back(std::move(log));
                }
            }

            file.close();
            response["data"] = std::move(bitacora);
            return crow::response(200, response);

        } catch (const std::exception& e) {
            response["error"] = string("Excepción: ") + e.what();
            return crow::response(500, response);
        }
    });
  
    
    CROW_ROUTE(app, "/api/discos")
    .methods("GET"_method)
    ([]() {
        crow::json::wvalue response;
        try {
            crow::json::wvalue::list discos_json;
            
            for (const auto& [path, letter] : CmdMount::diskLetters) {
                ifstream file(path, ios::binary);
                if (file.is_open()) {
                    MBR mbr;
                    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
                    file.close();

                    crow::json::wvalue disco;
                    disco["path"] = path;
                    
                    size_t pos = path.find_last_of("/\\");
                    disco["name"] = (pos != string::npos) ? path.substr(pos + 1) : path;
                    disco["size"] = mbr.mbr_size;
                    disco["fit"] = string(1, mbr.disk_fit);

                    crow::json::wvalue::list parts_json;
                    for (int i = 0; i < 4; i++) {
                        if (mbr.mbr_partitions[i].part_status == '1') {
                            crow::json::wvalue p;
                            string partName = string(mbr.mbr_partitions[i].part_name);
                            p["name"] = partName;
                            p["size"] = mbr.mbr_partitions[i].part_size;
                            p["fit"] = string(1, mbr.mbr_partitions[i].part_fit);
                            p["type"] = string(1, mbr.mbr_partitions[i].part_type);

                            bool isMounted = false;
                            string mountId = "";
                            for (const auto& [id, m_part] : CmdMount::mountedPartitions) {
                                if (m_part.path == path && m_part.name == partName) {
                                    isMounted = true;
                                    mountId = id;
                                    break;
                                }
                            }
                            
                            p["estado"] = isMounted ? "Montada" : "No Montada";
                            p["id"] = mountId;
                            parts_json.push_back(std::move(p));
                        }
                    }
                    disco["partitions"] = std::move(parts_json);
                    discos_json.push_back(std::move(disco));
                }
            }

            response["data"] = std::move(discos_json);
            return crow::response(200, response);

        } catch (const std::exception& e) {
            response["error"] = string("Excepción: ") + e.what();
            return crow::response(500, response);
        }
    });
    
    
    CROW_ROUTE(app, "/api/montajes")
    .methods("GET"_method)
    ([]() {
        crow::json::wvalue response;
        try {
            crow::json::wvalue::list particiones;
            
            for (const auto& [clave, particion] : CmdMount::mountedPartitions) {
                crow::json::wvalue p;
                p["id"] = particion.id;
                p["path"] = particion.path;
                p["name"] = particion.name;
                particiones.push_back(std::move(p));
            }

            response["data"] = std::move(particiones);
            return crow::response(200, response);

        } catch (const std::exception& e) {
            response["error"] = string("Excepción: ") + e.what();
            return crow::response(500, response);
        }
    });
    
    CROW_ROUTE(app, "/api/explorar")
    .methods("POST"_method)
    ([](const crow::request& req) {
        crow::json::wvalue response;
        try {
            auto body = crow::json::load(req.body);
            if (!body) {
                response["error"] = "JSON invalido";
                return crow::response(400, response);
            }
            
            string id = body["id"].s();
            string path = body["path"].s();

            CmdMount::MountedPartition partition;
            if (!CmdMount::getMountedPar(id, partition)) {
                response["error"] = "Particion no montada o ID incorrecto";
                return crow::response(400, response);
            }

            string expantedPath = CmdMount::expandPath(partition.path);
            fstream file(expantedPath, ios::binary | ios::in);
            if (!file.is_open()) {
                response["error"] = "No se pudo abrir el disco fisico";
                return crow::response(500, response);
            }

            Superblock sb;
            file.seekg(partition.start, ios::beg);
            file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

            char pathCopy[256];
            strncpy(pathCopy, path.c_str(), 255);
            pathCopy[255] = '\0';
            char* context;
            char* token = strtok_r(pathCopy, "/", &context);

            int currentInodeN = 0;
            Inode currentInode;

            while (token != nullptr) {
                file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
                file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

                char* peek = strtok_r(nullptr, "/", &context);
                
                bool foundNext = false;
                for(int i = 0; i < 12; i++){
                    if(currentInode.i_block[i] != -1){
                        FolderBlock fblock;
                        file.seekg(sb.s_block_start + currentInode.i_block[i] * 64, ios::beg);
                        file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));
                        for(int j = 0; j < 4; j++){
                            if(fblock.b_content[j].b_inodo != -1 && strcmp(fblock.b_content[j].b_name, token) == 0){
                                currentInodeN = fblock.b_content[j].b_inodo;
                                foundNext = true; break;
                            }
                        }
                    }
                    if(foundNext) break;
                }

                if (!foundNext) {
                    file.close();
                    response["error"] = "La ruta no existe en el disco";
                    return crow::response(404, response);
                }
                token = peek;
            }

            file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

            if (currentInode.i_type == '0') {
                response["tipo"] = "carpeta";
                std::vector<crow::json::wvalue> listaElementos;

                for(int i = 0; i < 12; i++) {
                    if (currentInode.i_block[i] != -1) {
                        FolderBlock fb;
                        file.seekg(sb.s_block_start + currentInode.i_block[i] * 64, ios::beg);
                        file.read(reinterpret_cast<char*>(&fb), sizeof(FolderBlock));
                        
                        for (int j = 0; j < 4; j++) {
                            if (fb.b_content[j].b_inodo != -1) {
                                string n = fb.b_content[j].b_name;
                                if (n != "." && n != "..") {
                                    Inode childInode;
                                    file.seekg(sb.s_inode_start + fb.b_content[j].b_inodo * sizeof(Inode), ios::beg);
                                    file.read(reinterpret_cast<char*>(&childInode), sizeof(Inode));

                                    crow::json::wvalue item;
                                    item["nombre"] = n;
                                    item["tipo"] = (childInode.i_type == '0') ? "carpeta" : "archivo";

                                    item["permisos"] = childInode.i_perm; 
                                    item["propietario"] = childInode.i_uid;

                                    listaElementos.push_back(std::move(item));
                                }
                            }
                        }
                    }
                }
                response["contenido"] = std::move(listaElementos);

            } 
            else if (currentInode.i_type == '1') {
                response["tipo"] = "archivo";
                string texto = "";
                int bytesLeidos = 0;

                for(int i = 0; i < 12; i++) {
                    if (currentInode.i_block[i] != -1) {
                        FileBlock fb;
                        file.seekg(sb.s_block_start + currentInode.i_block[i] * 64, ios::beg);
                        file.read(reinterpret_cast<char*>(&fb), sizeof(FileBlock));
                        
                        for (int j = 0; j < 64 && bytesLeidos < currentInode.i_size; j++) {
                            texto += fb.b_content[j];
                            bytesLeidos++;
                        }
                    }
                }
                response["texto"] = texto;
            }

            file.close();
            return crow::response(200, response);

        } catch (const std::exception& e) {
            response["error"] = string("Excepción: ") + e.what();
            return crow::response(500, response);
        }
    });
    
    cout << "\n============================================\n";
    cout << "   C++ BACKEND - MIA PROYECTO 1 - 2026\n";
    cout << "============================================\n";
    cout << " Servidor corriendo en el puerto 8081\n";
    cout << "============================================\n\n";
    
    app.port(8081).multithreaded().run();
    
    return 0;
}